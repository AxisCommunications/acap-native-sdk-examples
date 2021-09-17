/**
 * Copyright (C) 2018-2020, Axis Communications AB, Lund, Sweden
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
typedef struct ImgProvider {
    /// Stream configuration parameters.
    VdoFormat vdoFormat;

    /// Vdo stream and buffers handling.
    VdoStream* vdoStream;
    VdoBuffer* vdoBuffers[NUM_VDO_BUFFERS];

    /// Keeping track of frames' statuses.
    GQueue* deliveredFrames;
    GQueue* processedFrames;
    /// Number of frames to keep in the deliveredFrames queue.
    unsigned int numAppFrames;

    /// To support fetching frames asynchonously with VDO.
    pthread_mutex_t frameMutex;
    pthread_cond_t frameDeliverCond;
    pthread_t fetcherThread;
    atomic_bool shutDown;
} ImgProvider_t;

/**
 * brief Find VDO resolution that best fits requirement.
 *
 * Queries available stream resolutions from VDO and selects the smallest that
 * fits the requested width and height. If no valid resolutions are reported
 * by VDO then the original w/h are returned as chosenWidth/chosenHeight.
 *
 * param reqWidth Requested image width.
 * param reqHeight Requested image height.
 * param chosenWidth Selected image width.
 * param chosenHeight Selected image height.
 * return False if any errors occur, otherwise true.
 */
bool chooseStreamResolution(unsigned int reqWidth, unsigned int reqHeight,
                            unsigned int* chosenWidth,
                            unsigned int* chosenHeight);

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
ImgProvider_t* createImgProvider(unsigned int w, unsigned int h,
                                 unsigned int numFrames, VdoFormat vdoFormat);

/**
 * brief Release VDO buffers and deallocate provider.
 *
 * param provider Pointer to ImgProvider to be destroyed.
 */
void destroyImgProvider(ImgProvider_t* provider);

/**
 * brief Create the thread and start fetching frames.
 *
 * param provider Pointer to ImgProvider whose thread to be started.
 * return False if any errors occur, otherwise true.
 */
bool startFrameFetch(ImgProvider_t* provider);

/**
 * brief Stop fetching frames by closing thread.
 *
 * param provider Pointer to ImgProvider whose thread to be stopped.
 * return False if any errors occur, otherwise true.
 */
bool stopFrameFetch(ImgProvider_t* provider);

/**
 * brief Get the most recent frame the thread has fetched from VDO.
 *
 * param provider Pointer to an ImgProvider fetching frames.
 * return Pointer to an image buffer on success, otherwise NULL.
 */
VdoBuffer* getLastFrameBlocking(ImgProvider_t* provider);

/**
 * brief Release reference to an image buffer.
 *
 * param provider Pointer to an ImgProvider fetching frames.
 * param buffer Pointer to the image buffer to be released.
 */
void returnFrame(ImgProvider_t* provider, VdoBuffer* buffer);
