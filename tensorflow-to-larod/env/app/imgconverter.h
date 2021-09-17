/**
 * Copyright (C) 2019-2020, Axis Communications AB, Lund, Sweden
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

/**
 * This header file handles the libyuv part of the application.
 */

#pragma once

#include <arm_neon.h>
#include <stdbool.h>

#include "stdint.h"

/**
 * brief Converts an input NV12 image to float interleaved RGB.
 *
 * Output floats will have range of outSwing and centered around
 * outCenter. Example: if output range should be -2.0 to -6.0 we
 * provide outSwing = 4.0 and outCenter = -4.0.
 *
 * param width Width of input image.
 * param height Height of input image.
 * param inBuffer Memory address to start of input image buffer.
 * param outBuffer Memory address to start of output image buffer.
 * param outSwing Max per pixel distance from outCenter in output image.
 * param outCenter Conceptual mean of pixel values in output image.
 */
void convertU8yuvToFloat32RGB(unsigned int width, unsigned int height,
                              uint8_t* inBuffer, float* outBuffer,
                              float outSwing, float outCenter);

/**
 * brief Converts an input NV12 image to uint8 RGB.
 *
 * There is one 'naive' implementation in unoptimized C code and one
 * more optimized implementation using libYuv.
 * param width Width of input image.
 * param height Height of input image.
 * param yuvIn Memory address to start of input image buffer.
 * param rgbOut Memory address to start of output image buffer.
 */
void convertU8yuvToRGBlibYuv(unsigned int width, unsigned int height,
                             uint8_t* yuvIn, uint8_t* rgbOut);
void convertU8yuvToRGBnaive(unsigned int width, unsigned int height,
                            uint8_t* yuvIn, uint8_t* rgbOut);

/**
 * brief Convert, crop and scale image.
 *
 * 1. Converts the input NV12 image to BGRA format.
 * 2. Scale a region-of-interest to the destination size. The ROI will have
 *    same aspect ratio as dstWidht/dstHeight. While keeping this aspect ratio
 *    the ROI is expanded until it reaches srcHeight or srcWidth. Thus there
 *    will be some border cut off if the input aspect ratio is exactly the
 *    same as the output image.
 * 3. Convert the downscaled BGRA to RGB image.
 *
 * param nv12Data Pointer to start of NV12 data. UV plane is expected to be
 *                 placed directly after Y data.
 * param srcWidth Source image width in pixels.
 * param srcHeight Source image height in pixels.
 * param rgbData Start of output scaled RGB image.
 * param dstWidth Destination image width in pixels.
 * param False if any errors occur, otherwise true.
 */
bool convertCropScaleU8yuvToRGB(const uint8_t* nv12Data, unsigned int srcWidth,
                                unsigned int srcHeight, uint8_t* rgbData,
                                unsigned int dstWidth, unsigned int dstHeight);
