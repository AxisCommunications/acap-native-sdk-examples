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
 * This file handles the vdo part of the application.
 */

#include "imgprovider.h"

#include <assert.h>
#include <errno.h>
#include <gmodule.h>
#include <syslog.h>
#include <vdo-channel.h>

#include "vdo-map.h"

#define VDO_CHANNEL (1)

/**
 * brief Set up a stream through VDO.
 *
 * Set up stream settings, allocate image buffers and map memory.
 *
 * param provider ImageProvider pointer.
 * param w Requested stream width.
 * param h Requested stream height.
 * param provider Pointer to ImgProvider starting the stream.
 * return False if any errors occur, otherwise true.
 */
static bool createStream(ImgProvider_t* provider, unsigned int w,
                         unsigned int h);

/**
 * brief Allocate VDO buffers on a stream.
 *
 * Note that buffers are not relased upon error condition.
 *
 * param provider ImageProvider pointer.
 * param vdoStream VDO stream for buffer allocation.
 * return False if any errors occur, otherwise true.
 */
static bool allocateVdoBuffers(ImgProvider_t* provider, VdoStream* vdoStream);

/**
 * brief Release references to the buffers we allocated in createStream().
 *
 * param provider Pointer to ImgProvider owning the buffer references.
 */
static void releaseVdoBuffers(ImgProvider_t* provider);

/**
 * brief Starting point function for the thread fetching frames.
 *
 * Responsible for fetching buffers/frames from VDO and re-enqueue buffers back
 * to VDO when they are not needed by the application. The ImgProvider always
 * keeps one or several of the most recent frames available in the application.
 * There are two queues involved: deliveredFrames and processedFrames.
 * - deliveredFrames are frames delivered from VDO and
 *   not processed by the client.
 * - processedFrames are frames that the client has consumed and handed
 *   back to the ImgProvider.
 * The thread works roughly like this:
 * 1. The thread blocks on vdo_stream_get_buffer() until VDO deliver a new
 * frame.
 * 2. The fresh frame is put at the end of the deliveredFrame queue. If the
 *    client want to fetch a frame the item at the end of deliveredFrame
 *    list is returned.
 * 3. If there are any frames in the processedFrames list one of these are
 *    enqueued back to VDO to keep the flow of buffers.
 * 4. If the processedFrames list is empty we instead check if there are
 *    frames available in the deliveredFrames list. We want to make sure
 *    there is at least numAppFrames buffers available to the client to
 *    fetch. If there are more than numAppFrames in deliveredFrames we
 *    pick the first buffer (oldest) in the list and enqueue it to VDO.

 * param data Pointer to ImgProvider owning thread.
 * return Pointer to unused return data.
 */
static void* threadEntry(void* data);

ImgProvider_t* createImgProvider(unsigned int w, unsigned int h,
                                 unsigned int numFrames, VdoFormat format) {
    bool mtxInitialized = false;
    bool condInitialized = false;

    ImgProvider_t* provider = calloc(1, sizeof(ImgProvider_t));
    if (!provider) {
        syslog(LOG_ERR, "%s: Unable to allocate ImgProvider: %s", __func__,
                 strerror(errno));
        goto errorExit;
    }

    provider->vdoFormat = format;
    provider->numAppFrames = numFrames;

    if (pthread_mutex_init(&provider->frameMutex, NULL)) {
        syslog(LOG_ERR, "%s: Unable to initialize mutex: %s", __func__,
                 strerror(errno));
        goto errorExit;
    }
    mtxInitialized = true;

    if (pthread_cond_init(&provider->frameDeliverCond, NULL)) {
        syslog(LOG_ERR, "%s: Unable to initialize condition variable: %s", __func__,
                 strerror(errno));
        goto errorExit;
    }
    condInitialized = true;

    provider->deliveredFrames = g_queue_new();
    if (!provider->deliveredFrames) {
        syslog(LOG_ERR, "%s: Unable to create deliveredFrames queue!", __func__);
        goto errorExit;
    }

    provider->processedFrames = g_queue_new();
    if (!provider->processedFrames) {
        syslog(LOG_ERR, "%s: Unable to create processedFrames queue!", __func__);
        goto errorExit;
    }

    if (!createStream(provider, w, h)) {
        syslog(LOG_ERR, "%s: Could not create VDO stream!", __func__);
        goto errorExit;
    }

    return provider;

errorExit:
    if (mtxInitialized) {
        pthread_mutex_destroy(&provider->frameMutex);
    }
    if (condInitialized) {
        pthread_cond_destroy(&provider->frameDeliverCond);
    }
    if (provider->deliveredFrames) {
        g_queue_free(provider->deliveredFrames);
    }
    if (provider->processedFrames) {
        g_queue_free(provider->processedFrames);
    }

    free(provider);

    return NULL;
}

void destroyImgProvider(ImgProvider_t* provider) {
    if (!provider) {
        syslog(LOG_ERR, "%s: Invalid pointer to ImgProvider", __func__);
        return;
    }

    releaseVdoBuffers(provider);

    pthread_mutex_destroy(&provider->frameMutex);
    pthread_cond_destroy(&provider->frameDeliverCond);

    g_queue_free(provider->deliveredFrames);
    g_queue_free(provider->processedFrames);

    free(provider);
}

bool allocateVdoBuffers(ImgProvider_t* provider, VdoStream* vdoStream) {
    GError* error = NULL;
    bool ret = false;

    assert(provider);
    assert(vdoStream);

    for (size_t i = 0; i < NUM_VDO_BUFFERS; i++) {
        provider->vdoBuffers[i] =
            vdo_stream_buffer_alloc(vdoStream, NULL, &error);
        if (provider->vdoBuffers[i] == NULL) {
            syslog(LOG_ERR, "%s: Failed creating VDO buffer: %s", __func__,
                     (error != NULL) ? error->message : "N/A");
            goto errorExit;
        }

        // Make a 'speculative' vdo_buffer_get_data() call to trigger a
        // memory mapping of the buffer. The mapping is cached in the VDO
        // implementation.
        void* dummyPtr = vdo_buffer_get_data(provider->vdoBuffers[i]);
        if (!dummyPtr) {
            syslog(LOG_ERR, "%s: Failed initializing buffer memmap: %s", __func__,
                     (error != NULL) ? error->message : "N/A");
            goto errorExit;
        }

        if (!vdo_stream_buffer_enqueue(vdoStream, provider->vdoBuffers[i],
                                       &error)) {
            syslog(LOG_ERR, "%s: Failed enqueue VDO buffer: %s", __func__,
                     (error != NULL) ? error->message : "N/A");
            goto errorExit;
        }
    }

    ret = true;

errorExit:
    g_clear_error(&error);

    return ret;
}

bool chooseStreamResolution(unsigned int reqWidth, unsigned int reqHeight,
                            unsigned int* chosenWidth,
                            unsigned int* chosenHeight) {
    VdoResolutionSet* set = NULL;
    VdoChannel* channel = NULL;
    GError* error = NULL;
    bool ret = false;

    assert(chosenWidth);
    assert(chosenHeight);

    // Retrieve channel resolutions
    channel = vdo_channel_get(VDO_CHANNEL, &error);
    if (!channel) {
        syslog(LOG_ERR, "%s: Failed vdo_channel_get(): %s", __func__,
                 (error != NULL) ? error->message : "N/A");
        goto end;
    }
    set = vdo_channel_get_resolutions(channel, NULL, &error);
    if (!set) {
        syslog(LOG_ERR, "%s: Failed vdo_channel_get_resolutions(): %s", __func__,
                 (error != NULL) ? error->message : "N/A");
        goto end;
    }

    // Find smallest VDO stream resolution that fits the requested size.
    ssize_t bestResolutionIdx = -1;
    unsigned int bestResolutionArea = UINT_MAX;
    for (ssize_t i = 0; (gsize) i < set->count; ++i) {
        VdoResolution* res = &set->resolutions[i];
        if ((res->width >= reqWidth) && (res->height >= reqHeight)) {
            unsigned int area = res->width * res->height;
            if (area < bestResolutionArea) {
                bestResolutionIdx = i;
                bestResolutionArea = area;
            }
        }
    }

    // If we got a reasonable w/h from the VDO channel info we use that
    // for creating the stream. If that info for some reason was empty we
    // fall back to trying to create a stream with client-supplied w/h.
    *chosenWidth = reqWidth;
    *chosenWidth = reqHeight;
    if (bestResolutionIdx >= 0) {
        *chosenWidth = set->resolutions[bestResolutionIdx].width;
        *chosenHeight = set->resolutions[bestResolutionIdx].height;
        syslog(LOG_INFO, "%s: We select stream w/h=%u x %u based on VDO channel info.\n",
                __func__, *chosenWidth, *chosenHeight);
    } else {
        syslog(LOG_WARNING, "%s: VDO channel info contains no reslution info. Fallback "
                   "to client-requested stream resolution.",
                   __func__);
    }

    ret = true;

end:
    g_clear_object(&channel);
    g_free(set);
    g_clear_error(&error);

    return ret;
}

bool createStream(ImgProvider_t* provider, unsigned int w, unsigned int h) {
    VdoMap* vdoMap = vdo_map_new();
    GError* error = NULL;
    bool ret = false;

    if (!vdoMap) {
        syslog(LOG_ERR, "%s: Failed to create vdo_map", __func__);
        goto end;
    }

    vdo_map_set_uint32(vdoMap, "channel", VDO_CHANNEL);
    vdo_map_set_uint32(vdoMap, "format", provider->vdoFormat);
    vdo_map_set_uint32(vdoMap, "width", w);
    vdo_map_set_uint32(vdoMap, "height", h);
    // We will use buffer_alloc() and buffer_unref() calls.
    vdo_map_set_uint32(vdoMap, "buffer.strategy", VDO_BUFFER_STRATEGY_EXPLICIT);

    syslog(LOG_INFO, "Dump of vdo stream settings map =====");
    vdo_map_dump(vdoMap);

    VdoStream* vdoStream = vdo_stream_new(vdoMap, NULL, &error);
    if (!vdoStream) {
        syslog(LOG_ERR, "%s: Failed creating vdo stream: %s", __func__,
                 (error != NULL) ? error->message : "N/A");
        goto errorExit;
    }

    if (!allocateVdoBuffers(provider, vdoStream)) {
        syslog(LOG_ERR, "%s: Failed setting up VDO buffers!", __func__);
        goto errorExit;
    }

    // Start the actual VDO streaming.
    if (!vdo_stream_start(vdoStream, &error)) {
        syslog(LOG_ERR, "%s: Failed starting stream: %s", __func__,
                 (error != NULL) ? error->message : "N/A");
        goto errorExit;
    }

    provider->vdoStream = vdoStream;

    ret = true;

    goto end;

errorExit:
    // Clean up allocated buffers if any
    releaseVdoBuffers(provider);

end:
    // Always do this
    g_object_unref(vdoMap);
    g_clear_error(&error);
    return ret;
}

static void releaseVdoBuffers(ImgProvider_t* provider) {
    if (!provider->vdoStream) {
        return;
    }

    for (size_t i = 0; i < NUM_VDO_BUFFERS; i++) {
        if (provider->vdoBuffers[i] != NULL) {
            vdo_stream_buffer_unref(provider->vdoStream,
                                    &provider->vdoBuffers[i], NULL);
        }
    }
}

VdoBuffer* getLastFrameBlocking(ImgProvider_t* provider) {
    VdoBuffer* returnBuf = NULL;
    pthread_mutex_lock(&provider->frameMutex);

    while (g_queue_get_length(provider->deliveredFrames) < 1) {
        if (pthread_cond_wait(&provider->frameDeliverCond,
                              &provider->frameMutex)) {
            syslog(LOG_ERR, "%s: Failed to wait on condition: %s", __func__,
                     strerror(errno));
            goto errorExit;
        }
    }

    returnBuf = g_queue_pop_tail(provider->deliveredFrames);

errorExit:
    pthread_mutex_unlock(&provider->frameMutex);

    return returnBuf;
}

void returnFrame(ImgProvider_t* provider, VdoBuffer* buffer) {
    pthread_mutex_lock(&provider->frameMutex);

    g_queue_push_tail(provider->processedFrames, buffer);

    pthread_mutex_unlock(&provider->frameMutex);
}

static void* threadEntry(void* data) {
    GError* error = NULL;
    ImgProvider_t* provider = (ImgProvider_t*) data;

    while (!provider->shutDown) {
        // Block waiting for a frame from VDO
        VdoBuffer* newBuffer =
            vdo_stream_get_buffer(provider->vdoStream, &error);

        if (!newBuffer) {
            // Fail but we continue anyway hoping for the best.
            syslog(LOG_WARNING, "%s: Failed fetching frame from vdo: %s", __func__,
                       (error != NULL) ? error->message : "N/A");
            g_clear_error(&error);
            continue;
        }
        pthread_mutex_lock(&provider->frameMutex);

        g_queue_push_tail(provider->deliveredFrames, newBuffer);

        VdoBuffer* oldBuffer = NULL;

        // First check if there are any frames returned from app
        // processing
        if (g_queue_get_length(provider->processedFrames) > 0) {
            oldBuffer = g_queue_pop_head(provider->processedFrames);
        } else {
            // Client specifies the number-of-recent-frames it needs to collect
            // in one chunk (numAppFrames). Thus only enqueue buffers back to
            // VDO if we have collected more buffers than numAppFrames.
            if (g_queue_get_length(provider->deliveredFrames) >
                provider->numAppFrames) {
                oldBuffer = g_queue_pop_head(provider->deliveredFrames);
            }
        }

        if (oldBuffer) {
            if (!vdo_stream_buffer_enqueue(provider->vdoStream, oldBuffer,
                                           &error)) {
                // Fail but we continue anyway hoping for the best.
                syslog(LOG_WARNING, "%s: Failed enqueueing buffer to vdo: %s", __func__,
                           (error != NULL) ? error->message : "N/A");
                g_clear_error(&error);
            }
        }
        pthread_cond_signal(&provider->frameDeliverCond);
        pthread_mutex_unlock(&provider->frameMutex);
    }
}

bool startFrameFetch(ImgProvider_t* provider) {
    if (pthread_create(&provider->fetcherThread, NULL, threadEntry, provider)) {
        syslog(LOG_ERR, "%s: Failed to start thread fetching frames from vdo: %s",
                 __func__, strerror(errno));
        return false;
    }

    return true;
}

bool stopFrameFetch(ImgProvider_t* provider) {
    provider->shutDown = true;

    if (pthread_join(provider->fetcherThread, NULL)) {
        syslog(LOG_ERR, "%s: Failed to join thread fetching frames from vdo: %s",
                 __func__, strerror(errno));
        return false;
    }

    return true;
}
