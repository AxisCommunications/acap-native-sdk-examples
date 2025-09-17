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

#include "imgprovider.h"
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

    size_t image_buffer_size;

    int image_input_fd;
    void* image_input_addr;
    int larod_model_fd;

    bool use_preprocessing;

    img_info_t* img_info;
    model_tensor_output_t* model_output_tensors;
    const char* device_name;
    larodModel* model;
} model_provider_t;

bool model_run_inference(model_provider_t* provider, VdoBuffer* vdo_buf);

bool model_get_tensor_output_info(model_provider_t* provider,
                                  unsigned int tensor_output_index,
                                  model_tensor_output_t* tensor_output);

img_info_t model_provider_get_model_metadata(model_provider_t* provider);

bool model_provider_update_image_metadata(model_provider_t* provider, img_info_t* img_info);

model_provider_t* model_provider_new(char* model_file,
                                     char* device_name,
                                     const char* labels_file,
                                     size_t* num_output_tensors);

void model_provider_destroy(model_provider_t* provider);
