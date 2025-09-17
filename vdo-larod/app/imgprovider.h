/**
 * Copyright (C) 2018-2021, Axis Communications AB, Lund, Sweden
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
 * properties on the imgInputTensors.
 */
typedef struct img_info {
    VdoFormat format;
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    double framerate;
    unsigned int rotation;
} img_info_t;

/**
 * @brief A type representing a provider of frames from VDO.
 *
 * Keep track of what kind of images the user wants, all the necessary
 * VDO types to setup and maintain a stream.
 */
typedef struct img_provider {
    /// Stream format, typically YUV.

    /// Vdo stream object.
    VdoStream* vdo_stream;

    /// Number of frames to cache in vdo, default is 3.
    unsigned int buffer_count;

    // These values are updated from the info map for the stream
    // This means that they will follow rotation and may then differ from the
    // values the stream was created with
    unsigned int channel;
    img_info_t* img_info;

    // Used for chaging framerate if needed
    unsigned int frametime;
    unsigned int mean_analysis_time;
    unsigned int analysis_frame_count;
    unsigned int tot_analysis_time;

    int fd;
    double wanted_framerate;
} img_provider_t;

/**
 * @brief Update framerate for the imgProvider
 *
 * @param provider       The imageprovider to be used
 * @param analysis_time  The analysis time to be used for
 * framerate calculation
 *
 * @return true if framerate was set otherwise false
 */
bool img_provider_update_framerate(img_provider_t* provider, unsigned int analysis_time);

/**
 * @brief Start the imgProvider and get fd for this imgprovider
 *
 * @param provider  The imageprovider to be used
 *
 * @return true if the imgprovider was started and a fd was get.
 */
bool img_provider_start(img_provider_t* provider);

/**
 * @brief Get a frame from the imgProvider
 *
 * @param provider  The imageprovider to be used
 *
 * @return NULL if expected error otherwise a VdoBuffer
 */
VdoBuffer* img_provider_get_frame(img_provider_t* provider);

/**
 * @brief Flush all frames in vdo
 *
 * @param provider  The imageprovider to be used
 *
 */
void img_provider_flush_all_frames(img_provider_t* provider);

/**
 * @brief Get metadata about the image
 *
 * @param provider  The imageprovider to be used
 *
 * @return img_info_t struct
 */
img_info_t img_provider_get_image_metadata(img_provider_t* provider);

/**
 * @brief Initializes an ImgProvider.
 *
 * Make sure to check ImgProvider_t streamWidth and streamHeight members to
 * find resolution of the created stream. These numbers might not match the
 * requested resolution depending on platform properties.
 *
 * @param input_channel   Video input channel to be used
 * @param img_info        Requested stream properties
 * @param num_buffers     Number of buffers the vdo will allocate for the stream
 * @param framerate       Initial framerate of the stream
 * @param image_fit       String that can be "crop" or "scale"
 *
 * @return Pointer to new ImgProvider, or NULL if failed.
 */
img_provider_t* img_provider_new(unsigned int input_channel,
                                 img_info_t* img_info,
                                 unsigned int num_buffers,
                                 double framerate,
                                 char* image_fit);

/**
 * @brief Release VDO Stream object and deallocate provider.
 *
 * @param provider Pointer to ImgProvider to be destroyed.
 *
 * @return void
 */
void img_provider_destroy(img_provider_t* provider);
