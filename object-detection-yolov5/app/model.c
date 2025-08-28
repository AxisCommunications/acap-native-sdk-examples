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

bool model_get_tensor_output_info(model_provider_t* provider,
                                  unsigned int tensor_output_index,
                                  model_tensor_output_t* tensor_output) {
    if (tensor_output_index > (provider->num_outputs)) {
        panic("%s: Invalid output index %u", __func__, tensor_output_index);
    }
    *tensor_output = provider->model_output_tensors[tensor_output_index];
    return true;
}

bool model_run_preprocessing(model_provider_t* provider, VdoBuffer* vdo_buf) {
    larodError* error            = NULL;
    static int nbr_power_retries = 0;

    if (!provider->use_preprocessing) {
        return true;
    }
    uint8_t* data = vdo_buffer_get_data(vdo_buf);

    memcpy(provider->image_input_addr, data, provider->image_buffer_size);
    if (!larodRunJob(provider->conn, provider->pp_req, &error)) {
        if (error->code != LAROD_ERROR_POWER_NOT_AVAILABLE) {
            panic("%s: Unable to run preprocessing job: %s (%d)",
                  __func__,
                  error->msg,
                  error->code);
        }
        larodClearError(&error);
        // Currently this will only happen when there is no power
        //  Just a number but if no power available after 50 retries it is time to give up
        if (nbr_power_retries == 50) {
            panic("Still no power available when running larod job %u, giving up",
                  nbr_power_retries);
        }
        syslog(LOG_INFO,
               "No power available when running larod job, try nbr %u",
               nbr_power_retries);
        nbr_power_retries++;
        usleep(250 * 1000 * nbr_power_retries);
        return false;
    }
    nbr_power_retries = 0;
    return true;
}

bool model_run_inference(model_provider_t* provider, VdoBuffer* vdo_buf) {
    larodError* error            = NULL;
    static int nbr_power_retries = 0;

    if (!provider->use_preprocessing) {
        uint8_t* data = vdo_buffer_get_data(vdo_buf);

        memcpy(provider->image_input_addr, data, provider->image_buffer_size);
    }

    if (!larodRunJob(provider->conn, provider->inf_req, &error)) {
        if (error->code != LAROD_ERROR_POWER_NOT_AVAILABLE) {
            panic("%s: Unable to run inference on model: %s (%d)",
                  __func__,
                  error->msg,
                  error->code);
        }
        larodClearError(&error);
        // Currently this will only happen when there is no power
        //  Just a number but if no power available after 50 retries it is time to give up
        if (nbr_power_retries == 50) {
            panic("Still no power available when running larod job %u, giving up",
                  nbr_power_retries);
        }
        syslog(LOG_INFO,
               "No power available when running larod job, try nbr %u",
               nbr_power_retries);
        nbr_power_retries++;
        usleep(250 * 1000 * nbr_power_retries);
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

static larodModel*
create_inference_model(model_provider_t* provider, char* model_file, char* device_name) {
    larodError* error = NULL;

    // Create larod models
    provider->larod_model_fd = open(model_file, O_RDONLY);
    if (provider->larod_model_fd < 0) {
        panic("%s: Unable to open model file %s: %s", __func__, model_file, strerror(errno));
    }

    syslog(LOG_INFO, "Setting up larod connection with device %s", device_name);
    const larodDevice* device = larodGetDevice(provider->conn, device_name, 0, &error);
    syslog(LOG_INFO,
           "Loading the model... This might take up to 5 minutes depending on your device model.");
    larodModel* model = larodLoadModel(provider->conn,
                                       provider->larod_model_fd,
                                       device,
                                       LAROD_ACCESS_PRIVATE,
                                       "object_detection",
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
                               "object_detection",
                               NULL,
                               &error);
        // Sleep between retries
        usleep(250 * 1000 * nbr_power_retries);
        nbr_power_retries++;
        if (nbr_power_retries == 50) {
            panic(
                "%s: Still no power available "
                "when trying to load model %u, giving up",
                __func__,
                nbr_power_retries);
        }
    }
    if (!model) {
        panic("%s: Unable to load model with device %s: %s", __func__, device_name, error->msg);
    }
    syslog(LOG_INFO, "Model loaded successfully");

    return model;
}

static larodModel* create_preprocessing_model(model_provider_t* provider,
                                              char* device_name,
                                              bool allow_input_crop,
                                              VdoFormat input_format,
                                              VdoFormat output_format,
                                              unsigned int input_width,
                                              unsigned int input_height,
                                              unsigned int input_pitch,
                                              unsigned int stream_width,
                                              unsigned int stream_pitch,
                                              unsigned int stream_height) {
    larodError* error = NULL;
    char* input_format_str;
    char* output_format_str;

    switch (input_format) {
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
            panic("%s: Invalid input format %u", __func__, input_format);
    }
    switch (output_format) {
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
            panic("%s: Invalid output format %u", __func__, output_format);
    }

    // Create preprocessing maps
    larodMap* map = larodCreateMap(&error);
    if (!map) {
        panic("%s: Could not create preprocessing larodMap %s", __func__, error->msg);
    }
    if (!larodMapSetStr(map, "image.input.format", input_format_str, &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }
    if (!larodMapSetIntArr2(map, "image.input.size", stream_width, stream_height, &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }
    if (!larodMapSetInt(map, "image.input.row-pitch", stream_pitch, &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }
    if (!larodMapSetStr(map, "image.output.format", output_format_str, &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }
    if (!larodMapSetIntArr2(map, "image.output.size", input_width, input_height, &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }
    if (!larodMapSetInt(map, "image.output.row-pitch", input_pitch, &error)) {
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
              device_name,
              error->msg);
    }

    larodDestroyMap(&map);
    if (allow_input_crop && (input_width != stream_width || input_height != stream_height)) {
        // Calculate crop image
        // 1. The crop area shall fill the input image either horizontally or
        //    vertically.
        // 2. The crop area shall have the same aspect ratio as the output image.
        syslog(LOG_INFO, "Calculate crop image");
        float dest_WH_ratio = (float)input_width / (float)input_height;
        float crop_w        = (float)stream_width;
        float crop_h        = crop_w / dest_WH_ratio;
        if (crop_h > (float)stream_height) {
            crop_h = (float)stream_height;
            crop_w = crop_h * dest_WH_ratio;
        }
        unsigned int clip_w = (unsigned int)crop_w;
        unsigned int clip_h = (unsigned int)crop_h;
        unsigned int clip_x = (stream_width - clip_w) / 2;
        unsigned int clip_y = (stream_height - clip_h) / 2;
        syslog(LOG_INFO, "Crop input image X=%d Y=%d (%d x %d)", clip_x, clip_y, clip_w, clip_h);
        provider->crop_map = larodCreateMap(&error);
        if (!provider->crop_map) {
            panic("Could not create preprocessing crop larodMap %s", error->msg);
        }
        if (!larodMapSetIntArr4(provider->crop_map,
                                "image.input.crop",
                                clip_x,
                                clip_y,
                                clip_w,
                                clip_h,
                                &error)) {
            panic("Failed setting preprocessing parameters: %s", error->msg);
        }
    }

    return model;
}

void destroy_model_provider(model_provider_t* provider) {
    larodError* error = NULL;
    if (!provider) {
        panic("%s: Invalid pointer to model_provider_t", __func__);
    }

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

model_provider_t* create_model_provider(unsigned int input_width,
                                        unsigned int input_height,
                                        unsigned int stream_width,
                                        unsigned int stream_height,
                                        unsigned int stream_pitch,
                                        VdoFormat image_format,
                                        VdoFormat model_format,
                                        char* model_file,
                                        char* device_name,
                                        bool allow_input_crop,
                                        size_t* num_output_tensors) {
    model_provider_t* provider = calloc(1, sizeof(model_provider_t));
    if (!provider) {
        panic("%s: Unable to allocate model_provider_t: %s", __func__, strerror(errno));
    }

    larodError* error = NULL;

    if (!larodConnect(&provider->conn, &error)) {
        panic("%s: Could not connect to larod: %s", __func__, error->msg);
    }

    provider->crop_map   = NULL;
    larodModel* pp_model = NULL;
    larodModel* model    = create_inference_model(provider, model_file, device_name);
    setup_tensors(provider->conn,
                  model,
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
    uint32_t expected_input_width  = 0;
    uint32_t expected_input_height = 0;
    larodTensorLayout model_layout = LAROD_TENSOR_LAYOUT_UNSPECIFIED;
    if (model_format == VDO_FORMAT_RGB) {
        expected_input_width  = input_dims->dims[2];
        expected_input_height = input_dims->dims[1];
        model_layout          = LAROD_TENSOR_LAYOUT_NHWC;
    } else if (model_format == VDO_FORMAT_PLANAR_RGB) {
        expected_input_width  = input_dims->dims[3];
        expected_input_height = input_dims->dims[2];
        model_layout          = LAROD_TENSOR_LAYOUT_NCHW;
    } else {
        panic("%s Invalid model format %u", __func__, model_format);
    }
    if (expected_input_width != input_width || expected_input_height != input_height) {
        panic("%s: Incorrect input resolution %ux%u != %ux%u",
              __func__,
              input_width,
              input_height,
              expected_input_width,
              expected_input_height);
    }
    const larodTensorPitches* input_pitches =
        larodGetTensorPitches(provider->input_tensors[0], &error);
    if (!input_pitches) {
        panic("%s: Failed retrieving pitches for input tensor: %s", __func__, error->msg);
    }
    uint32_t expected_input_pitch = 0;
    if (model_format == VDO_FORMAT_RGB) {
        expected_input_pitch = input_pitches->pitches[2];
    } else if (model_format == VDO_FORMAT_PLANAR_RGB) {
        expected_input_pitch = input_pitches->pitches[3];
    } else {
        panic("%s Invalid model format %u", __func__, model_format);
    }

    provider->use_preprocessing = false;
    if (image_format != model_format || input_width != stream_width ||
        input_height != stream_height) {
        provider->use_preprocessing = true;
    }

    if (provider->use_preprocessing) {
        pp_model = create_preprocessing_model(provider,
                                              device_name,
                                              allow_input_crop,
                                              image_format,
                                              model_format,
                                              input_width,
                                              input_height,
                                              expected_input_pitch,
                                              stream_width,
                                              stream_pitch,
                                              stream_height);
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
        if (expected_input_pitch != stream_pitch) {
            panic("%s: Incorrect stream pitch %u != %u",
                  __func__,
                  stream_pitch,
                  expected_input_pitch);
        }
        setup_input_tensor_metadata(stream_pitch,
                                    stream_height,
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

    if (provider->use_preprocessing) {
        // Create job requests
        provider->pp_req = larodCreateJobRequest(pp_model,
                                                 provider->pp_input_tensors,
                                                 provider->pp_num_inputs,
                                                 provider->pp_output_tensors,
                                                 provider->pp_num_outputs,
                                                 provider->crop_map,
                                                 &error);
        if (!provider->pp_req) {
            panic("%s: Failed creating preprocessing job request: %s", __func__, error->msg);
        }
        larodDestroyMap(&provider->crop_map);

        // App supports only one input/output tensor.
        provider->inf_req = larodCreateJobRequest(model,
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
        provider->inf_req = larodCreateJobRequest(model,
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

    *num_output_tensors = provider->num_outputs;
    larodDestroyModel(&pp_model);
    larodDestroyModel(&model);

    return provider;
}
