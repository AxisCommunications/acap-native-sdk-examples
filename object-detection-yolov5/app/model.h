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

#pragma once

#include "larod.h"

typedef struct model_provider {
    larodConnection* conn;
    larodJobRequest* pp_req;
    larodJobRequest* inf_req;
    size_t yuyv_buffer_size;
    size_t rgb_buffer_size;
    size_t output_tensor_size;

    void* pp_input_addr;
    void* larod_input_addr;  // this address is both used for the output of the
                             // preprocessing and input for the inference
    void* larod_output_addr;

    int larod_model_fd;

    int pp_input_fd;
    int larod_input_fd;  // This file descriptor is used for both as output for the pre
                         // processing and input for the inference
    int larod_output_fd;
} model_provider_t;

void model_run_preprocessing(model_provider_t* provider, uint8_t* data);
uint8_t* model_run_inference(model_provider_t* provider);

model_provider_t* create_model_provider(int input_width,
                                        int input_height,
                                        int stream_width,
                                        int stream_height,
                                        int num_classes,
                                        int num_detections,
                                        char* model_file,
                                        char* device_name);
void destroy_model_provider(model_provider_t* provider);
