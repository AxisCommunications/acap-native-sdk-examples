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

#define MAX_NBR_IMG_PROVIDER_BUFFERS 5

/**
 * @brief A type representing a provider of frames from VDO.
 *
 * Keep track of what kind of images the user wants, all the necessary
 * VDO types to setup and maintain a stream.
 */
typedef struct img_provider {
    /// Stream format, typically YUV.
    VdoFormat format;

    /// Vdo stream object.
    VdoStream* vdo_stream;

    /// Number of frames to cache in vdo, default is 3.
    unsigned int buffer_count;

    // These values are updated from the info map for the stream
    // This means that they will follow rotation and may then differ from the
    // values the stream was created with
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    double framerate;
    double requested_framerate;
    unsigned int rotation;
    unsigned int channel;

    // Used for chaging framerate if needed
    unsigned int frametime;
    unsigned int mean_analysis_time;
    unsigned int analysis_frame_count;
    unsigned int tot_analysis_time;

    int fd;
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
 * @brief Find VDO resolution that best fits requirement.
 *
 * Checks is the requested witdh and height is within the valid
 * range reported by vdo. If no valid resolutions are reported
 * by VDO then the original w/h are returned as chosenWidth/chosenHeight.
 *
 * @param req_width      Requested image width.
 * @param req_height     Requested image height.
 * @param format         Requested format for the stream.
 * @param aspect_ratio   Request a specific aspect ratio, can be NULL.
 * @param select         Default is minmax, but could be all or minmax
 * @param chosen_width   Selected image width.
 * @param chosen_height  Selected image height.
 *
 * @return false if any errors occur, otherwise true.
 */
bool choose_stream_resolution(unsigned int req_width,
                              unsigned int req_height,
                              VdoFormat format,
                              const char* aspect_ratio,
                              const char* select,
                              unsigned int* chosen_width,
                              unsigned int* chosen_height);

/**
 * @brief Initializes an ImgProvider.
 *
 * Make sure to check ImgProvider_t streamWidth and streamHeight members to
 * find resolution of the created stream. These numbers might not match the
 * requested resolution depending on platform properties.
 *
 * @param width       Requested output image width.
 * @param height      Requested ouput image height.
 * @param num_buffers Number of fetched frames to keep inside vdo
 * @param format      Image format to be output by stream.
 * @param framerate   The framerate to retrive images in
 *
 * @return Pointer to new ImgProvider, or NULL if failed.
 */
img_provider_t* create_img_provider(unsigned int width,
                                    unsigned int height,
                                    unsigned int num_buffers,
                                    VdoFormat format,
                                    double framerate);

/**
 * @brief Release VDO Stream object and deallocate provider.
 *
 * @param provider Pointer to ImgProvider to be destroyed.
 *
 * @return void
 */
void destroy_img_provider(img_provider_t* provider);
