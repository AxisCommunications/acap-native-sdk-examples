/**
 * Copyright (C) 2025, Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "model.h"
#include "model_preprocessing.h"

#include "larod.h"
#include "panic.h"

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <syslog.h>
#include <unistd.h>

#define MAX_NBR_POWER_RETRIES 50

bool model_get_tensor_output_info(model_provider_t* provider,
                                  unsigned int tensor_output_index,
                                  model_tensor_output_t* tensor_output) {
    if (tensor_output_index > (provider->num_outputs)) {
        panic("%s: Invalid output index %u", __func__, tensor_output_index);
    }
    *tensor_output = provider->model_output_tensors[tensor_output_index];
    return true;
}

static void model_job_handle_no_power(int* nbr_of_retries) {
    // Currently this will only happen when there is no power
    //  Just a number but if no power available after 50 retries it is time to give up
    if (*nbr_of_retries == MAX_NBR_POWER_RETRIES) {
        panic("%s: Still no power available when running larod job %u, giving up",
              __func__,
              *nbr_of_retries);
    }
    syslog(LOG_INFO, "No power available when running larod job, try nbr %u", *nbr_of_retries);
    *nbr_of_retries = *nbr_of_retries + 1;
    usleep(250 * 1000 * *nbr_of_retries);
}

static int setup_tracked_tensors(model_provider_t* provider, VdoBuffer* vdo_buf) {
    larodError* error    = NULL;
    static int tensor_id = 0;
    int tracked_id       = -1;

    int64_t vdo_buf_offset = vdo_buffer_get_offset(vdo_buf);
    int vdo_buf_fd         = vdo_buffer_get_fd(vdo_buf);
    int buf_fd             = vdo_buf_fd;

    tracked_id                   = tensor_id;
    larodTensor** tracked_tensor = provider->img_input_tensors[tracked_id];

    if (!provider->img_info->dmabuf) {
        buf_fd = larodConvertVmemFdToDmabuf(vdo_buf_fd, vdo_buf_offset, &error);
        if (buf_fd == LAROD_INVALID_FD) {
            panic("%s: Failed to get fd from larod: %s", __func__, error->msg);
        }
        vdo_buf_offset = 0;
    }
    int duped_fd = dup(buf_fd);
    if (duped_fd < 0) {
        panic("%s: Failed to dup fd", __func__);
    }
    if (!larodSetTensorFd(tracked_tensor[0], duped_fd, &error)) {
        panic("%s: Failed to set fd for tensor: %s", __func__, error->msg);
    }
    if (!larodSetTensorFdOffset(tracked_tensor[0], vdo_buf_offset, &error)) {
        panic("%s: Failed to set offset for tensor: %s", __func__, error->msg);
    }
    size_t vdo_buf_capacity = vdo_buffer_get_capacity(vdo_buf);
    if (!larodSetTensorFdSize(tracked_tensor[0], vdo_buf_capacity, &error)) {
        panic("%s: Failed to set size for tensor: %s", __func__, error->msg);
    }
    if (!larodTrackTensor(provider->conn, tracked_tensor[0], &error)) {
        panic("%s: Failed to track tensor: %s", __func__, error->msg);
    }
    provider->img_input_tensors[tracked_id]   = tracked_tensor;
    provider->img_duped_fds[tracked_id]       = duped_fd;
    provider->img_tracked_tensors[tracked_id] = vdo_buf_fd;
    tensor_id++;
    return tracked_id;
}

bool model_run_inference(model_provider_t* provider, VdoBuffer* vdo_buf) {
    larodError* error            = NULL;
    static int nbr_power_retries = 0;
    int tracked_id               = -1;

    int vdo_buf_fd = vdo_buffer_get_fd(vdo_buf);
    if (vdo_buf_fd < 0) {
        panic("%s: fd from vdo_buffer_get_fd is negative", __func__);
    }
    for (size_t i = 0; i < provider->img_info->nbr_buffers; i++) {
        if (provider->img_tracked_tensors[i] == vdo_buf_fd) {
            tracked_id = i;
            break;
        }
    }
    if (tracked_id == -1) {
        tracked_id = setup_tracked_tensors(provider, vdo_buf);
    }

    larodTensor** input_tensors  = provider->img_input_tensors[tracked_id];
    larodJobRequest* input_req   = provider->inf_req;
    larodModel* input_model      = provider->model;
    larodTensor** output_tensors = provider->output_tensors;
    size_t num_outputs           = provider->num_outputs;

    if (provider->use_preprocessing) {
        input_req      = provider->pp_req;
        input_model    = provider->pp_model;
        output_tensors = provider->pp_output_tensors;
        num_outputs    = provider->pp_num_outputs;
    }

    if (!input_req) {
        input_req = larodCreateJobRequest(input_model,
                                          input_tensors,
                                          1,
                                          output_tensors,
                                          num_outputs,
                                          provider->crop_map,
                                          &error);
        if (!input_req) {
            panic("%s: Failed to create input job request: %s", __func__, error->msg);
        }
    } else {
        if (!larodSetJobRequestInputs(input_req, input_tensors, 1, &error)) {
            panic("%s: Failed to set input job request: %s", __func__, error->msg);
        }
    }
    if (provider->use_preprocessing) {
        provider->pp_req = input_req;
    } else {
        provider->inf_req = input_req;
    }

    // If the inference failed because of no power no need to run
    // the preprocssing job again
    if (provider->use_preprocessing) {
        if (!larodRunJob(provider->conn, provider->pp_req, &error)) {
            if (error->code != LAROD_ERROR_POWER_NOT_AVAILABLE) {
                panic("%s: Unable to run preprocessing job: %s (%d)",
                      __func__,
                      error->msg,
                      error->code);
            }
            larodClearError(&error);
            model_job_handle_no_power(&nbr_power_retries);
            return false;
        }
        nbr_power_retries = 0;
        if (!provider->inf_req) {
            provider->inf_req = larodCreateJobRequest(provider->model,
                                                      provider->pp_output_tensors,
                                                      provider->pp_num_outputs,
                                                      provider->output_tensors,
                                                      provider->num_outputs,
                                                      NULL,
                                                      &error);
            if (!provider->inf_req) {
                panic("%s: Failed creating inference job request: %s", __func__, error->msg);
            }
        }
    }

    if (!larodRunJob(provider->conn, provider->inf_req, &error)) {
        if (error->code != LAROD_ERROR_POWER_NOT_AVAILABLE) {
            panic("%s: Unable to run inference on model: %s (%d)",
                  __func__,
                  error->msg,
                  error->code);
        }
        larodClearError(&error);
        model_job_handle_no_power(&nbr_power_retries);
        return false;
    }
    VdoFrame* frame = vdo_buffer_get_frame(vdo_buf);
    uint64_t pts    = vdo_frame_get_timestamp(frame);
    // Update the tensor outputs with the timestamp
    for (size_t i = 0; i < provider->num_outputs; i++) {
        provider->model_output_tensors[i].timestamp = pts;
    }
    nbr_power_retries = 0;
    return true;
}

static void setup_tensors(larodConnection* conn,
                          larodModel* model,
                          larodTensor*** input_tensors,
                          size_t* num_inputs,
                          larodTensor*** output_tensors,
                          size_t* num_outputs) {
    larodError* error = NULL;

    *input_tensors = larodAllocModelInputs(conn, model, 0, num_inputs, NULL, &error);
    if (!*input_tensors) {
        panic("%s: Failed retrieving input tensors: %s", __func__, error->msg);
    }
    *output_tensors = larodAllocModelOutputs(conn,
                                             model,
                                             LAROD_FD_PROP_READWRITE | LAROD_FD_PROP_MAP,
                                             num_outputs,
                                             NULL,
                                             &error);
    if (!*output_tensors) {
        panic("%s: Failed retrieving output tensors: %s", __func__, error->msg);
    }
}

static larodModel*
create_inference_model(model_provider_t* provider, char* model_file, char* device_name) {
    larodError* error = NULL;

    // Create larod models
    provider->larod_model_fd = open(model_file, O_RDONLY);
    if (provider->larod_model_fd < 0) {
        panic("%s: Unable to open model file %s: %s", __func__, model_file, strerror(errno));
    }

    bool found_device  = false;
    size_t num_devices = 0;
    const larodDevice** devices;
    devices = larodListDevices(provider->conn, &num_devices, &error);
    if (num_devices == 0) {
        panic("%s: Unable to list devices: %s", __func__, error->msg);
    }

    const char* device_str = NULL;
    // Check for the supplied device name in all devices supported
    for (size_t i = 0; i < num_devices; ++i) {
        device_str = larodGetDeviceName(devices[i], &error);
        if (!g_strcmp0(device_str, device_name)) {
            found_device = true;
            break;
        }
    }

    if (!found_device) {
        panic("%s: No device found for %s", __func__, device_name);
    }
    provider->device_name = device_name;

    syslog(LOG_INFO,
           "Setting up larod connection with chip %s and model file %s",
           device_name,
           model_file);
    const larodDevice* device = larodGetDevice(provider->conn, device_name, 0, &error);
    syslog(LOG_INFO,
           "Loading the model... This might take up to 5 minutes depending on your device model.");
    larodModel* model = larodLoadModel(provider->conn,
                                       provider->larod_model_fd,
                                       device,
                                       LAROD_ACCESS_PRIVATE,
                                       "Vdo larod model",
                                       NULL,
                                       &error);
    if (!model && error->code != LAROD_ERROR_POWER_NOT_AVAILABLE) {
        panic("%s: Unable to load model: %s", __func__, error->msg);
    }
    uint8_t nbr_power_retries = 1;
    // Retry if there is not enough power to load the model
    while (!model && error != NULL && error->code == LAROD_ERROR_POWER_NOT_AVAILABLE) {
        larodClearError(&error);
        model = larodLoadModel(provider->conn,
                               provider->larod_model_fd,
                               device,
                               LAROD_ACCESS_PRIVATE,
                               "Vdo larod model",
                               NULL,
                               &error);
        // Sleep between retries
        usleep(250 * 1000 * nbr_power_retries);
        nbr_power_retries++;
        if (nbr_power_retries == MAX_NBR_POWER_RETRIES) {
            panic(
                "%s: Still no power available "
                "when trying to load model %u, giving up",
                __func__,
                nbr_power_retries);
        }
    }
    if (!model) {
        panic("%s: Unable to load model with device %s: %s", __func__, device_str, error->msg);
    }
    syslog(LOG_INFO, "Model loaded successfully");

    return model;
}

void model_provider_destroy(model_provider_t* provider) {
    larodError* error = NULL;
    if (!provider) {
        panic("%s: Invalid pointer to model_provider_t", __func__);
    }

    larodDestroyMap(&provider->crop_map);

    larodDestroyModel(&provider->model);
    // Only the model handle is released here. We count on larod service to
    // release the privately loaded model when the session is disconnected in
    // larodDisconnect().
    larodDisconnect(&(provider->conn), NULL);

    if (provider->larod_model_fd >= 0) {
        close(provider->larod_model_fd);
    }
    for (size_t i = 0; i < provider->num_outputs; i++) {
        if (provider->model_output_tensors[i].data != MAP_FAILED) {
            munmap(provider->model_output_tensors[i].data, provider->model_output_tensors[i].size);
        }

        if (provider->model_output_tensors[i].fd >= 0) {
            close(provider->model_output_tensors[i].fd);
        }
    }
    if (provider->model_output_tensors) {
        free(provider->model_output_tensors);
    }
    for (size_t i = 0; i < provider->img_info->nbr_buffers; i++) {
        larodDestroyTensors(provider->conn, &provider->img_input_tensors[i], 1, &error);
        if (provider->img_duped_fds[i] >= 0) {
            close(provider->img_duped_fds[i]);
        }
    }
    if (provider->img_info) {
        free(provider->img_info);
    }

    larodDestroyTensors(provider->conn,
                        &provider->pp_output_tensors,
                        provider->pp_num_outputs,
                        &error);
    larodDestroyTensors(provider->conn, &provider->output_tensors, provider->num_outputs, &error);

    larodDestroyJobRequest(&(provider->pp_req));
    larodDestroyJobRequest(&(provider->inf_req));

    free(provider);
}

model_provider_t*
model_provider_new(char* model_file, char* device_name, size_t* num_output_tensors) {
    model_provider_t* provider = calloc(1, sizeof(model_provider_t));
    if (!provider) {
        panic("%s: Unable to allocate model_provider_t: %s", __func__, strerror(errno));
    }

    larodError* error = NULL;

    if (!larodConnect(&provider->conn, &error)) {
        panic("%s: Could not connect to larod: %s", __func__, error->msg);
    }

    provider->crop_map = NULL;
    provider->pp_req   = NULL;
    provider->inf_req  = NULL;
    // setup a temporary input tensor to be able to
    // get the model information
    // The output tensors will be used for the inference job request
    larodTensor** input_tensors = NULL;
    size_t num_inputs           = 0;
    provider->model             = create_inference_model(provider, model_file, device_name);
    setup_tensors(provider->conn,
                  provider->model,
                  &input_tensors,
                  &num_inputs,
                  &provider->output_tensors,
                  &provider->num_outputs);
    if (num_inputs > 1) {
        panic("%s: Currently only 1 input tensor is supported but %zu was received",
              __func__,
              num_inputs);
    }

    const larodTensorDims* input_dims = larodGetTensorDims(input_tensors[0], &error);
    if (!input_dims) {
        panic("%s: Failed retrieving dim for input tensor: %s", __func__, error->msg);
    }
    provider->img_info = (img_info_t*)calloc(1, sizeof(img_info_t));
    if (!provider->img_info) {
        panic("%s: Unable to allocate img info: %s", __func__, strerror(errno));
    }

    if (input_dims->len != 4) {
        panic("%s: Only input dim = 4 supported %zu", __func__, input_dims->len);
    }
    const char* model_format_str = "RGB";
    if (g_strcmp0(provider->device_name, "ambarella-cvflow") == 0) {
        model_format_str           = "PLANAR RGB";
        provider->img_info->format = VDO_FORMAT_PLANAR_RGB;
        provider->img_info->width  = input_dims->dims[3];
        provider->img_info->height = input_dims->dims[2];
    } else {
        provider->img_info->format = VDO_FORMAT_RGB;
        provider->img_info->width  = input_dims->dims[2];
        provider->img_info->height = input_dims->dims[1];
    }
    syslog(LOG_INFO,
           "Detected model format %s and input resolution %ux%u",
           model_format_str,
           provider->img_info->width,
           provider->img_info->height);
    const larodTensorPitches* input_pitches = larodGetTensorPitches(input_tensors[0], &error);
    if (!input_pitches) {
        panic("%s: Failed retrieving pitches for input tensor: %s", __func__, error->msg);
    }

    if (provider->img_info->format == VDO_FORMAT_RGB) {
        provider->img_info->pitch = input_pitches->pitches[2];
    } else if (provider->img_info->format == VDO_FORMAT_PLANAR_RGB) {
        provider->img_info->pitch = input_pitches->pitches[3];
    } else {
        panic("%s: Invalid model format %u", __func__, provider->img_info->format);
    }
    provider->model_output_tensors = calloc(provider->num_outputs, sizeof(model_tensor_output_t));
    // To be able to get the data from the output tensors get the fd and mmap the memory
    for (size_t i = 0; i < provider->num_outputs; i++) {
        int fd = larodGetTensorFd(provider->output_tensors[i], &error);
        if (fd == LAROD_INVALID_FD) {
            panic("%s: Could not get tensor fd: %s", __func__, error->msg);
        }
        size_t output_size           = 0;
        void* data                   = NULL;
        larodTensorDataType datatype = LAROD_TENSOR_DATA_TYPE_INVALID;

        provider->model_output_tensors[i].fd = fd;
        if (!larodGetTensorFdSize(provider->output_tensors[i], &output_size, &error)) {
            panic("%s: Could not get byte size of tensor: %s", __func__, error->msg);
        }
        provider->model_output_tensors[i].size = output_size;
        data = mmap(NULL, output_size, PROT_READ, MAP_SHARED, fd, 0);
        if (data == MAP_FAILED) {
            panic("%s: Could not map inference output tensors fd: %s", __func__, strerror(errno));
        }
        provider->model_output_tensors[i].data = data;
        datatype = larodGetTensorDataType(provider->output_tensors[i], &error);
        if (datatype == LAROD_TENSOR_DATA_TYPE_INVALID) {
            panic("%s: Could not get output tensor data type: %s", __func__, error->msg);
        }
        provider->model_output_tensors[i].datatype = datatype;
        syslog(LOG_INFO, "Created mmaped model output %zu with size %zu", i, output_size);
    }
    *num_output_tensors = provider->num_outputs;
    larodDestroyTensors(provider->conn, &input_tensors, num_inputs, &error);

    return provider;
}

img_info_t model_provider_get_model_metadata(model_provider_t* provider) {
    return *provider->img_info;
}

bool model_provider_update_image_metadata(model_provider_t* provider, VdoMap* image_map) {
    larodError* error   = NULL;
    img_info_t img_info = {0};

    const char* buffer_type = vdo_map_get_string(image_map, "buffer.type", NULL, "memfd");
    img_info.format         = vdo_map_get_uint32(image_map, "format", 0);
    img_info.width          = vdo_map_get_uint32(image_map, "width", 0);
    img_info.height         = vdo_map_get_uint32(image_map, "height", 0);
    img_info.pitch          = vdo_map_get_uint32(image_map, "pitch", 0);
    img_info.nbr_buffers    = vdo_map_get_uint32(image_map, "buffer.count", 0);
    img_info.dmabuf         = true;
    if (!g_strcmp0(buffer_type, "vmem")) {
        img_info.dmabuf = false;
    }

    provider->use_preprocessing = false;
    if (img_info.format != provider->img_info->format ||
        provider->img_info->width != img_info.width ||
        provider->img_info->height != img_info.height) {
        provider->use_preprocessing = true;
    }

    larodTensorLayout tensor_layout = LAROD_TENSOR_LAYOUT_UNSPECIFIED;
    if (img_info.format == VDO_FORMAT_RGB) {
        tensor_layout = LAROD_TENSOR_LAYOUT_NHWC;
    } else if (img_info.format == VDO_FORMAT_PLANAR_RGB) {
        tensor_layout = LAROD_TENSOR_LAYOUT_NCHW;
    } else if (img_info.format == VDO_FORMAT_YUV) {
        tensor_layout = LAROD_TENSOR_LAYOUT_420SP;
    }
    if (tensor_layout == LAROD_TENSOR_LAYOUT_UNSPECIFIED) {
        panic("%s: Tensor layout unspecified for format %u", __func__, img_info.format);
    }
    provider->img_info->nbr_buffers = img_info.nbr_buffers;
    provider->img_info->dmabuf      = img_info.dmabuf;

    if (!provider->use_preprocessing) {
        if (provider->img_info->pitch != img_info.pitch) {
            panic("%s: Incorrect stream pitch %u != %u",
                  __func__,
                  img_info.pitch,
                  provider->img_info->pitch);
        }
    } else {
        if (!model_preprocessing_setup(provider, &img_info)) {
            panic("%s: Failed to setup preprocessing", __func__);
        }
    }
    // Create the input tensor for the images from vdo
    for (size_t i = 0; i < img_info.nbr_buffers; i++) {
        larodTensor** input_tensors = NULL;
        // Create one input tensor for each buffer from the img provider
        // These input tensors will be used either
        // 1. as input to preprocssing
        // 2. as input to the inference if preprocessing is not needed
        input_tensors = larodCreateTensors(1, &error);
        if (!input_tensors) {
            panic("%s: Failed to create model input [%zu] %s", __func__, i, error->msg);
        }
        if (!larodSetTensorDataType(input_tensors[0], LAROD_TENSOR_DATA_TYPE_UINT8, &error)) {
            panic("%s: Failed to set data type [%zu] %s", __func__, i, error->msg);
        }
        if (!larodSetTensorLayout(input_tensors[0], tensor_layout, &error)) {
            panic("%s: Failed to set tensor layout [%zu] %s", __func__, i, error->msg);
        }
        if (!larodBuildTensorDims(input_tensors[0],
                                  tensor_layout,
                                  img_info.width,
                                  img_info.height,
                                  3,
                                  &error)) {
            panic("%s: Failed to build tensor dims [%zu] %s", __func__, i, error->msg);
        }
        if (!larodBuildTensorPitches(input_tensors[0],
                                     tensor_layout,
                                     img_info.pitch,
                                     img_info.height,
                                     3,
                                     &error)) {
            panic("%s: Failed to build tensor pitches [%zu] %s", __func__, i, error->msg);
        }
        if (!larodSetTensorFdProps(input_tensors[0],
                                   LAROD_FD_PROP_MAP | LAROD_FD_PROP_DMABUF,
                                   &error)) {
            panic("%s: Failed to set fd props [%zu] %s", __func__, i, error->msg);
        }
        provider->img_input_tensors[i] = input_tensors;
    }

    return true;
}
