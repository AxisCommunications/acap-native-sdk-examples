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
 * This header file handles the vdo part of the application.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "vdo-error.h"
#include "vdo-stream.h"
#include "vdo-types.h"

// This is a limitation from vdo
#define MAX_NBR_IMG_PROVIDER_BUFFERS 5

/**
 * @brief A type representing the buffers from vdo
 *
 * Contains information needed by larod to be able to set correct
 * properties on the input tensors.
 */
typedef struct img_info {
    VdoFormat format;
    unsigned int width;
    unsigned int height;
    unsigned int pitch;

    unsigned int nbr_buffers;
    bool dmabuf;
} img_info_t;

typedef struct img_framerate {
    unsigned int frametime;
    unsigned int mean_analysis_time;
    unsigned int analysis_frame_count;
    unsigned int tot_analysis_time;
    double wanted_framerate;
    double framerate;
} img_framerate_t;

/**
 * @brief Update framerate for a VdoStream
 *
 * @param stream         The VdoStream to change framerate for
 * @param img_framerate  A struct containing all needed for framerate calculation
 * @param analysis_time  The analysis time to be used for
 * framerate calculation
 *
 * @return true if framerate was set otherwise false
 */
bool img_util_update_framerate(VdoStream* stream,
                               img_framerate_t* img_framerate,
                               unsigned int analysis_time);

/**
 * @brief Flush all buffers
 *
 * @param stream         The VdoStream to vlush buffers
 * @param buf            The VdoBuffer to return
 * @param error          If function fails this will be set
 *
 * @return true if flush was successful
 */
bool img_util_flush(VdoStream* stream, VdoBuffer** buf, GError** error);
