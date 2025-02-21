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

static void
create_and_map_tmp_file(char* file_name, size_t file_size, void** mapped_addr, int* conv_fd) {
    int fd = mkstemp(file_name);
    if (fd < 0) {
        panic("%s: Unable to open temp file %s: %s", __func__, file_name, strerror(errno));
    }

    // Allocate enough space in for the fd.
    if (ftruncate(fd, (off_t)file_size) < 0) {
        panic("%s: Unable to truncate temp file %s: %s", __func__, file_name, strerror(errno));
    }

    // Remove since we don't actually care about writing to the file system.
    if (unlink(file_name)) {
        panic("%s: Unable to unlink from temp file %s: %s", __func__, file_name, strerror(errno));
    }

    // Get an address to fd's memory for this process's memory space.
    void* data = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (data == MAP_FAILED) {
        panic("%s: Unable to mmap temp file %s: %s", __func__, file_name, strerror(errno));
    }

    *mapped_addr = data;
    *conv_fd     = fd;
}

void model_run_preprocessing(model_provider_t* provider, uint8_t* data) {
    larodError* error = NULL;

    memcpy(provider->pp_input_addr, data, provider->yuyv_buffer_size);
    if (!larodRunJob(provider->conn, provider->pp_req, &error)) {
        panic("%s: Unable to run preprocessing job: %s (%d)", __func__, error->msg, error->code);
    }
}

uint8_t* model_run_inference(model_provider_t* provider) {
    larodError* error = NULL;

    if (!larodRunJob(provider->conn, provider->inf_req, &error)) {
        panic("%s: Unable to run inference on model: %s (%d)", __func__, error->msg, error->code);
    }

    return (uint8_t*)provider->larod_output_addr;
}

static void setup_tensors(larodModel* model,
                          larodTensor*** input_tensors,
                          size_t* num_inputs,
                          larodTensor*** output_tensors,
                          size_t* num_outputs) {
    larodError* error = NULL;

    *input_tensors = larodCreateModelInputs(model, num_inputs, &error);
    if (!*input_tensors) {
        panic("%s: Failed retrieving input tensors: %s", __func__, error->msg);
    }
    *output_tensors = larodCreateModelOutputs(model, num_outputs, &error);
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
    if (!model) {
        panic("%s: Unable to load model with device %s: %s", __func__, device_name, error->msg);
    } else {
        syslog(LOG_INFO, "Model loaded successfully");
    }

    return model;
}

static larodModel* create_preprocessing_model(model_provider_t* provider,
                                              char* device_name,
                                              int input_width,
                                              int input_height,
                                              int stream_width,
                                              int stream_height) {
    larodError* error = NULL;

    // Create preprocessing maps
    larodMap* map = larodCreateMap(&error);
    if (!map) {
        panic("%s: Could not create preprocessing larodMap %s", __func__, error->msg);
    }
    if (!larodMapSetStr(map, "image.input.format", "nv12", &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }
    if (!larodMapSetIntArr2(map, "image.input.size", stream_width, stream_height, &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }
    if (!larodMapSetStr(map, "image.output.format", "rgb-interleaved", &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }
    if (!larodMapSetIntArr2(map, "image.output.size", input_width, input_height, &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }

    // Use libyuv as image preprocessing backend
    const larodDevice* pp_device = larodGetDevice(provider->conn, device_name, 0, &error);
    larodModel* model =
        larodLoadModel(provider->conn, -1, pp_device, LAROD_ACCESS_PRIVATE, "", map, &error);
    if (!model) {
        panic("%s: Unable to load preprocessing model with device %s: %s",
              __func__,
              device_name,
              error->msg);
    }

    larodDestroyMap(&map);

    return model;
}

void destroy_model_provider(model_provider_t* provider) {
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

    if (provider->pp_input_addr != MAP_FAILED) {
        munmap(provider->pp_input_addr, provider->rgb_buffer_size);
    }
    if (provider->pp_input_fd >= 0) {
        close(provider->pp_input_fd);
    }

    if (provider->larod_input_addr != MAP_FAILED) {
        munmap(provider->larod_input_addr, provider->yuyv_buffer_size);
    }
    if (provider->larod_input_fd >= 0) {
        close(provider->larod_input_fd);
    }

    if (provider->larod_output_addr != MAP_FAILED) {
        munmap(provider->larod_output_addr, provider->output_tensor_size);
    }
    if (provider->larod_output_fd >= 0) {
        close(provider->larod_input_fd);
    }

    larodDestroyJobRequest(&(provider->pp_req));
    larodDestroyJobRequest(&(provider->inf_req));

    free(provider);
}

model_provider_t* create_model_provider(int input_width,
                                        int input_height,
                                        int stream_width,
                                        int stream_height,
                                        int num_classes,
                                        int num_detections,
                                        char* model_file,
                                        char* device_name) {
    model_provider_t* provider = calloc(1, sizeof(model_provider_t));
    if (!provider) {
        panic("%s: Unable to allocate model_provider_t: %s", __func__, strerror(errno));
    }

    larodTensor** pp_input_tensors  = NULL;
    size_t pp_num_inputs            = 0;
    larodTensor** pp_output_tensors = NULL;
    size_t pp_num_outputs           = 0;
    larodTensor** input_tensors     = NULL;
    size_t num_inputs               = 0;
    larodTensor** output_tensors    = NULL;
    size_t num_outputs              = 0;

    // Hardcode to use three image "color" channels (eg. RGB).
    const unsigned int CHANNELS = 3;

    provider->output_tensor_size = num_detections * (5 + num_classes);

    // Name patterns for the temp file we will create.
    // The output of the pre-processing correspond with the input of the inference model
    char PP_INPUT_FILE_PATTERN[] = "/tmp/larod.pp.test-XXXXXX";
    char INPUT_FILE_PATTERN[]    = "/tmp/larod.in.test-XXXXXX";
    char OUT_FILE_PATTERN[]      = "/tmp/larod.out.test-XXXXXX";

    larodError* error = NULL;

    if (!larodConnect(&provider->conn, &error)) {
        panic("%s: Could not connect to larod: %s", __func__, error->msg);
    }

    larodModel* model    = create_inference_model(provider, model_file, device_name);
    larodModel* pp_model = create_preprocessing_model(provider,
                                                      "cpu-proc",
                                                      input_width,
                                                      input_height,
                                                      stream_width,
                                                      stream_height);

    setup_tensors(model, &input_tensors, &num_inputs, &output_tensors, &num_outputs);
    setup_tensors(pp_model, &pp_input_tensors, &pp_num_inputs, &pp_output_tensors, &pp_num_outputs);

    // Determine tensor buffer sizes
    if (!larodGetTensorByteSize(pp_input_tensors[0], &provider->yuyv_buffer_size, &error)) {
        panic("%s: Could not get byte size of tensor: %s", __func__, error->msg);
    }
    if (!larodGetTensorByteSize(pp_output_tensors[0], &provider->rgb_buffer_size, &error)) {
        panic("%s: Could not get byte size of tensor: %s", __func__, error->msg);
    }
    size_t expectedSize = input_width * input_height * CHANNELS;
    if (expectedSize != provider->rgb_buffer_size) {
        panic("%s: Expected video output size %zu, actual %zu",
              __func__,
              expectedSize,
              provider->rgb_buffer_size);
    }

    // Allocate space for input tensor
    create_and_map_tmp_file(PP_INPUT_FILE_PATTERN,
                            provider->yuyv_buffer_size,
                            &provider->pp_input_addr,
                            &provider->pp_input_fd);
    create_and_map_tmp_file(INPUT_FILE_PATTERN,
                            provider->rgb_buffer_size,
                            &provider->larod_input_addr,
                            &provider->larod_input_fd);
    create_and_map_tmp_file(OUT_FILE_PATTERN,
                            provider->output_tensor_size,
                            &provider->larod_output_addr,
                            &provider->larod_output_fd);

    // Connect tensors to file descriptors
    if (!larodSetTensorFd(pp_input_tensors[0], provider->pp_input_fd, &error)) {
        panic("%s: Failed setting preprocessing input tensor fd: %s", __func__, error->msg);
    }
    if (!larodSetTensorFd(pp_output_tensors[0], provider->larod_input_fd, &error)) {
        panic("%s: Failed setting preprocessing output tensor fd: %s", __func__, error->msg);
    }
    if (!larodSetTensorFd(input_tensors[0], provider->larod_input_fd, &error)) {
        panic("%s: Failed setting input tensor fd: %s", __func__, error->msg);
    }
    if (!larodSetTensorFd(output_tensors[0], provider->larod_output_fd, &error)) {
        panic("%s: Failed setting output tensor fd: %s", __func__, error->msg);
    }

    // Create job requests
    provider->pp_req = larodCreateJobRequest(pp_model,
                                             pp_input_tensors,
                                             pp_num_inputs,
                                             pp_output_tensors,
                                             pp_num_outputs,
                                             NULL,
                                             &error);
    if (!provider->pp_req) {
        panic("%s: Failed creating preprocessing job request: %s", __func__, error->msg);
    }

    // App supports only one input/output tensor.
    provider->inf_req = larodCreateJobRequest(model,
                                              input_tensors,
                                              num_inputs,
                                              output_tensors,
                                              num_outputs,
                                              NULL,
                                              &error);
    if (!provider->inf_req) {
        panic("%s: Failed creating inference job request: %s", __func__, error->msg);
    }

    larodDestroyModel(&pp_model);
    larodDestroyModel(&model);

    larodDestroyTensors(provider->conn, &pp_input_tensors, pp_num_inputs, &error);
    larodDestroyTensors(provider->conn, &pp_output_tensors, pp_num_outputs, &error);
    larodDestroyTensors(provider->conn, &input_tensors, num_inputs, &error);
    larodDestroyTensors(provider->conn, &output_tensors, num_outputs, &error);

    return provider;
}