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
 * following order: DEVICENAME MODEL WIDTH HEIGHT.
 *
 * First argument, DEVICENAME, is a string that is the larod device name
 *
 * Second argument, MODEL, is a string describing path to the model.
 *
 * Third argument, WIDTH, is an unsigned integer for the input width.
 *
 * Fourth argument, HEIGHT, is an unsigned integer for the input height.
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
    const unsigned int input_width        = atoi(argv[3]);
    const unsigned int input_height       = atoi(argv[4]);
    g_autoptr(GError) vdo_error           = NULL;
    img_provider_t* image_provider        = NULL;
    model_provider_t* model_provider      = NULL;
    model_tensor_output_t* tensor_outputs = NULL;

    // Stop main loop at signal
    signal(SIGTERM, shutdown);
    signal(SIGINT, shutdown);

    syslog(LOG_INFO, "Starting %s", argv[0]);

    if (argc != 5) {
        syslog(LOG_ERR,
               "Invalid number of arguments. Required arguments are: "
               "DEVICENAME MODEL_PATH WIDTH HEIGHT");
        goto end;
    }

    VdoFormat image_format = VDO_FORMAT_YUV;
    VdoFormat model_format = VDO_FORMAT_RGB;
    double vdo_framerate   = 30.0;

    // Choose a valid stream resolution since only certain resolutions are allowed
    unsigned int stream_width  = 0;
    unsigned int stream_height = 0;
    if (!choose_stream_resolution(input_width,
                                  input_height,
                                  image_format,
                                  NULL,
                                  "all",
                                  &stream_width,
                                  &stream_height)) {
        syslog(LOG_ERR, "%s: Failed choosing stream resolution", __func__);
        goto end;
    }

    syslog(LOG_INFO,
           "Creating VDO image provider and creating stream %u x %u",
           stream_width,
           stream_height);

    if (strcmp(device_name, "ambarella-cvflow") == 0) {
        model_format = VDO_FORMAT_PLANAR_RGB;
    }

    image_provider =
        create_img_provider(stream_width, stream_height, 2, image_format, vdo_framerate);
    if (!image_provider) {
        panic("%s: Could not create image provider", __func__);
    }

    size_t number_output_tensors = 0;
    model_provider               = create_model_provider(input_width,
                                           input_height,
                                           image_provider->width,
                                           image_provider->height,
                                           image_provider->pitch,
                                           image_format,
                                           model_format,
                                           model_file,
                                           device_name,
                                           true,
                                           &number_output_tensors);
    if (!model_provider) {
        panic("%s: Could not create model provider", __func__);
    }
    tensor_outputs = calloc(number_output_tensors, sizeof(model_tensor_output_t));
    if (!tensor_outputs) {
        panic("%s: Could not allocate tensor outputs", __func__);
    }

    syslog(LOG_INFO, "Start fetching video frames from VDO");
    if (!img_provider_start(image_provider)) {
        panic("%s: Could not start image provider", __func__);
    }

    while (running) {
        struct timeval start_ts, end_ts;
        unsigned int preprocessing_ms = 0;
        unsigned int inference_ms     = 0;
        unsigned int total_elapsed_ms = 0;

        g_autoptr(VdoBuffer) vdo_buf = img_provider_get_frame(image_provider);
        if (!vdo_buf) {
            // This can only happen if it is global rotation then
            // the stream has to be restarted because rotation has been changed.
            syslog(
                LOG_INFO,
                "No buffer because of changed global rotation. Application needs to be restarted");
            goto end;
        }
        // Convert data to the correct format
        gettimeofday(&start_ts, NULL);
        if (!model_run_preprocessing(model_provider, vdo_buf)) {
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

        preprocessing_ms = (unsigned int)(((end_ts.tv_sec - start_ts.tv_sec) * 1000) +
                                          ((end_ts.tv_usec - start_ts.tv_usec) / 1000));
        syslog(LOG_INFO, "Ran pre-processing for %u ms", preprocessing_ms);

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

        total_elapsed_ms = inference_ms + preprocessing_ms;

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
        destroy_img_provider(image_provider);
    }
    if (model_provider) {
        destroy_model_provider(model_provider);
    }
    free(tensor_outputs);

    syslog(LOG_INFO, "Exit %s", argv[0]);
    return 0;
}
