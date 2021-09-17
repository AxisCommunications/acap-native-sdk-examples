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
 * This file handles the libyuv part of the application.
 */

#include "imgconverter.h"

#include <arm_neon.h>
#include <assert.h>
#include <errno.h>
#include <libyuv.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>

#define ARGB_BYTES_PER_PIXEL (4)

static void ARGBtoRAW(const uint8_t* srcARGB, uint8_t* dstRGB, unsigned int w,
                      int unsigned h);

void convertU8yuvToRGBlibYuv(unsigned int width, unsigned int height,
                             uint8_t* yuvIn, uint8_t* rgbOut) {
    const uint8_t* src_y = yuvIn;
    int src_stride_y = (int) width;
    const uint8_t* src_uv = yuvIn + (width * height);
    int src_stride_uv = (int) width;
    uint8_t* dst_raw = rgbOut;
    int dst_stride_raw = 3 * (int) width;

    // libyuv 'RAW' format is RGB, while libyuv 'RGB24' is stored as BGR in
    // memory
    int result = NV12ToRAW(src_y, src_stride_y, src_uv, src_stride_uv, dst_raw,
                           dst_stride_raw, (int) width, (int) height);

    if (result != 0) {
        syslog(LOG_ERR, "%s: Failed NV12ToRAW(), result=%d!", __func__, result);
    }
}

void convertU8yuvToRGBnaive(unsigned int width, unsigned int height,
                            uint8_t* yuvIn, uint8_t* rgbOut) {
    uint8_t* yPlane = yuvIn;
    uint8_t* uvPlane = yPlane + (width * height);

    for (unsigned int yPos = 0; yPos < height; yPos++) {

        uint8_t* uvLine = uvPlane + (yPos / 2) * width;

        for (unsigned int x = 0; x < width; x++) {

            uint8_t Yval = yPlane[0];
            uint8_t Uval = uvLine[0];
            uint8_t Vval = uvLine[1];

            // Get y in range 0.0..1.0 and U,V in range -0.5..0.5.
            float y = Yval / 255.0f;
            float u = (Uval / 255.0f) - 0.5f;
            float v = (Vval / 255.0f) - 0.5f;

            // This conversion is from:
            // https://gist.github.com/CreaRo/0d50442145b63c6c288d1c1675909990
            float r = y + 1.13983f * v;
            float g = y - 0.39465f * u - 0.58060f * v;
            float b = y + 2.03211f * u;

            // Write to output array.
            rgbOut[0] = (uint8_t)(r * 255.0f);
            rgbOut[1] = (uint8_t)(g * 255.0f);
            rgbOut[2] = (uint8_t)(b * 255.0f);

            rgbOut += 3;
            yPlane++;
            uvLine += (x % 2) * 2; // Step forward one UV-pair in the uvPlane
                                   // every second loop.
        }
    }
}

void convertU8yuvToFloat32RGB(unsigned int width, unsigned int height,
                              uint8_t* inBuffer, float* outBuffer,
                              float outSwing, float outCenter) {
    uint8_t* yPlane = inBuffer;
    uint8_t* uvPlane = yPlane + (width * height);

    for (unsigned int yPos = 0; yPos < height; yPos++) {

        uint8_t* uvLine = uvPlane + (yPos / 2) * width;

        for (unsigned int x = 0; x < width; x++) {

            uint8_t Yval = yPlane[0];
            uint8_t Uval = uvLine[0];
            uint8_t Vval = uvLine[1];

            // Get y in range 0.0..1.0 and U,V in range -0.5..0.5.
            float y = Yval / 255.0f;
            float u = (Uval / 255.0f) - 0.5f;
            float v = (Vval / 255.0f) - 0.5f;

            // This conversion is from:
            // https://gist.github.com/CreaRo/0d50442145b63c6c288d1c1675909990
            float r = y + 1.13983f * v;
            float g = y - 0.39465f * u - 0.58060f * v;
            float b = y + 2.03211f * u;

            // Scale to desired outpout range.
            float k = outCenter - outSwing / 2.0f;

            r = r * outSwing + k;
            g = g * outSwing + k;
            b = b * outSwing + k;

            // Clamp.
            float maxV = outCenter + outSwing / 2.0f;
            float minV = outCenter - outSwing / 2.0f;

            if (r < minV) {
                r = minV;
            }
            if (g < minV) {
                g = minV;
            }
            if (b < minV) {
                b = minV;
            }
            if (r > maxV) {
                r = maxV;
            }
            if (g > maxV) {
                g = maxV;
            }
            if (b > maxV) {
                b = maxV;
            }

            // Write to output array.
            outBuffer[0] = r;
            outBuffer[1] = g;
            outBuffer[2] = b;

            outBuffer += 3;
            yPlane++;
            uvLine += (x % 2) * 2; // Step forward one UV-pair in the uvPlane
                                   // every second loop.
        }
    }
}

void ARGBtoRAW(const uint8_t* srcARGB, uint8_t* dstRGB, unsigned int w,
               unsigned int h) {

    const uint8_t* src = srcARGB;
    uint8_t* dst = dstRGB;
    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
            uint8_t b = *src++;
            uint8_t g = *src++;
            uint8_t r = *src++;
            src++; // Skip Alpha channel
            *dst++ = r;
            *dst++ = g;
            *dst++ = b;
        }
    }
}

bool convertCropScaleU8yuvToRGB(const uint8_t* nv12Data, unsigned int srcWidth,
                                unsigned int srcHeight, uint8_t* rgbData,
                                unsigned int dstWidth, unsigned int dstHeight) {
    bool ret = false;
    uint8_t* tempARGBbig = NULL;
    uint8_t* tempARGBsmall = NULL;

    tempARGBbig = malloc((size_t)(srcWidth * srcHeight * ARGB_BYTES_PER_PIXEL));
    if (!tempARGBbig) {
        syslog(LOG_ERR, "%s: Failed allocating big tempARGB buffer: %s", __func__,
                 strerror(errno));
        goto end;
    }

    tempARGBsmall =
        malloc((size_t)(dstWidth * dstHeight * ARGB_BYTES_PER_PIXEL));
    if (!tempARGBsmall) {
        syslog(LOG_ERR, "%s: Failed allocating small tempARGB buffer: %s", __func__,
                 strerror(errno));
        goto end;
    }

    const uint8_t* uvPlane = nv12Data + (srcWidth * srcHeight);
    unsigned int bigARGBstride = ARGB_BYTES_PER_PIXEL * srcWidth;
    unsigned int smallARGBstride = ARGB_BYTES_PER_PIXEL * dstWidth;
    int result = NV12ToARGB(nv12Data, (int) srcWidth, uvPlane, (int) srcWidth,
                            tempARGBbig, (int) bigARGBstride, (int) srcWidth,
                            (int) srcHeight);
    if (result != 0) {
        syslog(LOG_ERR, "%s: Failed NV12ToARGB() with result=%d", __func__, result);
        goto end;
    }

    // 1. The crop area shall fill the input image either horizontally or
    //    vertically.
    // 2. The crop area shall have the same aspect ratio as the output image.
    float destWHratio = (float) srcWidth / (float) srcHeight;

    float clipW = (float) srcWidth;
    float clipH = clipW / destWHratio;
    if (clipH > (float) srcHeight) {
        clipH = (float) srcHeight;
        clipW = clipH * destWHratio;
    }

    unsigned int clipX = (srcWidth - (unsigned int) clipW) / 2;
    unsigned int clipY = (srcHeight - (unsigned int) clipH) / 2;

    uint8_t* bigARGBcrop =
        tempARGBbig + (bigARGBstride * clipY) + (ARGB_BYTES_PER_PIXEL * clipX);

    // We use pointer offset (bigARGBcrop), clipW and clipH to realize the
    // cropping of source image.
    result = ARGBScale(bigARGBcrop, (int) bigARGBstride, (int) clipW,
                       (int) clipH, tempARGBsmall, (int) smallARGBstride,
                       (int) dstWidth, (int) dstHeight, kFilterBilinear);
    if (result != 0) {
        syslog(LOG_ERR, "%s: Failed ARGBScale() with result=%d", __func__, result);
        goto end;
    }

    ARGBtoRAW(tempARGBsmall, rgbData, dstWidth, dstHeight);

    ret = true;

end:
    if (tempARGBbig) {
        free(tempARGBbig);
        tempARGBbig = NULL;
    }
    if (tempARGBsmall) {
        free(tempARGBsmall);
        tempARGBsmall = NULL;
    }

    return ret;
}
