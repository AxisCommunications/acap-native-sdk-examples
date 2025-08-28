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

/**
 * - object_detection_bbox_yolov5 -
 *
 * This application loads a larod YOLOv5 model which takes an image as input. The output is
 * YOLOv5-specifically parsed to retrieve values corresponding to the class, score and location of
 * detected objects in the image.
 *
 * The application expects two arguments on the command line in the
 * following order: MODELFILE LABELSFILE.
 *
 * First argument, MODELFILE, is a string describing path to the model.
 *
 * Second argument, LABELSFILE, is a string describing path to the label txt.
 *
 */

#include "argparse.h"
#include "imgprovider.h"
#include "labelparse.h"
#include "model.h"
#include "model_params.h"  //Generated at build time
#include "panic.h"
#include "vdo-error.h"
#include "vdo-frame.h"
#include "vdo-types.h"
#include <axsdk/axparameter.h>
#include <bbox.h>

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <syslog.h>

#define APP_NAME "object_detection_yolov5"

volatile sig_atomic_t running = 1;

static void shutdown(int status) {
    (void)status;
    running = 0;
}

typedef struct model_params {
    int input_width;
    int input_height;
    float quantization_scale;
    float quantization_zero_point;
    int num_classes;
    int num_detections;
    int size_per_detection;
} model_params_t;

static int ax_parameter_get_int(AXParameter* handle, const char* name) {
    gchar* str_value = NULL;
    GError* error    = NULL;
    int value;

    // Get the value of the parameter
    if (!ax_parameter_get(handle, name, &str_value, &error)) {
        panic("%s", error->message);
    }

    // Convert the parameter value to int
    if (sscanf(str_value, "%d", &value) != 1) {
        panic("Axparameter %s was not an int", name);
    }

    syslog(LOG_INFO, "Axparameter %s: %s", name, str_value);

    g_free(str_value);

    return value;
}

static bbox_t* setup_bbox(void) {
    // Create box drawers
    bbox_t* bbox = bbox_view_new(1u);
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

static unsigned int elapsed_ms(struct timeval* start_ts, struct timeval* end_ts) {
    return (unsigned int)(((end_ts->tv_sec - start_ts->tv_sec) * 1000) +
                          ((end_ts->tv_usec - start_ts->tv_usec) / 1000));
}

static float intersection_over_union(float x1,
                                     float y1,
                                     float w1,
                                     float h1,
                                     float x2,
                                     float y2,
                                     float w2,
                                     float h2) {
    float xx1 = fmax(x1 - (w1 / 2), x2 - (w2 / 2));
    float yy1 = fmax(y1 - (h1 / 2), y2 - (h2 / 2));
    float xx2 = fmin(x1 + (w1 / 2), x2 + (w2 / 2));
    float yy2 = fmin(y1 + (h1 / 2), y2 + (h2 / 2));

    float inter_area = fmax(0, xx2 - xx1) * fmax(0, yy2 - yy1);
    float union_area = w1 * h1 + w2 * h2 - inter_area;

    return inter_area / union_area;
}

static void non_maximum_suppression(uint8_t* tensor,
                                    float iou_threshold,
                                    model_params_t* model_params,
                                    int* invalid_detections) {
    int size_per_detection = model_params->size_per_detection;
    int num_detections     = model_params->num_detections;
    float qt_zero_point    = model_params->quantization_zero_point;
    float qt_scale         = model_params->quantization_scale;

    for (int i = 0; i < num_detections; i++) {
        if (invalid_detections[i])  // Skip comparison if detection is already invalid
            continue;

        float x1                 = (tensor[size_per_detection * i + 0] - qt_zero_point) * qt_scale;
        float y1                 = (tensor[size_per_detection * i + 1] - qt_zero_point) * qt_scale;
        float w1                 = (tensor[size_per_detection * i + 2] - qt_zero_point) * qt_scale;
        float h1                 = (tensor[size_per_detection * i + 3] - qt_zero_point) * qt_scale;
        float object1_likelihood = (tensor[size_per_detection * i + 4] - qt_zero_point) * qt_scale;

        for (int j = i + 1; j < num_detections; j++) {
            if (invalid_detections[j])  // Skip comparison if detection is already invalid
                continue;

            float x2 = (tensor[size_per_detection * j + 0] - qt_zero_point) * qt_scale;
            float y2 = (tensor[size_per_detection * j + 1] - qt_zero_point) * qt_scale;
            float w2 = (tensor[size_per_detection * j + 2] - qt_zero_point) * qt_scale;
            float h2 = (tensor[size_per_detection * j + 3] - qt_zero_point) * qt_scale;
            float object2_likelihood =
                (tensor[size_per_detection * j + 4] - qt_zero_point) * qt_scale;

            if (intersection_over_union(x1, y1, w1, h1, x2, y2, w2, h2) > iou_threshold) {
                // invalidates the detection with lowest object likelihood score
                if (object1_likelihood > object2_likelihood) {
                    invalid_detections[j] = 1;
                } else {
                    invalid_detections[i] = 1;
                    break;
                }
            }
        }
    }
}

static void filter_detections(uint8_t* tensor,
                              float conf_theshold,
                              float iou_threshold,
                              model_params_t* model_params,
                              int* invalid_detections) {
    // Filter boxes by confidence
    for (int i = 0; i < model_params->num_detections; i++) {
        float object_likelihood = (tensor[model_params->size_per_detection * i + 4] -
                                   model_params->quantization_zero_point) *
                                  model_params->quantization_scale;

        if (object_likelihood < conf_theshold) {
            invalid_detections[i] = 1;
        } else {
            invalid_detections[i] = 0;
        }
    }

    non_maximum_suppression(tensor, iou_threshold, model_params, invalid_detections);
}

static void determine_class_and_object_likelihood(uint8_t* tensor,
                                                  int detection_idx,
                                                  int size_per_detection,
                                                  float qt_zero_point,
                                                  float qt_scale,
                                                  float* highest_class_likelihood,
                                                  int* label_idx,
                                                  float* object_likelihood) {
    // Find what class this object is
    for (int j = 5; j < size_per_detection; j++) {
        float class_likelihood =
            (tensor[size_per_detection * detection_idx + j] - qt_zero_point) * qt_scale;
        if (class_likelihood > *highest_class_likelihood) {
            *highest_class_likelihood = class_likelihood;
            *label_idx                = j - 5;
        }
    }

    *object_likelihood =
        (tensor[size_per_detection * detection_idx + 4] - qt_zero_point) * qt_scale;
}

static void
find_corners(float x, float y, float w, float h, float* x1, float* y1, float* x2, float* y2) {
    *x1 = fmax(0.0, x - (w / 2));
    *y1 = fmax(0.0, y - (h / 2));
    *x2 = fmin(1.0, x + (w / 2));
    *y2 = fmin(1.0, y + (h / 2));
}

static void determine_bbox_coordinates(uint8_t* tensor,
                                       int detection_idx,
                                       int size_per_detection,
                                       float qt_zero_point,
                                       float qt_scale,
                                       float* x1,
                                       float* y1,
                                       float* x2,
                                       float* y2) {
    // Get coordinates for the object
    float x = (tensor[size_per_detection * detection_idx + 0] - qt_zero_point) * qt_scale;
    float y = (tensor[size_per_detection * detection_idx + 1] - qt_zero_point) * qt_scale;
    float w = (tensor[size_per_detection * detection_idx + 2] - qt_zero_point) * qt_scale;
    float h = (tensor[size_per_detection * detection_idx + 3] - qt_zero_point) * qt_scale;
    find_corners(x, y, w, h, x1, y1, x2, y2);
}

int main(int argc, char** argv) {
    g_autoptr(GError) vdo_error           = NULL;
    img_provider_t* image_provider        = NULL;
    model_provider_t* model_provider      = NULL;
    model_tensor_output_t* tensor_outputs = NULL;
    bbox_t* bbox                          = NULL;

    openlog(APP_NAME, LOG_PID | LOG_CONS, LOG_USER);

    // Stop main loop at signal
    signal(SIGTERM, shutdown);
    signal(SIGINT, shutdown);

    args_t args;
    parse_args(argc, argv, &args);

    model_params_t* model_params = (model_params_t*)malloc(sizeof(model_params_t));
    if (model_params == NULL) {
        panic("%s: Unable to allocate model_params_t: %s", __func__, strerror(errno));
    }

    // Comes from model_params.h
    model_params->input_width             = MODEL_INPUT_WIDTH;
    model_params->input_height            = MODEL_INPUT_HEIGHT;
    model_params->quantization_scale      = QUANTIZATION_SCALE;
    model_params->quantization_zero_point = QUANTIZATION_ZERO_POINT;
    model_params->num_classes             = NUM_CLASSES;
    model_params->num_detections          = NUM_DETECTIONS;
    model_params->size_per_detection =
        5 + NUM_CLASSES;  // Each detection consists of [x, y, w, h, object_likelihood,
                          // class1_likelihood, class2_likelihood, class3_likelihood, ... ]

    syslog(LOG_INFO,
           "Model input size w/h: %d x %d",
           model_params->input_width,
           model_params->input_height);
    syslog(LOG_INFO, "Quantization scale: %f", model_params->quantization_scale);
    syslog(LOG_INFO, "Quantization zero point: %f", model_params->quantization_zero_point);
    syslog(LOG_INFO, "Number of classes: %d", model_params->num_classes);
    syslog(LOG_INFO, "Number of detections: %d", model_params->num_detections);

    int invalid_detections[model_params->num_detections];

    // Create a new axparameter instance
    GError* axparameter_error       = NULL;
    AXParameter* axparameter_handle = ax_parameter_new(APP_NAME, &axparameter_error);
    if (axparameter_handle == NULL) {
        panic("%s", axparameter_error->message);
    }

    float conf_threshold = ax_parameter_get_int(axparameter_handle, "ConfThresholdPercent") / 100.0;
    float iou_threshold  = ax_parameter_get_int(axparameter_handle, "IouThresholdPercent") / 100.0;

    ax_parameter_free(axparameter_handle);

    VdoFormat vdo_format = VDO_FORMAT_YUV;
    double vdo_framerate = 30.0;

    if (!g_strcmp0(args.device_name, "a9-dlpu-tflite")) {
        // Possible to run RGB on ARTPEC-9
        vdo_format = VDO_FORMAT_RGB;
    }

    // Choose a valid stream resolution since only certain resolutions are allowed
    unsigned int stream_width  = 0;
    unsigned int stream_height = 0;
    if (!choose_stream_resolution(model_params->input_width,
                                  model_params->input_height,
                                  vdo_format,
                                  "native",
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

    image_provider = create_img_provider(stream_width, stream_height, 2, vdo_format, vdo_framerate);
    if (!image_provider) {
        panic("%s: Could not create image provider", __func__);
    }

    size_t number_output_tensors = 0;
    model_provider               = create_model_provider(model_params->input_width,
                                           model_params->input_height,
                                           image_provider->width,
                                           image_provider->height,
                                           image_provider->pitch,
                                           image_provider->format,
                                           VDO_FORMAT_RGB,
                                           args.model_file,
                                           args.device_name,
                                           false,
                                           &number_output_tensors);
    if (!model_provider) {
        panic("%s: Could not create model provider", __func__);
    }
    tensor_outputs = calloc(number_output_tensors, sizeof(model_tensor_output_t));
    if (!tensor_outputs) {
        panic("%s: Could not allocate tensor outputs", __func__);
    }

    char** labels = NULL;          // This is the array of label strings. The label
                                   // entries points into the large label_file_data buffer.
    size_t num_labels;             // Number of entries in the labels array.
    char* label_file_data = NULL;  // Buffer holding the complete collection of label strings.

    parse_labels(&labels, &label_file_data, args.labels_file, &num_labels);

    syslog(LOG_INFO, "Start fetching video frames from VDO");
    if (!img_provider_start(image_provider)) {
        panic("%s: Could not start image provider", __func__);
    }

    bbox = setup_bbox();

    int size_per_detection = model_params->size_per_detection;
    float qt_zero_point    = model_params->quantization_zero_point;
    float qt_scale         = model_params->quantization_scale;

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
        // If needed convert and scale/crop to correct input format and resolution
        // Its up to the model provider to decide if needed or not
        // If not needed the model_run_preprocessing will return true without
        // any work
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

        // Retrieve detections from data
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

        uint8_t* tensor_data = tensor_outputs[0].data;
        // Parse the output
        gettimeofday(&start_ts, NULL);
        filter_detections(tensor_data,
                          conf_threshold,
                          iou_threshold,
                          model_params,
                          invalid_detections);
        gettimeofday(&end_ts, NULL);
        syslog(LOG_INFO, "Ran parsing for %u ms", elapsed_ms(&start_ts, &end_ts));

        bbox_clear(bbox);

        int valid_detection_count = 0;

        for (int i = 0; i < model_params->num_detections; i++) {
            if (invalid_detections[i] == 1) {
                continue;
            }

            valid_detection_count++;

            float highest_class_likelihood = 0.0;
            int label_idx                  = 0;
            float object_likelihood        = 0.0;

            determine_class_and_object_likelihood(tensor_data,
                                                  i,
                                                  size_per_detection,
                                                  qt_zero_point,
                                                  qt_scale,
                                                  &highest_class_likelihood,
                                                  &label_idx,
                                                  &object_likelihood);
            // Log info about object
            syslog(LOG_INFO,
                   "Object %d: Label=%s, Object Likelihood=%.2f, Class Likelihood=%.2f, ",
                   valid_detection_count,
                   labels[label_idx],
                   object_likelihood,
                   highest_class_likelihood);

            float x1, y1, x2, y2;
            determine_bbox_coordinates(tensor_data,
                                       i,
                                       size_per_detection,
                                       qt_zero_point,
                                       qt_scale,
                                       &x1,
                                       &y1,
                                       &x2,
                                       &y2);
            syslog(LOG_INFO, "Bounding Box: [%.2f, %.2f, %.2f, %.2f]", x1, y1, x2, y2);

            // No need to compensate for rotation since bbox will handle this
            bbox_coordinates_frame_normalized(bbox);
            bbox_rectangle(bbox, x1, y1, x2, y2);
        }

        if (!bbox_commit(bbox, 0u)) {
            panic("Failed to commit box drawer");
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
    // Cleanup
    free(model_params);
    if (image_provider) {
        destroy_img_provider(image_provider);
    }
    if (model_provider) {
        destroy_model_provider(model_provider);
    }
    free(tensor_outputs);
    free(labels);
    free(label_file_data);
    bbox_destroy(bbox);

    syslog(LOG_INFO, "Exit %s", argv[0]);

    return 0;
}
