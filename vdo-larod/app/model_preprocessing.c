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

#include "model_preprocessing.h"
#include "panic.h"
#include <syslog.h>

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
    if (!larodMapSetInt(map, "image.output.row-pitch", provider->img_info->pitch, &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }

    if (!larodMapSetIntArr2(map,
                            "image.output.size",
                            provider->img_info->width,
                            provider->img_info->height,
                            &error)) {
        panic("%s: Failed setting preprocessing parameters: %s", __func__, error->msg);
    }

    // Use libyuv as image preprocessing backend
    const larodDevice* pp_device = larodGetDevice(provider->conn, "cpu-proc", 0, &error);
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

bool model_preprocessing_setup(model_provider_t* provider, img_info_t* img_info) {
    larodError* error  = NULL;
    provider->pp_model = create_preprocessing_model(provider, img_info);
    // Create the output tensors for the preprocessing
    provider->pp_output_tensors =
        larodAllocModelOutputs(provider->conn,
                               provider->pp_model,
                               LAROD_FD_PROP_READWRITE | LAROD_FD_PROP_MAP,
                               &provider->pp_num_outputs,
                               NULL,
                               &error);
    if (!provider->pp_output_tensors) {
        panic("%s: Failed retrieving output tensors: %s", __func__, error->msg);
    }
    if (provider->pp_num_outputs > 1) {
        panic("%s: Currently only 1 pp output tensor is supported but %zu was received",
              __func__,
              provider->pp_num_outputs);
    }
    const larodTensorDims* output_dims = larodGetTensorDims(provider->pp_output_tensors[0], &error);
    if (!output_dims) {
        panic("%s: Failed retrieving dims for pp output tensor: %s", __func__, error->msg);
    }
    if (output_dims->len != 4) {
        panic("%s: Only output dim = 4 supported %zu", __func__, output_dims->len);
    }

    const larodTensorPitches* output_pitches =
        larodGetTensorPitches(provider->pp_output_tensors[0], &error);
    if (!output_pitches) {
        panic("%s: Failed retrieving pitches for pp output tensor: %s", __func__, error->msg);
    }
    if (output_pitches->len != 4) {
        panic("%s: Only output pitches = 4 supported %zu", __func__, output_pitches->len);
    }
    size_t rgb_buffer_size = 0;
    size_t expected_size   = 3 * provider->img_info->width * provider->img_info->height;
    if (!larodGetTensorByteSize(provider->pp_output_tensors[0], &rgb_buffer_size, &error)) {
        panic("%s: Could not get byte size for pp output tensor: %s", __func__, error->msg);
    }
    if (expected_size != rgb_buffer_size) {
        panic("%s Expected pp module output size %zu, actual %zu",
              __func__,
              expected_size,
              rgb_buffer_size);
    }

    return true;
}
