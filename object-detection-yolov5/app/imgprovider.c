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
 * This file handles the vdo part of the application.
 */

#include "imgprovider.h"

#include "panic.h"
#include "vdo-map.h"
#include <vdo-channel.h>

#include <assert.h>
#include <errno.h>
#include <gmodule.h>
#include <syslog.h>

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
static void create_stream(img_provider_t* provider, unsigned int w, unsigned int h);

/**
 * brief Allocate VDO buffers on a stream.
 *
 * Note that buffers are not relased upon error condition.
 *
 * param provider ImageProvider pointer.
 * param vdoStream VDO stream for buffer allocation.
 * return False if any errors occur, otherwise true.
 */
static void allocate_vdo_buffers(img_provider_t* provider, VdoStream* vdo_stream);

/**
 * brief Release references to the buffers we allocated in createStream().
 *
 * param provider Pointer to ImgProvider owning the buffer references.
 */
static void release_vdo_buffers(img_provider_t* provider);

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
static void* thread_entry(void* data);

img_provider_t*
create_img_provider(unsigned int w, unsigned int h, unsigned int num_frames, VdoFormat format) {
    img_provider_t* provider = calloc(1, sizeof(img_provider_t));
    if (!provider) {
        panic("%s: Unable to allocate ImgProvider: %s", __func__, strerror(errno));
    }

    provider->vdo_format     = format;
    provider->num_app_frames = num_frames;

    if (pthread_mutex_init(&provider->frame_mutex, NULL)) {
        panic("%s: Unable to initialize mutex: %s", __func__, strerror(errno));
    }

    if (pthread_cond_init(&provider->frame_deliver_cond, NULL)) {
        panic("%s: Unable to initialize condition variable: %s", __func__, strerror(errno));
    }

    provider->delivered_frames = g_queue_new();
    if (!provider->delivered_frames) {
        panic("%s: Unable to create deliveredFrames queue!", __func__);
    }

    provider->processed_frames = g_queue_new();
    if (!provider->processed_frames) {
        panic("%s: Unable to create processedFrames queue!", __func__);
    }

    create_stream(provider, w, h);

    return provider;
}

void destroy_img_provider(img_provider_t* provider) {
    if (!provider) {
        syslog(LOG_ERR, "%s: Invalid pointer to ImgProvider", __func__);
        return;
    }

    release_vdo_buffers(provider);

    pthread_mutex_destroy(&provider->frame_mutex);
    pthread_cond_destroy(&provider->frame_deliver_cond);

    g_queue_free(provider->delivered_frames);
    g_queue_free(provider->processed_frames);

    free(provider);
}

void allocate_vdo_buffers(img_provider_t* provider, VdoStream* vdo_stream) {
    GError* error = NULL;

    assert(provider);
    assert(vdo_stream);

    for (size_t i = 0; i < NUM_VDO_BUFFERS; i++) {
        provider->vdo_buffers[i] = vdo_stream_buffer_alloc(vdo_stream, NULL, &error);
        if (provider->vdo_buffers[i] == NULL) {
            panic("%s: Failed creating VDO buffer: %s",
                  __func__,
                  (error != NULL) ? error->message : "N/A");
        }

        // Make a 'speculative' vdo_buffer_get_data() call to trigger a
        // memory mapping of the buffer. The mapping is cached in the VDO
        // implementation.
        void* dummyPtr = vdo_buffer_get_data(provider->vdo_buffers[i]);
        if (!dummyPtr) {
            panic("%s: Failed initializing buffer memmap: %s",
                  __func__,
                  (error != NULL) ? error->message : "N/A");
        }

        if (!vdo_stream_buffer_enqueue(vdo_stream, provider->vdo_buffers[i], &error)) {
            panic("%s: Failed enqueue VDO buffer: %s",
                  __func__,
                  (error != NULL) ? error->message : "N/A");
        }
    }

    g_clear_error(&error);
}

void choose_stream_resolution(unsigned int req_width,
                              unsigned int req_height,
                              unsigned int* chosen_width,
                              unsigned int* chosen_height) {
    VdoResolutionSet* set = NULL;
    VdoChannel* channel   = NULL;
    GError* error         = NULL;

    assert(chosen_width);
    assert(chosen_height);

    channel = vdo_channel_get(VDO_CHANNEL, &error);
    if (!channel) {
        panic("%s: Failed vdo_channel_get(): %s",
              __func__,
              (error != NULL) ? error->message : "N/A");
    }

    // Only retrieve resolutions with native aspect ratio
    VdoMap* map = vdo_map_new();
    vdo_map_set_string(map, "aspect_ratio", "native");

    // Retrieve channel resolutions
    set = vdo_channel_get_resolutions(channel, map, &error);
    if (!set) {
        panic("%s: Failed vdo_channel_get_resolutions(): %s",
              __func__,
              (error != NULL) ? error->message : "N/A");
    }

    // Find smallest VDO stream resolution that fits the requested size.
    ssize_t best_resolution_idx       = -1;
    unsigned int best_resolution_area = UINT_MAX;
    for (size_t i = 0; i < set->count; ++i) {
        VdoResolution* res = &set->resolutions[i];
        if ((res->width >= req_width) && (res->height >= req_height)) {
            unsigned int area = res->width * res->height;
            if (area < best_resolution_area) {
                best_resolution_idx  = i;
                best_resolution_area = area;
            }
        }
    }

    // If we got a reasonable w/h from the VDO channel info we use that
    // for creating the stream. If that info for some reason was empty we
    // fall back to trying to create a stream with client-supplied w/h.
    *chosen_width  = req_width;
    *chosen_height = req_height;
    if (best_resolution_idx >= 0) {
        *chosen_width  = set->resolutions[best_resolution_idx].width;
        *chosen_height = set->resolutions[best_resolution_idx].height;
        syslog(LOG_INFO,
               "%s: We select stream w/h=%u x %u based on VDO channel info.\n",
               __func__,
               *chosen_width,
               *chosen_height);
    } else {
        syslog(LOG_WARNING,
               "%s: VDO channel info contains no reslution info. Fallback "
               "to client-requested stream resolution.",
               __func__);
    }

    g_object_unref(map);
    g_clear_object(&channel);
    g_free(set);
    g_clear_error(&error);
}

void create_stream(img_provider_t* provider, unsigned int w, unsigned int h) {
    VdoMap* vdo_map = vdo_map_new();
    GError* error   = NULL;

    if (!vdo_map) {
        panic("%s: Failed to create vdo_map", __func__);
    }

    vdo_map_set_uint32(vdo_map, "channel", VDO_CHANNEL);
    vdo_map_set_uint32(vdo_map, "format", provider->vdo_format);
    vdo_map_set_uint32(vdo_map, "width", w);
    vdo_map_set_uint32(vdo_map, "height", h);
    // We will use buffer_alloc() and buffer_unref() calls.
    vdo_map_set_uint32(vdo_map, "buffer.strategy", VDO_BUFFER_STRATEGY_EXPLICIT);

    VdoStream* vdo_stream = vdo_stream_new(vdo_map, NULL, &error);
    if (!vdo_stream) {
        panic("%s: Failed creating vdo stream: %s",
              __func__,
              (error != NULL) ? error->message : "N/A");
    }

    allocate_vdo_buffers(provider, vdo_stream);

    // Start the actual VDO streaming.
    if (!vdo_stream_start(vdo_stream, &error)) {
        panic("%s: Failed starting stream: %s", __func__, (error != NULL) ? error->message : "N/A");
    }

    provider->vdo_stream = vdo_stream;

    // Always do this
    g_object_unref(vdo_map);
    g_clear_error(&error);
}

static void release_vdo_buffers(img_provider_t* provider) {
    if (!provider->vdo_stream) {
        return;
    }

    for (size_t i = 0; i < NUM_VDO_BUFFERS; i++) {
        if (provider->vdo_buffers[i] != NULL) {
            vdo_stream_buffer_unref(provider->vdo_stream, &provider->vdo_buffers[i], NULL);
        }
    }
}

VdoBuffer* get_last_frame_blocking(img_provider_t* provider) {
    VdoBuffer* return_buf = NULL;
    pthread_mutex_lock(&provider->frame_mutex);

    while (g_queue_get_length(provider->delivered_frames) < 1) {
        if (pthread_cond_wait(&provider->frame_deliver_cond, &provider->frame_mutex)) {
            panic("%s: Failed to wait on condition: %s", __func__, strerror(errno));
        }
    }

    return_buf = g_queue_pop_tail(provider->delivered_frames);

    pthread_mutex_unlock(&provider->frame_mutex);

    return return_buf;
}

void return_frame(img_provider_t* provider, VdoBuffer* buffer) {
    pthread_mutex_lock(&provider->frame_mutex);

    g_queue_push_tail(provider->processed_frames, buffer);

    pthread_mutex_unlock(&provider->frame_mutex);
}

static void* thread_entry(void* data) {
    GError* error            = NULL;
    img_provider_t* provider = (img_provider_t*)data;

    while (!provider->shut_down) {
        // Block waiting for a frame from VDO
        VdoBuffer* new_buffer = vdo_stream_get_buffer(provider->vdo_stream, &error);

        if (!new_buffer) {
            // Fail but we continue anyway hoping for the best.
            syslog(LOG_WARNING,
                   "%s: Failed fetching frame from vdo: %s",
                   __func__,
                   (error != NULL) ? error->message : "N/A");
            g_clear_error(&error);
            continue;
        }
        pthread_mutex_lock(&provider->frame_mutex);

        g_queue_push_tail(provider->delivered_frames, new_buffer);

        VdoBuffer* old_buffer = NULL;

        // First check if there are any frames returned from app
        // processing
        if (g_queue_get_length(provider->processed_frames) > 0) {
            old_buffer = g_queue_pop_head(provider->processed_frames);
        } else {
            // Client specifies the number-of-recent-frames it needs to collect
            // in one chunk (numAppFrames). Thus only enqueue buffers back to
            // VDO if we have collected more buffers than numAppFrames.
            if (g_queue_get_length(provider->delivered_frames) > provider->num_app_frames) {
                old_buffer = g_queue_pop_head(provider->delivered_frames);
            }
        }

        if (old_buffer) {
            if (!vdo_stream_buffer_enqueue(provider->vdo_stream, old_buffer, &error)) {
                // Fail but we continue anyway hoping for the best.
                syslog(LOG_WARNING,
                       "%s: Failed enqueueing buffer to vdo: %s",
                       __func__,
                       (error != NULL) ? error->message : "N/A");
                g_clear_error(&error);
            }
        }
        g_object_unref(new_buffer);  // Release the ref from vdo_stream_get_buffer
        pthread_cond_signal(&provider->frame_deliver_cond);
        pthread_mutex_unlock(&provider->frame_mutex);
    }

    return provider;
}

void start_frame_fetch(img_provider_t* provider) {
    if (pthread_create(&provider->fetcher_thread, NULL, thread_entry, provider)) {
        panic("%s: Failed to start thread fetching frames from vdo: %s", __func__, strerror(errno));
    }
}

void stop_frame_fetch(img_provider_t* provider) {
    provider->shut_down = true;

    if (pthread_join(provider->fetcher_thread, NULL)) {
        panic("%s: Failed to join thread fetching frames from vdo: %s", __func__, strerror(errno));
    }
}

guint32 get_stream_rotation(img_provider_t* provider) {
    GError* error = NULL;

    g_autoptr(VdoMap) info = vdo_stream_get_info(provider->vdo_stream, &error);
    if (error) {
        panic("%s: Could not get stream info: %s", __func__, error->message);
    }

    return vdo_map_get_uint32(info, "rotation", 0);
}