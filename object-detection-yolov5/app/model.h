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
#include "vdo-buffer.h"
#include "vdo-error.h"
#include "vdo-frame.h"
#include "vdo-types.h"

typedef struct model_tensor_output {
    int fd;
    void* data;
    size_t size;
    larodTensorDataType datatype;
} model_tensor_output_t;

typedef struct model_provider {
    larodConnection* conn;
    larodJobRequest* pp_req;
    larodJobRequest* inf_req;

    larodTensor** pp_input_tensors;
    size_t pp_num_inputs;
    larodTensor** pp_output_tensors;
    size_t pp_num_outputs;
    larodTensor** input_tensors;
    size_t num_inputs;
    larodTensor** output_tensors;
    size_t num_outputs;
    larodMap* crop_map;

    size_t image_buffer_size;

    int image_input_fd;
    void* image_input_addr;
    int larod_model_fd;

    bool use_preprocessing;

    model_tensor_output_t* model_output_tensors;
} model_provider_t;

bool model_run_preprocessing(model_provider_t* provider, VdoBuffer* vdo_buf);

bool model_run_inference(model_provider_t* provider, VdoBuffer* vdo_buf);

bool model_get_tensor_output_info(model_provider_t* provider,
                                  unsigned int tensor_output_index,
                                  model_tensor_output_t* tensor_output);

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
                                        size_t* num_output_tensors);

void destroy_model_provider(model_provider_t* provider);
