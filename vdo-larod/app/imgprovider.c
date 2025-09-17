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
 * This file handles the vdo part of the application.
 */

#include "imgprovider.h"

#include <assert.h>
#include <errno.h>
#include <glib-object.h>
#include <gmodule.h>
#include <math.h>
#include <poll.h>
#include <syslog.h>

#include "panic.h"
#include "vdo-map.h"
#include <vdo-channel.h>
#include <vdo-error.h>

G_DEFINE_AUTOPTR_CLEANUP_FUNC(VdoResolutionSet, g_free);

#define IMG_PROVIDER_ANALYSIS_MAX (10)

#define MIN_SIZE 64

/**
 * @brief Calculate a new img provider framerate based on inference time
 *
 * @param provider        The img provider to be used
 * @param analysis_time   Time in ms for the analysis
 *
 * @return void
 */
static void calculate_new_framerate(img_provider_t* provider, unsigned int analysis_time) {
    if (analysis_time > 201) {
        provider->img_info->framerate = 1.0;
        provider->frametime           = 1001;
        return;
    }
    if (analysis_time < 34) {
        provider->img_info->framerate = 30.0;
        provider->frametime           = 34;
    } else if (analysis_time < 41) {
        provider->img_info->framerate = 25.0;
        provider->frametime           = 41;
    } else if (analysis_time < 51) {
        provider->img_info->framerate = 20.0;
        provider->frametime           = 51;
    } else if (analysis_time < 67) {
        provider->img_info->framerate = 15.0;
        provider->frametime           = 67;
    } else if (analysis_time < 101) {
        provider->img_info->framerate = 10.0;
        provider->frametime           = 101;
    } else if (analysis_time < 201) {
        provider->img_info->framerate = 5.0;
        provider->frametime           = 201;
    }
    if (provider->img_info->framerate > provider->wanted_framerate) {
        provider->img_info->framerate = provider->wanted_framerate;
    }
}

static void update_framerate(img_provider_t* provider, unsigned analysis_time) {
    g_autoptr(GError) error    = NULL;
    unsigned int old_frametime = provider->frametime;

    calculate_new_framerate(provider, analysis_time);
    if (old_frametime != provider->frametime) {
        if (!vdo_stream_set_framerate(provider->vdo_stream,
                                      provider->img_info->framerate,
                                      &error)) {
            panic("%s: Failed to change framerate: %s", __func__, error->message);
        }
        syslog(LOG_INFO,
               "Change VDO stream framerate to %f because of the mean analysis time %u ms",
               provider->img_info->framerate,
               analysis_time);
        // Flush all frames in vdo so the latest is used
        img_provider_flush_all_frames(provider);
    }
}

static bool choose_stream_resolution(unsigned int input_channel,
                                     img_info_t* img_info,
                                     const char* image_fit,
                                     unsigned int* chosen_width,
                                     unsigned int* chosen_height,
                                     VdoFormat* format) {
    g_autoptr(VdoResolutionSet) set     = NULL;
    g_autoptr(VdoChannel) channel       = NULL;
    g_autoptr(GError) error             = NULL;
    g_autoptr(VdoMap) resolution_filter = vdo_map_new();
    const char* select                  = "all";
    const char* aspect_ratio            = "native";
    bool ambarella_workaround           = false;

    *format = img_info->format;

    assert(chosen_width);
    assert(chosen_height);

    g_autoptr(VdoMap) ch_desc = vdo_map_new();
    vdo_map_set_uint32(ch_desc, "input", input_channel);
    channel = vdo_channel_get_ex(ch_desc, &error);
    if (!channel) {
        panic("%s: Failed vdo_channel_get(): %s", __func__, error->message);
    }

    // Only ambarella based cameras have PLANAR RGB as model input
    if (*format == VDO_FORMAT_PLANAR_RGB) {
        vdo_map_set_uint32(resolution_filter, "format", VDO_FORMAT_YUV);
        vdo_map_set_string(resolution_filter, "select", "minmax");
        set = vdo_channel_get_resolutions(channel, resolution_filter, &error);
        if (!set || set->count == 0) {
            panic("%s: Not possible to get any resolution from vdo for %u", __func__, *format);
        }
        // The minimum width will be 64 on 12.6 and later
        if (set->resolutions[0].width > MIN_SIZE) {
            ambarella_workaround = true;
            *format              = VDO_FORMAT_YUV;
        }
    }

    // Start to see if the supplied image format is available on this
    // product. If not default to yuv
    vdo_map_set_uint32(resolution_filter, "format", *format);
    if (!g_strcmp0(image_fit, "crop")) {
        aspect_ratio = NULL;
        if (!ambarella_workaround) {
            // Try to get the choosen resolution from vdo
            // The only limits is the min and max resolution
            select = "minmax";
        } else {
            select = "all";
        }
    }
    vdo_map_set_string(resolution_filter, "select", select);
    if (aspect_ratio) {
        vdo_map_set_string(resolution_filter, "aspect_ratio", aspect_ratio);
    }
    set = vdo_channel_get_resolutions(channel, resolution_filter, &error);
    if (!set || set->count == 0) {
        // The supplied format is not supported, default to YUV
        if (set) {
            free(set);
        }
        if (*format == VDO_FORMAT_YUV) {
            panic("%s: Not possible to get any resolution from vdo for %u", __func__, *format);
        }
        *format = VDO_FORMAT_YUV;
        vdo_map_set_uint32(resolution_filter, "format", *format);
        set = vdo_channel_get_resolutions(channel, resolution_filter, &error);
        if (!set || set->count == 0) {
            panic("%s: Not possible to get any resolution from vdo for %u", __func__, *format);
        }
    }

    // For all try to find the closest match or an exact match
    if (!g_strcmp0(select, "all")) {
        // Find smallest VDO stream resolution that fits the requested size.
        ssize_t best_resolution_idx       = -1;
        unsigned int best_resolution_area = UINT_MAX;
        for (ssize_t i = 0; i < (ssize_t)set->count; ++i) {
            VdoResolution* res = &set->resolutions[i];
            if ((res->width >= img_info->width) && (res->height >= img_info->height)) {
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
        *chosen_width  = img_info->width;
        *chosen_height = img_info->height;
        if (best_resolution_idx >= 0) {
            *chosen_width  = set->resolutions[best_resolution_idx].width;
            *chosen_height = set->resolutions[best_resolution_idx].height;
        } else {
            syslog(LOG_WARNING,
                   "%s: VDO channel info contains no reslution info. Fallback "
                   "to client-requested stream resolution.",
                   __func__);
        }
    } else {
        // Check towards min and max.
        *chosen_width  = img_info->width;
        *chosen_height = img_info->height;

        // Check the requested width and height towards max resolution
        if (img_info->width > set->resolutions[1].width ||
            img_info->height > set->resolutions[1].height) {
            *chosen_width  = set->resolutions[1].width;
            *chosen_height = set->resolutions[1].height;
            syslog(LOG_WARNING,
                   "%s: Requested width or height larger than max resolution."
                   "Limit the requested resolution to max %ux%u.",
                   __func__,
                   set->resolutions[1].width,
                   set->resolutions[1].height);
        }
        // Check the requested width and height towards min resolution
        if (img_info->width < set->resolutions[0].width ||
            img_info->height < set->resolutions[0].height) {
            *chosen_width  = set->resolutions[0].width;
            *chosen_height = set->resolutions[0].height;
            syslog(LOG_WARNING,
                   "%s: Requested width or height smaller than min resolution."
                   "Limit the requested resolution to min %u x %u.",
                   __func__,
                   set->resolutions[0].width,
                   set->resolutions[0].height);
        }
    }
    const char* format_str = "rgb interleaved";
    if (*format == VDO_FORMAT_YUV) {
        format_str = "yuv";
    } else if (*format == VDO_FORMAT_PLANAR_RGB) {
        format_str = "planar rgb";
    }
    syslog(LOG_INFO,
           "%s: We select stream w/h=%u x %u with format %s based on VDO channel info.\n",
           __func__,
           *chosen_width,
           *chosen_height,
           format_str);

    return true;
}

img_info_t img_provider_get_image_metadata(img_provider_t* provider) {
    return *provider->img_info;
}

img_provider_t* img_provider_new(unsigned int input_channel,
                                 img_info_t* img_info,
                                 unsigned int num_buffers,
                                 double framerate,
                                 char* image_fit) {
    g_autoptr(VdoMap) vdo_settings = vdo_map_new();
    g_autoptr(GError) error        = NULL;
    unsigned int chosen_width      = 0;
    unsigned int chosen_height     = 0;

    img_provider_t* provider = (img_provider_t*)calloc(1, sizeof(img_provider_t));
    if (!provider) {
        panic("%s: Unable to allocate ImgProvider: %s", __func__, strerror(errno));
    }

    provider->buffer_count = num_buffers;
    provider->vdo_stream   = NULL;
    provider->fd           = -1;

    if (!vdo_settings) {
        panic("%s: Failed to create vdo_map", __func__);
    }

    // Start to get the best match for the provided img_info
    if (!choose_stream_resolution(input_channel,
                                  img_info,
                                  image_fit,
                                  &chosen_width,
                                  &chosen_height,
                                  &img_info->format)) {
        panic("%s:Failed to choose stream resolution", __func__);
    }

    vdo_map_set_uint32(vdo_settings, "input", input_channel);

    // format is the image format that is supplied from vdo
    vdo_map_set_uint32(vdo_settings, "format", img_info->format);
    // Set initial framerate
    vdo_map_set_double(vdo_settings, "framerate", framerate);
    vdo_map_set_uint32(vdo_settings, "width", chosen_width);
    vdo_map_set_uint32(vdo_settings, "height", chosen_height);
    // Make it possible to change the framerate for the stream after it is started
    vdo_map_set_boolean(vdo_settings, "dynamic.framerate", true);
    // It is not needed to set buffer.strategy since VDO_BUFFER_STRATEGY_INFINITE is default
    // vdo_map_set_uint32(vdo_settings, "buffer.strategy", VDO_BUFFER_STRATEGY_INFINITE);

    // The number of buffers that vdo will allocate for this stream
    // Normally two buffers are enough and using too many buffers will use
    // more memory in the product.
    vdo_map_set_uint32(vdo_settings, "buffer.count", provider->buffer_count);

    // The vdo_stream_get_buffer is non blocking and will return immediately
    // Then we need to poll instead when it is ok to get a buffer
    vdo_map_set_boolean(vdo_settings, "socket.blocking", false);

    syslog(LOG_INFO, "Dump of vdo stream settings map =====");
    vdo_map_dump(vdo_settings);

    // Create a vdo stream using the vdoMap filled in above
    g_autoptr(VdoStream) vdo_stream = vdo_stream_new(vdo_settings, NULL, &error);
    if (!vdo_stream) {
        panic("%s: Failed creating vdo stream: %s", __func__, error->message);
    }

    // Get the info map from the vdo stream.
    // This will contain values how the stream was actually created and may
    // differ from the settings map used above
    // The most useful is width/height and pitch since these values will follow rotation
    // and will be the resolution that the buffers from vdo have.
    g_autoptr(VdoMap) vdo_info = vdo_stream_get_info(vdo_stream, &error);
    if (!vdo_info) {
        panic("%s: Failed to get info map for stream: %s", __func__, error->message);
    }

    provider->img_info = (img_info_t*)calloc(1, sizeof(img_info_t));
    if (!provider->img_info) {
        panic("%s: Unable to allocate img info: %s", __func__, strerror(errno));
    }
    provider->img_info->height = vdo_map_get_uint32(vdo_info, "height", chosen_height);
    provider->img_info->width  = vdo_map_get_uint32(vdo_info, "width", chosen_width);
    provider->img_info->pitch  = vdo_map_get_uint32(vdo_info, "pitch", provider->img_info->width);
    provider->img_info->format = vdo_map_get_uint32(vdo_info, "format", img_info->format);
    provider->img_info->framerate = vdo_map_get_double(vdo_info, "framerate", framerate);
    provider->img_info->rotation  = vdo_map_get_uint32(vdo_info, "rotation", 0);
    provider->channel             = vdo_map_get_uint32(vdo_info, "channel", 0);
    provider->wanted_framerate    = framerate;

    // Calculate the time between the images from vdo
    provider->frametime            = (unsigned int)((1 / provider->img_info->framerate) * 1000);
    provider->mean_analysis_time   = 0;
    provider->analysis_frame_count = 0;
    provider->tot_analysis_time    = 0;

    provider->vdo_stream = g_steal_pointer(&vdo_stream);

    return provider;
}

void img_provider_destroy(img_provider_t* provider) {
    assert(provider);

    g_clear_object(&provider->vdo_stream);

    free(provider->img_info);
    free(provider);
}

bool img_provider_update_framerate(img_provider_t* provider, unsigned analysis_time) {
    assert(provider);

    provider->analysis_frame_count++;
    provider->tot_analysis_time += analysis_time;
    if (provider->analysis_frame_count == IMG_PROVIDER_ANALYSIS_MAX) {
        provider->mean_analysis_time = provider->tot_analysis_time / provider->analysis_frame_count;

        // If the analysis time is higher or lower than the time between frames from
        // vdo change the framerate so the latest frame will be fetched from vdo
        if (provider->frametime < provider->mean_analysis_time && provider->frametime < 201) {
            update_framerate(provider, provider->mean_analysis_time);
        } else if (provider->frametime > provider->mean_analysis_time) {
            update_framerate(provider, provider->mean_analysis_time);
        }
        provider->mean_analysis_time   = 0;
        provider->analysis_frame_count = 0;
        provider->tot_analysis_time    = 0;
    }

    return true;
}

bool img_provider_start(img_provider_t* provider) {
    g_autoptr(GError) error = NULL;
    assert(provider);

    // Start the actual VDO streaming.
    // The internal buffers will then be filled at the framerate set to vdo
    // or if default the capture frequency
    if (!vdo_stream_start(provider->vdo_stream, &error)) {
        panic("%s: Failed to start stream: %s", __func__, error->message);
    }

    // Get the stream fd from vdo to be used for polling
    int fd = vdo_stream_get_fd(provider->vdo_stream, &error);
    if (fd < 0) {
        panic("%s: Failed to get fd for stream: %s", __func__, error->message);
    }
    provider->fd = fd;
    return true;
}

VdoBuffer* img_provider_get_frame(img_provider_t* provider) {
    g_autoptr(GError) error = NULL;
    assert(provider);

    struct pollfd fds = {
        .fd     = provider->fd,
        .events = POLL_IN,
    };
    while (true) {
        int status = 0;
        do {
            // If poll returns -1 then errno is set
            // if the errno is set to EINTR then just
            // continue this loop
            status = poll(&fds, 1, -1);
        } while (status == -1 && errno == EINTR);

        if (status < 0) {
            panic("%s: Failed to poll fd: %s", __func__, strerror(errno));
        }

        // Get video frame from the imaging pipeline
        // If the inference time is too long this may not be the latest buffer since
        // vdo will fill up its internal buffers and give out the oldest one
        g_autoptr(VdoBuffer) vdo_buf = vdo_stream_get_buffer(provider->vdo_stream, &error);
        if (!vdo_buf) {
            if (g_error_matches(error, VDO_ERROR, VDO_ERROR_NO_DATA)) {
                g_clear_object(&error);
                continue;  // Transient Error -> Retry
            }
            // Maintenance/Installation in progress (e.g Global Rotation)
            if (vdo_error_is_expected(&error)) {
                syslog(LOG_INFO, "Likely global rotation: %s", error->message);
                return NULL;
            } else {
                panic("%s: Unexpexted error: %s", __func__, error->message);
            }
        }
        return g_steal_pointer(&vdo_buf);
    }
}

void img_provider_flush_all_frames(img_provider_t* provider) {
    g_autoptr(GError) error = NULL;
    assert(provider);
    // Read out all buffers from vdo
    while (true) {
        // Since this call will be non blocking it will return immediately
        g_autoptr(VdoBuffer) read_vdo_buf = vdo_stream_get_buffer(provider->vdo_stream, NULL);
        // if readVdoBuf is NULL it means that all buffers have been fetched from vdo
        if (!read_vdo_buf) {
            break;
        }
        if (!vdo_stream_buffer_unref(provider->vdo_stream, &read_vdo_buf, &error)) {
            if (!vdo_error_is_expected(&error)) {
                panic("%s: Unexpexted error: %s", __func__, error->message);
            }
        }
    }
}
