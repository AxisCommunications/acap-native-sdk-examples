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
 * This file handles the framerate calculation part of the application.
 */

#include "img_util.h"

#include <assert.h>
#include <errno.h>
#include <glib-object.h>
#include <gmodule.h>
#include <math.h>
#include <poll.h>
#include <syslog.h>

#include "panic.h"
#include "vdo-map.h"
#include "vdo-stream.h"
#include <vdo-error.h>

#define IMG_PROVIDER_ANALYSIS_MAX (10)

/**
 * @brief Calculate a new img provider framerate based on inference time
 *
 * @param img_framerate   Struct for the framerate calculations
 * @param analysis_time   Time in ms for the analysis
 *
 */
static void calculate_new_framerate(img_framerate_t* img_framerate, unsigned int analysis_time) {
    if (analysis_time > 201) {
        img_framerate->framerate = 1.0;
        img_framerate->frametime = 1001;
        return;
    }
    if (analysis_time < 34) {
        img_framerate->framerate = 30.0;
        img_framerate->frametime = 34;
    } else if (analysis_time < 41) {
        img_framerate->framerate = 25.0;
        img_framerate->frametime = 41;
    } else if (analysis_time < 51) {
        img_framerate->framerate = 20.0;
        img_framerate->frametime = 51;
    } else if (analysis_time < 67) {
        img_framerate->framerate = 15.0;
        img_framerate->frametime = 67;
    } else if (analysis_time < 101) {
        img_framerate->framerate = 10.0;
        img_framerate->frametime = 101;
    } else if (analysis_time < 201) {
        img_framerate->framerate = 5.0;
        img_framerate->frametime = 201;
    }
    if (img_framerate->framerate > img_framerate->wanted_framerate) {
        img_framerate->framerate = img_framerate->wanted_framerate;
    }
}

static bool
update_framerate(VdoStream* stream, img_framerate_t* img_framerate, unsigned analysis_time) {
    g_autoptr(GError) error    = NULL;
    unsigned int old_frametime = img_framerate->frametime;

    calculate_new_framerate(img_framerate, analysis_time);
    if (old_frametime != img_framerate->frametime) {
        if (!vdo_stream_set_framerate(stream, img_framerate->framerate, &error)) {
            panic("%s: Failed to change framerate: %s", __func__, error->message);
        }
        syslog(LOG_INFO,
               "Change VDO stream framerate to %f because of the mean analysis time %u ms",
               img_framerate->framerate,
               analysis_time);
        return true;
    }
    return false;
}

bool img_util_update_framerate(VdoStream* stream,
                               img_framerate_t* img_framerate,
                               unsigned analysis_time) {
    bool ret = false;
    assert(stream);

    img_framerate->analysis_frame_count++;
    img_framerate->tot_analysis_time += analysis_time;
    if (img_framerate->analysis_frame_count == IMG_PROVIDER_ANALYSIS_MAX) {
        img_framerate->mean_analysis_time =
            img_framerate->tot_analysis_time / img_framerate->analysis_frame_count;

        // If the analysis time is higher or lower than the time between frames from
        // vdo change the framerate so the latest frame will be fetched from vdo
        if (img_framerate->frametime < img_framerate->mean_analysis_time &&
            img_framerate->frametime < 201) {
            ret = update_framerate(stream, img_framerate, img_framerate->mean_analysis_time);
        } else if (img_framerate->frametime > img_framerate->mean_analysis_time) {
            ret = update_framerate(stream, img_framerate, img_framerate->mean_analysis_time);
        }
        img_framerate->mean_analysis_time   = 0;
        img_framerate->analysis_frame_count = 0;
        img_framerate->tot_analysis_time    = 0;
    }

    return ret;
}

bool img_util_flush(VdoStream* stream, VdoBuffer** buf, GError** error) {
    // Flush the imaging pipeline
    vdo_stream_stop(stream);
    // Return the buffer
    if (!vdo_stream_buffer_unref(stream, buf, error)) {
        if (!vdo_error_is_expected(error)) {
            panic("%s: Unexpected error: %s", __func__, (*error)->message);
        }
        g_clear_error(error);
    }
    if (!vdo_stream_start(stream, error)) {
        return false;
    }
    return true;
}
