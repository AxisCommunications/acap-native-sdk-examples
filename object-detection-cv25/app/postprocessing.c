/**
 * Copyright (C) 2022, Axis Communications AB, Lund, Sweden
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


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <syslog.h>
#include "postprocessing.h"

//structure detection
typedef struct {
    float dy;
    float dx;
    float dh;
    float dw;
    float score;
    int label;
    float anchor_ymin;
    float anchor_xmin;
    float anchor_ymax;
    float anchor_xmax;
} detection;


/*
* This function loads the data structure reading the anchors from file, and the detections from parameter.
* It expects detections in he format [dy,dx,dh,dw] and the anchors in the format [xmin,ymin,xmax,ymax]
*
*/
int loadDetectionStruct(const float *locations, float* classes, int num_of_detections, int num_of_classes, const char * anchor_file, detection* dets) {

    //Open anchor file
    FILE *fp;
    fp = fopen(anchor_file, "rb");
    if (fp == NULL){
        printf("Error opening anchor file");
        return 1;
    }


    //Load detections and anchors in struct array
    for(int i = 0; i < num_of_detections; i++){
        dets[i].dy = locations[i*4];
        dets[i].dx = locations[i*4+1];
        dets[i].dh = locations[i*4+2];
        dets[i].dw = locations[i*4+3];
        dets[i].score = 0;
        for (int j = 0; j < num_of_classes; j++){
            if (classes[i*num_of_classes+j] > dets[i].score){
                dets[i].score = classes[i*num_of_classes+j];
                dets[i].label = j;
            }
        }
        if(fread(&dets[i].anchor_xmin, sizeof(float), 1, fp)!=1){
            syslog(LOG_ERR, "Error when reading anchor file");
            return 1;
        }
        if(fread(&dets[i].anchor_ymin, sizeof(float), 1, fp)!=1){
            syslog(LOG_ERR, "Error when reading anchor file");
            return 1;
        }
        if(fread(&dets[i].anchor_xmax, sizeof(float), 1, fp)!=1){
            syslog(LOG_ERR, "Error when reading anchor file");
            return 1;
        }
        if(fread(&dets[i].anchor_ymax, sizeof(float), 1, fp)!=1){
            syslog(LOG_ERR, "Error when reading anchor file");
            return 1;
        }

    }

    //Close anchor file
    fclose(fp);
    return 0;
}
float max(float a, float b){
    if (a > b) return a;
    return b;
}
float min(float a, float b){
    if (a < b) return a;
    return b;
}


//Apply anchors to detections to obtain boxes
void applyAnchors(detection* dets, int num_of_detections, box* boxes, float y_scale, float x_scale, float h_scale, float w_scale){
    float prior_center_y, prior_center_x, prior_height, prior_width;
    float center_y, center_x, height, width;
    for(int i = 0; i < num_of_detections; i++){
        prior_center_x = (dets[i].anchor_xmin + dets[i].anchor_xmax) / 2.0;
        prior_center_y = (dets[i].anchor_ymin + dets[i].anchor_ymax) / 2.0;
        prior_width = dets[i].anchor_xmax - dets[i].anchor_xmin;
        prior_height = dets[i].anchor_ymax - dets[i].anchor_ymin;

        center_x = dets[i].dx * prior_width / x_scale + prior_center_x;
        center_y = dets[i].dy * prior_height / y_scale + prior_center_y;
        width = exp(dets[i].dw / w_scale) * prior_width;
        height = exp(dets[i].dh / h_scale) * prior_height;

        boxes[i].x_min = center_x - width / 2.0;
        boxes[i].y_min = center_y - height / 2.0;
        boxes[i].x_max = center_x + width / 2.0;
        boxes[i].y_max = center_y + height / 2.0;
        boxes[i].score = dets[i].score;
        boxes[i].label = dets[i].label;

        //Limit boxes from 0 to 1
        boxes[i].x_min = max(0, boxes[i].x_min);
        boxes[i].y_min = max(0, boxes[i].y_min);
        boxes[i].x_max = min(1, boxes[i].x_max);
        boxes[i].y_max = min(1, boxes[i].y_max);

    }
}

//Suppress low score boxes
void suppressLowScoreBoxes(box* boxes, int num_of_detections, float score_threshold){
    for(int i = 0; i < num_of_detections; i++){
        if (boxes[i].score < score_threshold){
            boxes[i].score = 0;
        }
    }
}

//sort boxes by score
void sortBoxes(box* boxes, int num_of_detections){
    box temp;
    for(int i = 0; i < num_of_detections; i++){
        for(int j = i+1; j < num_of_detections; j++){
            if (boxes[i].score < boxes[j].score){
                temp = boxes[i];
                boxes[i] = boxes[j];
                boxes[j] = temp;
            }
        }
    }
}

//Sort boxes by score efficiently
void sortBoxesEfficient(box* boxes, int num_of_detections){
    box temp;
    int i = 0;
    int j = num_of_detections-1;
    float pivot = boxes[num_of_detections/2].score;
    while (i <= j){
        while (boxes[i].score > pivot) i++;
        while (boxes[j].score < pivot) j--;
        if (i <= j){
            temp = boxes[i];
            boxes[i] = boxes[j];
            boxes[j] = temp;
            i++;
            j--;
        }
    }
    if (j > 0) sortBoxesEfficient(boxes, j+1);
    if (i < num_of_detections-1) sortBoxesEfficient(boxes+i, num_of_detections-i);
}


//Calculate IOU
float calculateIOU(box box1, box box2){
    float intersection_xmin = max(box1.x_min, box2.x_min);
    float intersection_ymin = max(box1.y_min, box2.y_min);
    float intersection_xmax = min(box1.x_max, box2.x_max);
    float intersection_ymax = min(box1.y_max, box2.y_max);
    float intersection_area = max(intersection_xmax - intersection_xmin, 0) * max(intersection_ymax - intersection_ymin, 0);
    float union_area = (box1.x_max - box1.x_min) * (box1.y_max - box1.y_min) + (box2.x_max - box2.x_min) * (box2.y_max - box2.y_min) - intersection_area;
    return intersection_area / union_area;
}

//Suppress overlapping boxes (non-maxima-suppression) return sorted boxes by score, without overlapping
void suppressOverlappingBoxes(box* boxes, int num_of_detections, float iou_threshold){
    sortBoxesEfficient(boxes, num_of_detections);
    float iou;
    for(int i = 0; i < num_of_detections; i++){
        for(int j = i+1; j < num_of_detections; j++){
            iou = calculateIOU(boxes[i], boxes[j]);
            if (iou > iou_threshold && boxes[i].label == boxes[j].label){
                boxes[j].score = 0;
            }
        }
    }
}



int countNonNullBoxes(box* boxes, int num_of_detections){
    int count = 0;
    for(int i = 0; i < num_of_detections; i++){
        if (boxes[i].score > 0) count++;
    }
    return count;
}

void copyBoxes(box* boxes, box* boxes2, int num_of_boxes){
    for(int i = 0; i < num_of_boxes; i++){
        boxes2[i] = boxes[i];
    }
}

int postProcessing(float* locations, float* classes,
                    int num_of_detections, char * anchor_file,
                    int num_of_classes, float score_threshold, float nms_threshold,
                    float y_scale, float x_scale, float h_scale, float w_scale,
                    box* boxes){

    //Load detections and anchors in struct array
    detection* dets = (detection*)malloc(num_of_detections*sizeof(detection));
    if(loadDetectionStruct(locations, classes, num_of_detections, num_of_classes, anchor_file, dets)!=0){
        printf("Error loading detection struct");
        free(dets);
        return 1;
    }

    //Convert detections to boxes
    applyAnchors(dets, num_of_detections, boxes, y_scale, x_scale, h_scale, w_scale);
    free(dets);
    suppressLowScoreBoxes(boxes, num_of_detections, score_threshold);
    suppressOverlappingBoxes(boxes, num_of_detections, nms_threshold);


    return 0;
}
