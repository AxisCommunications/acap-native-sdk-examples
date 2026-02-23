/**
 * Copyright (C) 2021 Axis Communications AB, Lund, Sweden
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
 * - object_detection -
 *
 * This application loads a larod model which takes an image as input and
 * outputs values corresponding to the class, score and location of detected
 * objects in the image.
 *
 * The application expects at least one argument on the command line in the
 * following order: MODEL.
 *
 * If THRESHOLD and LABELSFILE is supplied postprocessing will be used.
 *
 * First argument, MODEL, is a string describing path to the model.
 *
 * Second argument, THRESHOLD is an integer ranging from 0 to 100 to select good detections.
 *
 * Third argument, LABELSFILE, is a string describing path to the label txt.
 *
 * FOURTH argument, DEVICE, is a string for which larod device to use.
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "argparse.h"
#include "channel_util.h"
#include "img_util.h"
#include "labelparse.h"
#include "model.h"
#include "panic.h"
#include "vdo-error.h"
#include "vdo-frame.h"
#include "vdo-types.h"
#include <bbox.h>

#include <math.h>
#include <poll.h>
#include <unistd.h>

volatile sig_atomic_t running = 1;

// define box struct
typedef struct {
    float y_min;
    float x_min;
    float y_max;
    float x_max;
    float score;
    int label;
} box;

static void shutdown(int status) {
    (void)status;
    running = 0;
}

static int handle_vdo_failed(GError* error) {
    // Maintenance/Installation in progress (e.g. Global-Rotation)
    if (vdo_error_is_expected(&error)) {
        syslog(LOG_INFO, "Expected vdo error %s", error->message);
        return EXIT_SUCCESS;
    } else {
        panic("Unexpected vdo error %s", error->message);
    }
}

static VdoStream* create_new_vdo_stream(unsigned int channel,
                                        VdoFormat format,
                                        VdoResolution res,
                                        unsigned int num_buffers,
                                        const char* image_fit,
                                        double framerate) {
    g_autoptr(VdoMap) vdo_settings = vdo_map_new();
    g_autoptr(GError) error        = NULL;

    if (!vdo_settings) {
        panic("%s: Failed to create vdo_map", __func__);
    }

    vdo_map_set_uint32(vdo_settings, "channel", channel);
    // format is the image format that is supplied from vdo
    vdo_map_set_uint32(vdo_settings, "format", format);
    // Set initial framerate
    vdo_map_set_double(vdo_settings, "framerate", framerate);
    VdoPair32u resolution = {
        .w = res.width,
        .h = res.height,
    };
    vdo_map_set_pair32u(vdo_settings, "resolution", resolution);
    // Make it possible to change the framerate for the stream after it is started
    vdo_map_set_boolean(vdo_settings, "dynamic.framerate", true);
    // It is not needed to set buffer.strategy since VDO_BUFFER_STRATEGY_INFINITE is default
    // vdo_map_set_uint32(vdo_settings, "buffer.strategy", VDO_BUFFER_STRATEGY_INFINITE);

    // The number of buffers that vdo will allocate for this stream
    // Normally two buffers are enough and using too many buffers will use
    // more memory in the product
    vdo_map_set_uint32(vdo_settings, "buffer.count", num_buffers);
    // The vdo_stream_get_buffer is non blocking and will return immediately
    // Then we need to poll instead when it is ok to get a buffer
    vdo_map_set_boolean(vdo_settings, "socket.blocking", false);
    vdo_map_set_string(vdo_settings, "image.fit", image_fit);

    // Create a vdo stream using the vdoMap filled in above
    g_autoptr(VdoStream) vdo_stream = vdo_stream_new(vdo_settings, NULL, &error);
    if (!vdo_stream) {
        panic("%s: Failed creating vdo stream: %s", __func__, error->message);
    }
    syslog(LOG_INFO, "Dump of vdo stream settings map =====");
    vdo_map_dump(vdo_settings);

    return g_steal_pointer(&vdo_stream);
}

static bbox_t* setup_bbox(uint32_t channel) {
    // Create box drawer for channel
    bbox_t* bbox = bbox_view_new(channel);
    if (!bbox) {
        panic("Failed to create box drawer");
    }

    bbox_clear(bbox);
    const bbox_color_t red = bbox_color_from_rgb(0xff, 0x00, 0x00);

    bbox_style_outline(bbox);   // Switch to outline style
    bbox_thickness_thin(bbox);  // Switch to thin lines
    bbox_color(bbox, red);      // Switch to red

    return bbox;
}

static bool parse_and_postprocess_output_tensors(bbox_t* bbox,
                                                 model_tensor_output_t* tensor_outputs,
                                                 float confidence_threshold,
                                                 char** labels,
                                                 unsigned int* post_processing_ms) {
    box* boxes = NULL;
    struct timeval start_ts, end_ts;

    // From here this is different dependent on model
    float* locations = (float*)tensor_outputs[0].data;
    float* classes   = (float*)tensor_outputs[1].data;

    bbox_clear(bbox);

    gettimeofday(&start_ts, NULL);

    float* scores            = (float*)tensor_outputs[2].data;
    float* nbr_detections    = (float*)tensor_outputs[3].data;
    int number_of_detections = (int)nbr_detections[0];
    if (number_of_detections == 0) {
        syslog(LOG_INFO, "No object is detected");
        return true;
    }
    boxes = (box*)malloc(sizeof(box) * number_of_detections);
    for (int i = 0; i < number_of_detections; i++) {
        boxes[i].y_min = locations[4 * i];
        boxes[i].x_min = locations[4 * i + 1];
        boxes[i].y_max = locations[4 * i + 2];
        boxes[i].x_max = locations[4 * i + 3];
        boxes[i].score = scores[i];
        boxes[i].label = classes[i];
    }
    gettimeofday(&end_ts, NULL);

    *post_processing_ms = (unsigned int)(((end_ts.tv_sec - start_ts.tv_sec) * 1000) +
                                         ((end_ts.tv_usec - start_ts.tv_usec) / 1000));
    if (*post_processing_ms != 0) {
        syslog(LOG_INFO, "Postprocessing in %u ms", *post_processing_ms);
    }

    for (int i = 0; i < number_of_detections; i++) {
        if (boxes[i].score >= confidence_threshold) {
            float top    = boxes[i].y_min;
            float bottom = boxes[i].y_max;
            float right  = boxes[i].x_max;
            float left   = boxes[i].x_min;

            syslog(LOG_INFO,
                   "Object %d: Classes: %s - Scores: %f - Locations: [%f,%f,%f,%f]",
                   i,
                   labels[boxes[i].label],
                   boxes[i].score,
                   left,
                   top,
                   right,
                   bottom);
            bbox_coordinates_frame_normalized(bbox);
            bbox_rectangle(bbox, left, top, right, bottom);
        }
    }

    if (!bbox_commit(bbox, 0u)) {
        panic("Failed to commit box drawer");
    }
    if (boxes) {
        free(boxes);
    }
    return true;
}

/**
 * @brief Main function that starts a stream with different options.
 */
int main(int argc, char** argv) {
    bbox_t* bbox                          = NULL;
    g_autoptr(GError) vdo_error           = NULL;
    model_provider_t* model_provider      = NULL;
    model_tensor_output_t* tensor_outputs = NULL;
    img_info_t model_metadata             = {0};
    img_framerate_t image_framerate       = {0};
    g_autoptr(VdoStream) vdo_stream       = NULL;
    g_autoptr(VdoMap) vdo_stream_info     = NULL;

    // Stop main loop at signal
    signal(SIGTERM, shutdown);
    signal(SIGINT, shutdown);

    args_t args;
    parse_args(argc, argv, &args);

    char* device_name        = args.device_name;
    char* model_file         = args.model_file;
    const char* labels_file  = args.labels_file;
    const int threshold      = args.threshold;
    size_t number_of_classes = 0;
    bool parse_tensors       = true;

    // Start by loading the model and get the model metadata
    size_t number_output_tensors = 0;
    model_provider =
        model_provider_new(model_file, device_name, labels_file, &number_output_tensors);
    if (!model_provider) {
        panic("%s: Could not create model provider", __func__);
    }

    tensor_outputs = calloc(number_output_tensors, sizeof(model_tensor_output_t));
    if (!tensor_outputs) {
        panic("%s: Could not allocate tensor outputs", __func__);
    }

    // Get the model format and model input dimension and pitches
    model_metadata = model_provider_get_model_metadata(model_provider);

    // Set to a framerate that is sufficient for inference
    double vdo_stream_framerate = 30.0;
    // The vdo channel to be used
    // When using VAPIX and rtsp the camera parameter normally corresponds
    // to the channel number here.
    unsigned int vdo_channel = 1;

    // The buffer count will affect memory consumption so keep it as low
    // as possible
    unsigned int vdo_stream_buffer_count = 2;

    // Set to false if e.g a view area is wanted instead of the whole sensor
    bool fetch_from_whole_sensor = true;

    if (fetch_from_whole_sensor) {
        // Currently only take from the first input channel
        vdo_channel = channel_util_get_first_input_channel();
    }

    // Get the current global rotation and print
    uint32_t rotation = channel_util_get_image_rotation(vdo_channel);
    syslog(LOG_INFO, "[Channel %u] Current global rotation is %u", vdo_channel, rotation);
    VdoPair32u channel_ar = channel_util_get_aspect_ratio(vdo_channel);
    syslog(LOG_INFO,
           "[Channel %u] Current aspect ratio is %u:%u",
           vdo_channel,
           channel_ar.w,
           channel_ar.h);

    VdoResolution req_res    = {model_metadata.width, model_metadata.height};
    VdoResolution chosen_req = req_res;

    // Get the a resolution with the same aspect ratio as the channel aspect ratio
    if (!channel_util_choose_stream_resolution(vdo_channel,
                                               req_res,
                                               &chosen_req,
                                               rotation,
                                               &model_metadata.format)) {
        panic("%s: Could not chose a resolution", __func__);
    }
    // In this case use default image.fit crop because the resolution
    // fetched from choose function have the same aspect ratio as the
    // channel aspect ratio
    vdo_stream = create_new_vdo_stream(vdo_channel,
                                       model_metadata.format,
                                       chosen_req,
                                       vdo_stream_buffer_count,
                                       "crop",
                                       vdo_stream_framerate);
    if (!vdo_stream) {
        return handle_vdo_failed(vdo_error);
    }
    vdo_stream_info = vdo_stream_get_info(vdo_stream, &vdo_error);
    if (!vdo_stream_info) {
        return handle_vdo_failed(vdo_error);
    }
    VdoPair32u aspect_ratio_def = {.w = 0u, .h = 0u};
    VdoPair32u stream_ar = vdo_map_get_pair32u(vdo_stream_info, "aspect_ratio", aspect_ratio_def);
    syslog(LOG_INFO, "Stream aspect ratio is %u:%u", stream_ar.w, stream_ar.h);

    image_framerate.wanted_framerate = vdo_stream_framerate;
    double info_framerate = vdo_map_get_double(vdo_stream_info, "framerate", vdo_stream_framerate);
    image_framerate.frametime = (unsigned int)((1.0 / info_framerate) * 1000.0);

    int fd = vdo_stream_get_fd(vdo_stream, &vdo_error);
    if (fd < 0) {
        return handle_vdo_failed(vdo_error);
    }
    struct pollfd fds = {
        .fd     = fd,
        .events = POLL_IN,
    };

    if (labels_file == NULL) {
        parse_tensors = false;
    }

    char** labels = NULL;          // This is the array of label strings. The label
                                   // entries points into the large label_file_data buffer.
    char* label_file_data = NULL;  // Buffer holding the complete collection of label strings.

    if (parse_tensors) {
        parse_labels(&labels, &label_file_data, labels_file, &number_of_classes);
        bbox = setup_bbox(vdo_channel);
    }

    if (!vdo_stream_start(vdo_stream, &vdo_error)) {
        return handle_vdo_failed(vdo_error);
    }
    syslog(LOG_INFO, "Start fetching video frames from VDO");

    // Use the vdo info map to update the model metadata
    model_provider_update_image_metadata(model_provider, vdo_stream_info);

    while (running) {
        struct timeval start_ts, end_ts;
        unsigned int inference_ms     = 0;
        unsigned int total_elapsed_ms = 0;

        int status = 0;
        do {
            // If poll returns -1 then errno is set
            // if the errno is set to EINTR then just
            // continue this loop
            status = poll(&fds, 1, -1);
        } while (status == -1 && errno == EINTR);

        if (status < 0) {
            panic("Failed to poll with status %d", status);
        }

        g_autoptr(VdoBuffer) vdo_buf = vdo_stream_get_buffer(vdo_stream, &vdo_error);
        if (!vdo_buf && g_error_matches(vdo_error, VDO_ERROR, VDO_ERROR_NO_DATA)) {
            g_clear_error(&vdo_error);
            continue;
        }
        if (!vdo_buf) {
            return handle_vdo_failed(vdo_error);
        }
        gettimeofday(&start_ts, NULL);
        // Run inference and preprocessing if needed
        if (!model_run_inference(model_provider, vdo_buf)) {
            if (!img_util_flush(vdo_stream, &vdo_buf, &vdo_error)) {
                return handle_vdo_failed(vdo_error);
            }
            continue;
        }
        gettimeofday(&end_ts, NULL);
        inference_ms = (unsigned int)(((end_ts.tv_sec - start_ts.tv_sec) * 1000) +
                                      ((end_ts.tv_usec - start_ts.tv_usec) / 1000));
        syslog(LOG_INFO, "Ran inference for %u ms", inference_ms);

        for (size_t i = 0; i < number_output_tensors; i++) {
            if (!model_get_tensor_output_info(model_provider, i, &tensor_outputs[i])) {
                panic("Failed to get output tensor info for %zu", i);
            }
        }
        total_elapsed_ms = inference_ms;

        if (parse_tensors) {
            unsigned int post_processing_ms = 0;
            float confidence_threshold      = (float)(threshold / 100.0);
            parse_and_postprocess_output_tensors(bbox,
                                                 tensor_outputs,
                                                 confidence_threshold,
                                                 labels,
                                                 &post_processing_ms);
            total_elapsed_ms += post_processing_ms;
        }

        // Check if the framerate from vdo should be changed
        if (img_util_update_framerate(vdo_stream, &image_framerate, total_elapsed_ms)) {
            if (!img_util_flush(vdo_stream, &vdo_buf, &vdo_error)) {
                return handle_vdo_failed(vdo_error);
            }
        } else {
            // This will allow vdo to fill this buffer with data again
            if (!vdo_stream_buffer_unref(vdo_stream, &vdo_buf, &vdo_error)) {
                if (!vdo_error_is_expected(&vdo_error)) {
                    panic("%s: Unexpected error: %s", __func__, vdo_error->message);
                }
                g_clear_error(&vdo_error);
            }
        }
    }

    if (model_provider) {
        model_provider_destroy(model_provider);
    }
    free(tensor_outputs);

    if (labels) {
        free(labels);
    }
    if (label_file_data) {
        free(label_file_data);
    }
    if (parse_tensors) {
        bbox_destroy(bbox);
    }

    syslog(LOG_INFO, "Exit %s", argv[0]);
    return 0;
}
