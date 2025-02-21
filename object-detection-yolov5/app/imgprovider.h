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

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "vdo-stream.h"
#include "vdo-types.h"

#define NUM_VDO_BUFFERS (8)

/**
 * brief A type representing a provider of frames from VDO.
 *
 * Keep track of what kind of images the user wants, all the necessary
 * VDO types to setup and maintain a stream, as well as parameters to make
 * the streaming thread safe.
 */
typedef struct img_provider {
    /// Stream configuration parameters.
    VdoFormat vdo_format;

    /// Vdo stream and buffers handling.
    VdoStream* vdo_stream;
    VdoBuffer* vdo_buffers[NUM_VDO_BUFFERS];

    /// Keeping track of frames' statuses.
    GQueue* delivered_frames;
    GQueue* processed_frames;
    /// Number of frames to keep in the deliveredFrames queue.
    unsigned int num_app_frames;

    /// To support fetching frames asynchonously with VDO.
    pthread_mutex_t frame_mutex;
    pthread_cond_t frame_deliver_cond;
    pthread_t fetcher_thread;
    atomic_bool shut_down;
} img_provider_t;

/**
 * brief Find VDO resolution that best fits requirement.
 *
 * Queries available stream resolutions in native aspect raito from VDO and
 * selects the smallest that fits the requested width and height. If no valid
 * resolutions are reported by VDO then the original w/h are returned as
 * chosenWidth/chosenHeight.
 *
 * param reqWidth Requested image width.
 * param reqHeight Requested image height.
 * param chosenWidth Selected image width.
 * param chosenHeight Selected image height.
 * return False if any errors occur, otherwise true.
 */
void choose_stream_resolution(unsigned int req_width,
                              unsigned int req_height,
                              unsigned int* chosen_width,
                              unsigned int* chosen_height);

/**
 * brief Initializes and starts an ImgProvider.
 *
 * Make sure to check ImgProvider_t streamWidth and streamHeight members to
 * find resolution of the created stream. These numbers might not match the
 * requested resolution depending on platform properties.
 *
 * param w Requested output image width.
 * param h Requested ouput image height.
 * param numFrames Number of fetched frames to keep.
 * param vdoFormat Image format to be output by stream.
 * return Pointer to new ImgProvider, or NULL if failed.
 */
img_provider_t*
create_img_provider(unsigned int w, unsigned int h, unsigned int num_frames, VdoFormat vdo_format);

/**
 * brief Release VDO buffers and deallocate provider.
 *
 * param provider Pointer to ImgProvider to be destroyed.
 */
void destroy_img_provider(img_provider_t* provider);

/**
 * brief Create the thread and start fetching frames.
 *
 * param provider Pointer to ImgProvider whose thread to be started.
 * return False if any errors occur, otherwise true.
 */
void start_frame_fetch(img_provider_t* provider);

/**
 * brief Stop fetching frames by closing thread.
 *
 * param provider Pointer to ImgProvider whose thread to be stopped.
 * return False if any errors occur, otherwise true.
 */
void stop_frame_fetch(img_provider_t* provider);

/**
 * brief Get the most recent frame the thread has fetched from VDO.
 *
 * param provider Pointer to an ImgProvider fetching frames.
 * return Pointer to an image buffer on success, otherwise NULL.
 */
VdoBuffer* get_last_frame_blocking(img_provider_t* provider);

/**
 * brief Release reference to an image buffer.
 *
 * param provider Pointer to an ImgProvider fetching frames.
 * param buffer Pointer to the image buffer to be released.
 */
void return_frame(img_provider_t* provider, VdoBuffer* buffer);

guint32 get_stream_rotation(img_provider_t* provider);
