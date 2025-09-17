/**
 * Copyright (C) 2021, Axis Communications AB, Lund, Sweden
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

/**
 * - vdo-larod -
 *
 * This application loads a larod model which takes an image as input and
 * outputs values corresponding to either person or car.
 *
 * The application expects four arguments on the command line in the
 * following order: DEVICENAME MODEL.
 *
 * First argument, DEVICENAME, is a string that is the larod device name
 *
 * Second argument, MODEL, is a string describing path to the model.
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <glib-object.h>
#include <gmodule.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "imgprovider.h"
#include "larod.h"
#include "model.h"
#include "panic.h"
#include "vdo-error.h"
#include "vdo-frame.h"
#include "vdo-types.h"

volatile sig_atomic_t running = 1;

static void shutdown(int status) {
    (void)status;
    running = 0;
}

/**
 * @brief Main function that starts a stream with different options.
 */
int main(int argc, char** argv) {
    char* device_name                     = argv[1];
    char* model_file                      = argv[2];
    g_autoptr(GError) vdo_error           = NULL;
    img_provider_t* image_provider        = NULL;
    model_provider_t* model_provider      = NULL;
    model_tensor_output_t* tensor_outputs = NULL;
    img_info_t model_metadata             = {0};
    img_info_t image_metadata             = {0};

    // Stop main loop at signal
    signal(SIGTERM, shutdown);
    signal(SIGINT, shutdown);

    syslog(LOG_INFO, "Starting %s", argv[0]);

    if (argc != 3) {
        syslog(LOG_ERR,
               "Invalid number of arguments. Required arguments are: "
               "DEVICENAME MODEL_PATH");
        goto end;
    }

    double vdo_framerate           = 30.0;
    unsigned int vdo_input_channel = 1;

    // Start by loading the model and get the model metadata
    size_t number_output_tensors = 0;
    model_provider = model_provider_new(model_file, device_name, &number_output_tensors);
    if (!model_provider) {
        panic("%s: Could not create model provider", __func__);
    }

    tensor_outputs = calloc(number_output_tensors, sizeof(model_tensor_output_t));
    if (!tensor_outputs) {
        panic("%s: Could not allocate tensor outputs", __func__);
    }

    // Get the model format and model input dimension and pitches
    model_metadata = model_provider_get_model_metadata(model_provider);

    // Set crop as scale method meaning that if possible use vdo to get a center cropped
    // image if the native aspect ratio does not match the model resolution aspect ratio.
    image_provider = img_provider_new(vdo_input_channel, &model_metadata, 2, vdo_framerate, "crop");
    if (!image_provider) {
        // It is considered an error if the img provider can not supply the
        // requested stream
        panic("%s: Could not create image provider", __func__);
    }

    image_metadata = img_provider_get_image_metadata(image_provider);
    model_provider_update_image_metadata(model_provider, &image_metadata);

    // An cropped image is required
    // If VDO couldn't supply the correct resolution
    // use larod to perform the crop instead.
    if (image_metadata.width != model_metadata.width ||
        image_metadata.height != model_metadata.height) {
        // Calculate crop image
        // 1. The crop area shall fill the input image either horizontally or
        //    vertically.
        // 2. The crop area shall have the same aspect ratio as the output image.
        unsigned int input_width   = model_metadata.width;
        unsigned int input_height  = model_metadata.height;
        unsigned int stream_width  = image_metadata.width;
        unsigned int stream_height = image_metadata.height;

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
        model_provider_update_crop(model_provider, clip_x, clip_y, clip_w, clip_h);
    }

    syslog(LOG_INFO, "Start fetching video frames from VDO");
    if (!img_provider_start(image_provider)) {
        panic("%s: Could not start image provider", __func__);
    }

    while (running) {
        struct timeval start_ts, end_ts;
        unsigned int inference_ms     = 0;
        unsigned int total_elapsed_ms = 0;

        g_autoptr(VdoBuffer) vdo_buf = img_provider_get_frame(image_provider);
        if (!vdo_buf) {
            // This can only happen if it is global rotation then
            // the stream has to be restarted because rotation has been changed.
            panic("%s: No buffer because of changed global rotation.", __func__);
        }
        // Convert data to the correct format
        gettimeofday(&start_ts, NULL);
        if (!model_run_inference(model_provider, vdo_buf)) {
            // No power
            if (!vdo_stream_buffer_unref(image_provider->vdo_stream, &vdo_buf, &vdo_error)) {
                if (!vdo_error_is_expected(&vdo_error)) {
                    panic("%s: Unexpexted error: %s", __func__, vdo_error->message);
                }
                g_clear_error(&vdo_error);
            }
            img_provider_flush_all_frames(image_provider);
            continue;
        }
        gettimeofday(&end_ts, NULL);

        inference_ms = (unsigned int)(((end_ts.tv_sec - start_ts.tv_sec) * 1000) +
                                      ((end_ts.tv_usec - start_ts.tv_usec) / 1000));
        syslog(LOG_INFO, "Ran inference for %u ms", inference_ms);

        total_elapsed_ms = inference_ms;

        // Check if the framerate from vdo should be changed
        img_provider_update_framerate(image_provider, total_elapsed_ms);

        for (size_t i = 0; i < number_output_tensors; i++) {
            if (!model_get_tensor_output_info(model_provider, i, &tensor_outputs[i])) {
                panic("Failed to get output tensor info for %zu", i);
            }
        }

        if (strcmp(device_name, "ambarella-cvflow") != 0) {
            uint8_t* person_pred = (uint8_t*)tensor_outputs[0].data;
            uint8_t* car_pred    = (uint8_t*)tensor_outputs[1].data;
            syslog(LOG_INFO,
                   "Person detected: %.2f%% - Car detected: %.2f%%",
                   (float)*person_pred / 2.55f,
                   (float)*car_pred / 2.55f);
        } else {
            uint8_t* car_pred    = (uint8_t*)tensor_outputs[0].data;
            uint8_t* person_pred = (uint8_t*)tensor_outputs[1].data;

            float float_score_car    = *((float*)car_pred);
            float float_score_person = *((float*)person_pred);
            syslog(LOG_INFO,
                   "Person detected: %.2f%% - Car detected: %.2f%%",
                   float_score_person * 100,
                   float_score_car * 100);
        }

        // This will allow vdo to fill this buffer with data again
        if (!vdo_stream_buffer_unref(image_provider->vdo_stream, &vdo_buf, &vdo_error)) {
            if (!vdo_error_is_expected(&vdo_error)) {
                panic("%s: Unexpexted error: %s", __func__, vdo_error->message);
            }
            g_clear_error(&vdo_error);
        }
    }
end:
    if (image_provider) {
        img_provider_destroy(image_provider);
    }
    if (model_provider) {
        model_provider_destroy(model_provider);
    }
    free(tensor_outputs);

    syslog(LOG_INFO, "Exit %s", argv[0]);
    return 0;
}
