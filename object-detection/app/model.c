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
        panic("Still no power available when running larod job %u, giving up", *nbr_of_retries);
    }
    syslog(LOG_INFO, "No power available when running larod job, try nbr %u", *nbr_of_retries);
    *nbr_of_retries = *nbr_of_retries + 1;
    usleep(250 * 1000 * *nbr_of_retries);
}

bool model_run_inference(model_provider_t* provider, VdoBuffer* vdo_buf) {
    larodError* error            = NULL;
    static int nbr_power_retries = 0;

    uint8_t* data = vdo_buffer_get_data(vdo_buf);
    memcpy(provider->image_input_addr, data, provider->image_buffer_size);
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
    nbr_power_retries = 0;
    return true;
}

static bool setup_input_tensor_metadata(unsigned int pitch,
                                        unsigned int height,
                                        larodTensorLayout model_layout,
                                        larodTensor* tensor) {
    larodError* error          = NULL;
    larodTensorPitches pitches = {0};

    // Update the pitch dependent on the layout used
    switch (model_layout) {
        case LAROD_TENSOR_LAYOUT_420SP:
            pitches.len        = 3;
            pitches.pitches[2] = pitch;
            pitches.pitches[1] = height * pitches.pitches[2];
            pitches.pitches[0] = 3 * pitches.pitches[1] / 2;
            break;
        case LAROD_TENSOR_LAYOUT_NHWC:
            pitches.len        = 4;
            pitches.pitches[3] = 3;
            pitches.pitches[2] = pitch;
            pitches.pitches[1] = height * pitches.pitches[2];
            pitches.pitches[0] = 1 * pitches.pitches[1];
            break;
        case LAROD_TENSOR_LAYOUT_NCHW:
            pitches.len        = 4;
            pitches.pitches[3] = pitch;
            pitches.pitches[2] = height * pitches.pitches[3];
            pitches.pitches[1] = 3 * pitches.pitches[2];
            pitches.pitches[0] = 1 * pitches.pitches[1];
            break;
        default:
            break;
    }

    if (!larodSetTensorPitches(tensor, &pitches, &error)) {
        panic("%s: Failed to set tensor pitches: %s", __func__, error->msg);
    }

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
    *output_tensors = larodAllocModelOutputs(conn, model, 0, num_outputs, NULL, &error);
    if (!*output_tensors) {
        panic("%s: Failed retrieving output tensors: %s", __func__, error->msg);
    }
}

static larodModel* create_inference_model(model_provider_t* provider,
                                          char* model_file,
                                          char* device_name,
                                          const char* labels_file) {
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
           "Setting up larod connection with chip %s, model %s and label file %s",
           device_name,
           model_file,
           labels_file);

    const larodDevice* device = larodGetDevice(provider->conn, device_name, 0, &error);
    syslog(LOG_INFO,
           "Loading the model... This might take up to 5 minutes depending on your device model.");
    larodModel* model = larodLoadModel(provider->conn,
                                       provider->larod_model_fd,
                                       device,
                                       LAROD_ACCESS_PRIVATE,
                                       "Object detection model",
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
                               "Object detection model",
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

static larodModel* create_preprocessing_model(model_provider_t* provider, img_info_t* img_info) {
    larodError* error = NULL;
    char* input_format_str;
    char* output_format_str;

    switch (img_info->format) {
        case VDO_FORMAT_YUV:
            input_format_str = "nv12";
            break;
        case VDO_FORMAT_RGB:
            input_format_str = "rgb-interleaved";
            break;
        case VDO_FORMAT_PLANAR_RGB:
            input_format_str = "rgb-planar";
            break;
        default:
            panic("%s: Invalid input format %u", __func__, img_info->format);
    }
    switch (provider->img_info->format) {
        case VDO_FORMAT_YUV:
            output_format_str = "nv12";
            break;
        case VDO_FORMAT_RGB:
            output_format_str = "rgb-interleaved";
            break;
        case VDO_FORMAT_PLANAR_RGB:
            output_format_str = "rgb-planar";
            break;
        default:
            panic("%s: Invalid output format %u", __func__, provider->img_info->format);
    }
    syslog(LOG_INFO,
           "Use preprocessing with input format %s and output format %s",
           input_format_str,
           output_format_str);
    // Create preprocessing maps
    larodMap* map = larodCreateMap(&error);
    if (!map) {
        panic("%s: Could not create preprocessing larodMap %s", __func__, error->msg);
    }
    if (!larodMapSetStr(map, "image.input.format", input_format_str, &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }
    if (!larodMapSetIntArr2(map, "image.input.size", img_info->width, img_info->height, &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }
    if (!larodMapSetInt(map, "image.input.row-pitch", img_info->pitch, &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }
    if (!larodMapSetStr(map, "image.output.format", output_format_str, &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }
    if (!larodMapSetIntArr2(map,
                            "image.output.size",
                            provider->img_info->width,
                            provider->img_info->height,
                            &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }
    if (!larodMapSetInt(map, "image.output.row-pitch", provider->img_info->pitch, &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }

    char* pre_processing_device_str = "cpu-proc";
    // Use libyuv as image preprocessing backend
    const larodDevice* pp_device =
        larodGetDevice(provider->conn, pre_processing_device_str, 0, &error);
    larodModel* model =
        larodLoadModel(provider->conn, -1, pp_device, LAROD_ACCESS_PRIVATE, "", map, &error);
    if (!model) {
        panic("%s: Unable to load preprocessing model with device %s: %s",
              __func__,
              provider->device_name,
              error->msg);
    }

    larodDestroyMap(&map);

    return model;
}

void model_provider_destroy(model_provider_t* provider) {
    larodError* error = NULL;
    if (!provider) {
        panic("%s: Invalid pointer to model_provider_t", __func__);
    }

    larodDestroyModel(&provider->model);
    // Only the model handle is released here. We count on larod service to
    // release the privately loaded model when the session is disconnected in
    // larodDisconnect().
    larodDisconnect(&(provider->conn), NULL);

    if (provider->larod_model_fd >= 0) {
        close(provider->larod_model_fd);
    }
    if (provider->image_input_addr != MAP_FAILED) {
        munmap(provider->image_input_addr, provider->image_buffer_size);
    }
    if (provider->image_input_fd >= 0) {
        close(provider->image_input_fd);
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
    if (provider->img_info) {
        free(provider->img_info);
    }

    larodDestroyTensors(provider->conn,
                        &provider->pp_input_tensors,
                        provider->pp_num_inputs,
                        &error);
    larodDestroyTensors(provider->conn,
                        &provider->pp_output_tensors,
                        provider->pp_num_outputs,
                        &error);
    larodDestroyTensors(provider->conn, &provider->input_tensors, provider->num_inputs, &error);
    larodDestroyTensors(provider->conn, &provider->output_tensors, provider->num_outputs, &error);

    larodDestroyJobRequest(&(provider->pp_req));
    larodDestroyJobRequest(&(provider->inf_req));

    free(provider);
}

model_provider_t* model_provider_new(char* model_file,
                                     char* device_name,
                                     const char* labels_file,
                                     size_t* num_output_tensors) {
    model_provider_t* provider = calloc(1, sizeof(model_provider_t));
    if (!provider) {
        panic("%s: Unable to allocate model_provider_t: %s", __func__, strerror(errno));
    }

    larodError* error = NULL;

    if (!larodConnect(&provider->conn, &error)) {
        panic("%s: Could not connect to larod: %s", __func__, error->msg);
    }

    provider->model = create_inference_model(provider, model_file, device_name, labels_file);
    setup_tensors(provider->conn,
                  provider->model,
                  &provider->input_tensors,
                  &provider->num_inputs,
                  &provider->output_tensors,
                  &provider->num_outputs);
    if (provider->num_inputs > 1) {
        panic("%s Currently only 1 input tensor is supported but %zu was received",
              __func__,
              provider->num_inputs);
    }

    const larodTensorDims* input_dims = larodGetTensorDims(provider->input_tensors[0], &error);
    if (!input_dims) {
        panic("%s: Failed retrieving dim for input tensor: %s", __func__, error->msg);
    }
    provider->img_info = (img_info_t*)calloc(1, sizeof(img_info_t));
    if (!provider->img_info) {
        panic("%s: Unable to allocate img info: %s", __func__, strerror(errno));
    }
    provider->img_info->format = VDO_FORMAT_RGB;
    provider->img_info->width  = input_dims->dims[2];
    provider->img_info->height = input_dims->dims[1];
    syslog(LOG_INFO,
           "Detected model format RGB and input resolution %ux%u",
           provider->img_info->width,
           provider->img_info->height);

    const larodTensorPitches* input_pitches =
        larodGetTensorPitches(provider->input_tensors[0], &error);
    if (!input_pitches) {
        panic("%s: Failed retrieving pitches for input tensor: %s", __func__, error->msg);
    }
    provider->img_info->pitch = input_pitches->pitches[2];

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

    return provider;
}

img_info_t model_provider_get_model_metadata(model_provider_t* provider) {
    return *provider->img_info;
}

bool model_provider_update_image_metadata(model_provider_t* provider, img_info_t* img_info) {
    larodError* error = NULL;

    larodModel* pp_model        = NULL;
    provider->use_preprocessing = false;
    if (img_info->format != provider->img_info->format ||
        provider->img_info->width != img_info->width ||
        provider->img_info->height != img_info->height) {
        provider->use_preprocessing = true;
    }

    if (provider->use_preprocessing) {
        pp_model = create_preprocessing_model(provider, img_info);
        setup_tensors(provider->conn,
                      pp_model,
                      &provider->pp_input_tensors,
                      &provider->pp_num_inputs,
                      &provider->pp_output_tensors,
                      &provider->pp_num_outputs);
        if (provider->pp_num_inputs > 1) {
            panic("%s Currently only 1 pp input tensor is supported but %zu was received",
                  __func__,
                  provider->pp_num_inputs);
        }
        if (provider->pp_num_outputs > 1) {
            panic("%s Currently only 1 pp output tensor is supported but %zu was received",
                  __func__,
                  provider->pp_num_outputs);
        }

        // Needed to be used for copying data
        // No need to setup input tensor metadata since it will all be handled by the map
        // that is setup in the create_preprocessing_model function.
        provider->image_input_fd = larodGetTensorFd(provider->pp_input_tensors[0], &error);
        if (provider->image_input_fd != LAROD_INVALID_FD) {
            // Determine tensor buffer sizes
            if (!larodGetTensorFdSize(provider->pp_input_tensors[0],
                                      &provider->image_buffer_size,
                                      &error)) {
                panic("%s: Could not get byte size of tensor: %s", __func__, error->msg);
            }
            provider->image_input_addr = mmap(NULL,
                                              provider->image_buffer_size,
                                              PROT_READ | PROT_WRITE,
                                              MAP_SHARED,
                                              provider->image_input_fd,
                                              0);
            if (provider->image_input_addr == MAP_FAILED) {
                panic("%s: Could not map pp input tensors fd: %s", __func__, strerror(errno));
            }
        }
    } else {
        if (provider->img_info->pitch != img_info->pitch) {
            panic("%s: Incorrect stream pitch %u != %u",
                  __func__,
                  img_info->pitch,
                  provider->img_info->pitch);
        }
        larodTensorLayout model_layout = LAROD_TENSOR_LAYOUT_UNSPECIFIED;
        if (provider->img_info->format == VDO_FORMAT_RGB) {
            model_layout = LAROD_TENSOR_LAYOUT_NHWC;
        } else if (provider->img_info->format == VDO_FORMAT_PLANAR_RGB) {
            model_layout = LAROD_TENSOR_LAYOUT_NCHW;
        }
        if (model_layout == LAROD_TENSOR_LAYOUT_UNSPECIFIED) {
            panic("%s Model layout unspecified for format %u",
                  __func__,
                  provider->img_info->format);
        }
        setup_input_tensor_metadata(img_info->pitch,
                                    img_info->height,
                                    model_layout,
                                    provider->input_tensors[0]);
        // Needed to be used for copying data
        provider->image_input_fd = larodGetTensorFd(provider->input_tensors[0], &error);
        if (provider->image_input_fd != LAROD_INVALID_FD) {
            // Determine tensor buffer sizes
            if (!larodGetTensorFdSize(provider->input_tensors[0],
                                      &provider->image_buffer_size,
                                      &error)) {
                panic("%s: Could not get byte size of tensor: %s", __func__, error->msg);
            }
            provider->image_input_addr = mmap(NULL,
                                              provider->image_buffer_size,
                                              PROT_READ | PROT_WRITE,
                                              MAP_SHARED,
                                              provider->image_input_fd,
                                              0);
            if (provider->image_input_addr == MAP_FAILED) {
                panic("%s: Could not map input tensors fd: %s", __func__, strerror(errno));
            }
        }
    }

    if (provider->use_preprocessing) {
        // Create job requests
        provider->pp_req = larodCreateJobRequest(pp_model,
                                                 provider->pp_input_tensors,
                                                 provider->pp_num_inputs,
                                                 provider->pp_output_tensors,
                                                 provider->pp_num_outputs,
                                                 NULL,
                                                 &error);
        if (!provider->pp_req) {
            panic("%s: Failed creating preprocessing job request: %s", __func__, error->msg);
        }

        // App supports only one input/output tensor.
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
    } else {
        // App supports only one input/output tensor.
        provider->inf_req = larodCreateJobRequest(provider->model,
                                                  provider->input_tensors,
                                                  provider->num_inputs,
                                                  provider->output_tensors,
                                                  provider->num_outputs,
                                                  NULL,
                                                  &error);
        if (!provider->inf_req) {
            panic("%s: Failed creating inference job request: %s", __func__, error->msg);
        }
    }

    larodDestroyModel(&pp_model);

    return true;
}
