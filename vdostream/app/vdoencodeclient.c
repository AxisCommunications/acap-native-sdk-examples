/**
 * Copyright (C) 2019-2021, Axis Communications AB, Lund, Sweden
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
 * - vdoencodeclient -
 *
 * This application is a basic vdo type of application.
 *
 * The application starts a vdo steam and then illustrates how to continuously
 * capture frames from the vdo service, access the received buffer contents
 * as well as the frame metadata.
 *
 * The application expects three arguments on the command line in the following
 * order: format frames output.
 *
 * First argument, format, is a string describing the video compression format.
 * Possible values are avif, h264 (default), h265, jpeg, nv12, and y800.
 *
 * Second argument, frames, is an integer for number of captured frames.
 *
 * Finally, the third argument, output, is the output filename.
 *
 * Suppose that you have done through the steps of installation.
 * Then you would go to /usr/local/packages/vdoencodeclient on your device
 * and then for example run:
 *     ./vdoencodeclient \
 *         --format h264 \
 *         --frames 10 \
 *         --output vdo.out
 *
 * or in short argument syntax
 *      ./vdoencodeclient \
 *         -t h264 \
 *         -n 10 \
 *         -o vdo.out
 */

#include "vdo-error.h"
#include "vdo-map.h"
#include "vdo-stream.h"
#include "vdo-types.h"

#include <glib.h>
#include <glib/gstdio.h>
// Needed for g_autoptr
#include <glib-object.h>
#include <signal.h>
#include <stdlib.h>
#include <syslog.h>

#include "panic.h"

static gboolean shutdown       = FALSE;
static const gchar* param_desc = "";
static const gchar* summary    = "Encoded video client";

// Facilitate graceful shutdown with CTRL-C
static void handle_sigint(int signum) {
    (void)signum;
    shutdown = TRUE;
}

// Determine and log the received frame type
static void print_frame(VdoFrame* frame) {
    gchar* frame_type;
    switch (vdo_frame_get_frame_type(frame)) {
        case VDO_FRAME_TYPE_AVIF:
            frame_type = "avif";
            break;
        case VDO_FRAME_TYPE_H264_IDR:
        case VDO_FRAME_TYPE_H265_IDR:
        case VDO_FRAME_TYPE_H264_I:
        case VDO_FRAME_TYPE_H265_I:
            frame_type = "I";
            break;
        case VDO_FRAME_TYPE_H264_P:
        case VDO_FRAME_TYPE_H265_P:
            frame_type = "P";
            break;
        case VDO_FRAME_TYPE_JPEG:
            frame_type = "jpeg";
            break;
        case VDO_FRAME_TYPE_YUV:
            frame_type = "yuv";
            break;
        default:
            frame_type = "NA";
    }

    syslog(LOG_INFO,
           "frame = %4u, type = %s, size = %zu\n",
           vdo_frame_get_sequence_nbr(frame),
           frame_type,
           vdo_frame_get_size(frame));
}

// Set vdo format from input parameter
static void set_format(VdoMap* settings, gchar* format) {
    if (g_strcmp0(format, "avif") == 0) {
        vdo_map_set_uint32(settings, "format", VDO_FORMAT_AVIF);
    } else if (g_strcmp0(format, "h264") == 0) {
        vdo_map_set_uint32(settings, "format", VDO_FORMAT_H264);
    } else if (g_strcmp0(format, "h265") == 0) {
        vdo_map_set_uint32(settings, "format", VDO_FORMAT_H265);
    } else if (g_strcmp0(format, "jpeg") == 0) {
        vdo_map_set_uint32(settings, "format", VDO_FORMAT_JPEG);
    } else if (g_strcmp0(format, "nv12") == 0) {
        vdo_map_set_uint32(settings, "format", VDO_FORMAT_YUV);
        vdo_map_set_string(settings, "subformat", "NV12");
    } else if (g_strcmp0(format, "y800") == 0) {
        vdo_map_set_uint32(settings, "format", VDO_FORMAT_YUV);
        vdo_map_set_string(settings, "subformat", "Y800");
    } else {
        panic("%s: Format \"%s\" is not supported\n", __func__, format);
    }
}

static void save_frame_to_file(VdoBuffer* buffer, FILE* dest_f) {
    // Lifetimes of buffer and frame are linked, no need to free frame
    VdoFrame* frame = vdo_buffer_get_frame(buffer);

    print_frame(frame);

    gpointer data = vdo_buffer_get_data(buffer);
    if (!data)
        panic("%s: Failed to get data: %m", __func__);

    if (!fwrite(data, vdo_frame_get_size(frame), 1, dest_f))
        panic("%s: Failed to write frame: %m", __func__);
}

static int handle_vdo_failed(GError* error) {
    // Maintenance/Installation in progress (e.g. Global-Rotation)
    if (vdo_error_is_expected(&error)) {
        syslog(LOG_INFO, "Expected vdo error %s", error->message);
        return EXIT_SUCCESS;
    } else {
        panic("Unexpected vdo error %s", error->message);
    }
    return EXIT_FAILURE;
}

/**
 * Main function that starts a stream with the following options:
 *
 * --format [avif, h264, h265, jpeg, nv12, y800]
 * --frames [number of frames]
 * --output [output filename]
 */
int main(int argc, char* argv[]) {
    g_autoptr(GError) error     = NULL;
    g_autoptr(VdoMap) settings  = NULL;
    g_autoptr(VdoMap) info      = NULL;
    g_autoptr(VdoStream) stream = NULL;
    gchar* format               = "h264";
    guint frames                = G_MAXUINT;
    gchar* output_file          = "/dev/null";
    FILE* dest_f                = NULL;

    GOptionEntry options[] = {
        {"format",
         't',
         0,
         G_OPTION_ARG_STRING,
         &format,
         "format (avif, h264, h265, jpeg, nv12, y800)",
         NULL},
        {"frames", 'n', 0, G_OPTION_ARG_INT, &frames, "number of frames", NULL},
        {"output", 'o', 0, G_OPTION_ARG_FILENAME, &output_file, "output filename", NULL},
        {
            NULL,
            0,
            0,
            0,
            NULL,
            NULL,
            NULL,
        }};

    GOptionContext* context = g_option_context_new(param_desc);
    if (!context)
        panic("%s Context is null", __func__);

    g_option_context_set_summary(context, summary);
    g_option_context_add_main_entries(context, options, NULL);
    if (!g_option_context_parse(context, &argc, &argv, &error))
        panic("%s Failed to use option_context_parse: %s", __func__, error->message);

    dest_f = fopen(output_file, "wb");
    if (!dest_f)
        panic("%s open failed: %m", __func__);

    if (signal(SIGINT, handle_sigint) == SIG_ERR)
        panic("%s Failed to install signal handler: %m", __func__);

    settings = vdo_map_new();
    set_format(settings, format);

    // Set default arguments
    VdoPair32u resolution = {
        .w = 640,
        .h = 360,
    };
    vdo_map_set_pair32u(settings, "resolution", resolution);

    // Use snapshot API when nbr of frames are 1
    if (frames == 1) {
        g_autoptr(VdoBuffer) buffer = vdo_stream_snapshot(settings, &error);
        if (!buffer)
            panic("%s: Failed to get snapshot: %s", __func__, error->message);
        syslog(LOG_INFO,
               "Starting stream: %s, %ux%u, 1 fps\n",
               format,
               vdo_map_get_uint32(settings, "width", 0),
               vdo_map_get_uint32(settings, "height", 0));
        save_frame_to_file(buffer, dest_f);
        goto exit;
    }

    // When several frames should be retrieved
    // Not to be used for AVIF
    if (g_strcmp0(format, "avif") == 0)
        panic("AVIF should not be used for more frames than one");

    // Create a new stream
    stream = vdo_stream_new(settings, NULL, &error);
    if (!stream)
        panic("%s: Failed creating vdo stream: %s", __func__, error->message);

    info = vdo_stream_get_info(stream, &error);
    if (!info)
        panic("%s: Failed to get vdo stream info: %s", __func__, error->message);

    syslog(LOG_INFO,
           "Starting stream: %s, %ux%u, %u fps\n",
           format,
           vdo_map_get_uint32(info, "width", 0),
           vdo_map_get_uint32(info, "height", 0),
           (unsigned int)(vdo_map_get_double(info, "framerate", 0.0) + 0.5));

    // Start the stream
    if (!vdo_stream_start(stream, &error))
        panic("%s: Failed to start vdo stream : %s", __func__, error->message);

    // Loop until interrupt by Ctrl-C or reaching G_MAXUINT
    for (guint n = 0; n < frames; ++n) {
        // SIGINT occurred
        if (shutdown)
            goto exit;

        g_autoptr(VdoBuffer) buffer = vdo_stream_get_buffer(stream, &error);
        if (!buffer && g_error_matches(error, VDO_ERROR, VDO_ERROR_NO_DATA)) {
            g_clear_error(&error);
            continue;  // Transient error -> Retry
        }

        if (!buffer)
            return handle_vdo_failed(error);

        save_frame_to_file(buffer, dest_f);

        // Release the buffer and allow the server to reuse it
        if (!vdo_stream_buffer_unref(stream, &buffer, &error)) {
            if (!vdo_error_is_expected(&error))
                panic("%s: Unexpected error: %s", __func__, error->message);
            g_clear_error(&error);
        }
    }

exit:
    if (dest_f)
        fclose(dest_f);

    g_option_context_free(context);

    syslog(LOG_INFO, "Exit %s", argv[0]);
    return 0;
}
