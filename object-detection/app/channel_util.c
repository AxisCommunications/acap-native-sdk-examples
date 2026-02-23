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
 * This file handles the vdo channel part of the application.
 */

#include "channel_util.h"

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

bool channel_util_choose_stream_resolution(unsigned int channel_id,
                                           VdoResolution req_res,
                                           VdoResolution* chosen_req,
                                           unsigned int rotation,
                                           VdoFormat* chosen_format) {
    g_autoptr(VdoResolutionSet) set     = NULL;
    g_autoptr(VdoChannel) channel       = NULL;
    g_autoptr(GError) error             = NULL;
    g_autoptr(VdoMap) resolution_filter = vdo_map_new();

    assert(chosen_format);
    assert(chosen_req);

    channel = vdo_channel_get(channel_id, &error);
    if (!channel) {
        panic("%s: Failed vdo_channel_get(): %s", __func__, error->message);
    }

    *chosen_req = req_res;

    if (rotation == 90 || rotation == 270) {
        // To be able to get the wanted resolution the resolution
        // needs to be unrotated then vdo will supply frames that have
        // the resolution img_info->width x img_info->height
        unsigned int tmp_width = req_res.width;
        chosen_req->width      = req_res.height;
        chosen_req->height     = tmp_width;
    }

    // Start to see if the supplied image format is available on this
    // product. If not default to yuv
    vdo_map_set_uint32(resolution_filter, "format", *chosen_format);
    vdo_map_set_string(resolution_filter, "select", "all");
    vdo_map_set_string(resolution_filter, "aspect_ratio", "native");

    set = vdo_channel_get_resolutions(channel, resolution_filter, &error);
    if (!set || set->count == 0) {
        // The supplied format is not supported, default to YUV
        if (set) {
            free(set);
        }
        if (*chosen_format == VDO_FORMAT_YUV) {
            panic("%s: Not possible to get any resolution from vdo for %u",
                  __func__,
                  *chosen_format);
        }
        *chosen_format = VDO_FORMAT_YUV;
        vdo_map_set_uint32(resolution_filter, "format", *chosen_format);
        set = vdo_channel_get_resolutions(channel, resolution_filter, &error);
        if (!set || set->count == 0) {
            panic("%s: Not possible to get any resolution from vdo for %u",
                  __func__,
                  *chosen_format);
        }
    }

    // Find smallest VDO stream resolution that fits the requested size.
    ssize_t best_resolution_idx       = -1;
    unsigned int best_resolution_area = UINT_MAX;
    for (ssize_t i = 0; i < (ssize_t)set->count; ++i) {
        VdoResolution* res = &set->resolutions[i];
        if ((res->width >= chosen_req->width) && (res->height >= chosen_req->height)) {
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
    if (best_resolution_idx >= 0) {
        chosen_req->width  = set->resolutions[best_resolution_idx].width;
        chosen_req->height = set->resolutions[best_resolution_idx].height;
    } else {
        syslog(LOG_WARNING,
               "%s: VDO channel info contains no reslution info. Fallback "
               "to client-requested stream resolution.",
               __func__);
    }
    const char* format_str = "rgb interleaved";
    switch (*chosen_format) {
        case VDO_FORMAT_YUV:
            format_str = "yuv";
            break;
        case VDO_FORMAT_PLANAR_RGB:
            format_str = "planar rgb";
            break;
        case VDO_FORMAT_RGB:
            format_str = "rgb interleaved";
            break;
        default:
            panic("%s Unknown format %u", __func__, *chosen_format);
    }
    syslog(LOG_INFO,
           "%s: We select stream w/h=%u x %u with format %s based on VDO channel info.\n",
           __func__,
           chosen_req->width,
           chosen_req->height,
           format_str);

    return true;
}

unsigned int channel_util_get_image_rotation(unsigned int channel_id) {
    g_autoptr(VdoChannel) channel = NULL;
    g_autoptr(GError) error       = NULL;

    channel = vdo_channel_get(channel_id, &error);
    if (!channel) {
        panic("%s: Failed vdo_channel_get() for %u: %s", __func__, channel_id, error->message);
    }
    g_autoptr(VdoMap) info = vdo_channel_get_info(channel, &error);
    if (!info) {
        panic("%s: Failed vdo_channel_get_info(): %s", __func__, error->message);
    }
    return vdo_map_get_uint32(info, "rotation", 0);
}

unsigned int channel_util_get_first_input_channel(void) {
    g_autoptr(VdoChannel) channel = NULL;
    g_autoptr(GError) error       = NULL;
    g_autoptr(VdoMap) ch_desc     = vdo_map_new();

    // Take the first input channel
    vdo_map_set_uint32(ch_desc, "input", 1);
    channel = vdo_channel_get_ex(ch_desc, &error);
    if (!channel) {
        panic("%s: Failed vdo_channel_get(): %s", __func__, error->message);
    }
    g_autoptr(VdoMap) info = vdo_channel_get_info(channel, &error);
    if (!info) {
        panic("%s: Failed vdo_channel_get_info(): %s", __func__, error->message);
    }
    return vdo_map_get_uint32(info, "id", 1);
}

VdoPair32u channel_util_get_aspect_ratio(unsigned int channel_id) {
    g_autoptr(VdoChannel) channel = NULL;
    g_autoptr(GError) error       = NULL;
    VdoPair32u aspect_ratio_def   = {.w = 0u, .h = 0u};

    // Take the first input channel
    channel = vdo_channel_get(channel_id, &error);
    if (!channel) {
        panic("%s: Failed vdo_channel_get(): %s", __func__, error->message);
    }
    g_autoptr(VdoMap) info = vdo_channel_get_info(channel, &error);
    if (!info) {
        panic("%s: Failed vdo_channel_get_info(): %s", __func__, error->message);
    }
    return vdo_map_get_pair32u(info, "aspect_ratio", aspect_ratio_def);
}
