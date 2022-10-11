/**
 * Copyright (C) 2022, Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// define box struct
typedef struct {
    float y_min;
    float x_min;
    float y_max;
    float x_max;
    float score;
    int label;
} box;



/**
 * @brief convert output from model into detection boxes
 *
 * @param locations output from the model of size num_of_detections*4 containing the location of the boxes in the format [dy, dx, dh, dw]
 * @param classes output from the model of size num_of_detections*num_of_classes containing the confidence for each class
 * @param num_of_detections number of detections
 * @param anchors_file path to file containing anchors
 * @param num_of_classes number of classes
 * @param score_threshold minimum threshold for a box to be considered a detection
 * @param nms_threshold threshold for the iou non-maximum suppression
 * @param y_scale scale factor for the y coordinate
 * @param x_scale scale factor for the x coordinate
 * @param h_scale scale factor for the height
 * @param w_scale scale factor for the width
 * @param boxes output array of boxes
 */
int postProcessing(float* locations, float* classes,
                    int num_of_detections, char* anchor_file,
                    int num_of_classes, float score_threshold, float nms_threshold,
                    float y_scale, float x_scale, float h_scale, float w_scale,
                    box* boxes);
