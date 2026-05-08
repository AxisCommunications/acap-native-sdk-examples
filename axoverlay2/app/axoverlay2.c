// Copyright (C) 2026 Axis Communications AB, Lund, Sweden
// Licensed under the MIT License. See LICENSE file for details.

/**
 * - axoverlay2 -
 *
 * This application demonstrates how the use the axoverlay API version 2.0.
 *
 * Here we use the GLib event loop and the Cairo graphics toolkit for demonstation purposes. In
 * your application, you may use any event library and any graphics toolkit.
 */

#include <assert.h>
#include <axoverlay2.h>
#include <cairo/cairo.h>
#include <glib-unix.h>
#include <glib.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <vdo-error.h>
#include <vdo-stream.h>

/* Record of an overlay that we own */
struct overlay {
    int overlay_id;     /* Overlay ID number (from Axoverlay) */
    unsigned stream_id; /* Stream ID number (from VDO) */

    /* The used portion of the overlay in pixels */
    unsigned used_width;
    unsigned used_height;

    /* The total size of the overlay in pixels, including padding */
    unsigned full_width;
    unsigned full_height;

    /* Cairo-related implementation details */
    cairo_surface_t* surface;
};

static void overlay_record_deleter(void* overlay_void);
static int signal_callback(void* userdata);
static int animation_tick_callback(void* userdata);
static int stream_event_callback(GIOChannel* channel, GIOCondition condition, void* userdata);
static void create_overlay(unsigned stream_id, unsigned stream_width, unsigned stream_height);
static void remove_overlay(unsigned stream_id);
static void process_next_frame(struct overlay* overlay);
static void render_frame(struct overlay* overlay, char* target_buffer);

static VdoStream* vdo_event_stream;

/* A table of all overlays we currently own. Key: int stream_id; Value: struct overlay * */
static GHashTable* overlay_table;

/*
 * Parameters for overlay animation. In a real application we would probably use an external data
 * source. Here we use a fixed 30 fps tick to advance our placeholder animation
 */
static unsigned animation_state;
static unsigned tick_period_us = 1000000 / 30;

/* GLib main loop */
static GMainLoop* main_loop;

/* Enable debug logging? */
static const bool debug = false;

int main(void) {
    GError* error           = NULL;
    axo_err* axo_error      = NULL;
    VdoMap* stream_filter   = NULL;
    bool axo_running        = false;
    GIOChannel* vdo_channel = NULL;
    unsigned vdo_watch_id   = 0;
    int ret                 = 0;

    /* Start Axoverlay */
    if (!axo_start(NULL, &axo_error)) {
        syslog(LOG_ERR, "Failed to start Axoverlay: %s", axo_err_get_message(axo_error));
        goto out;
    }

    axo_running = true;

    /* Create overlay table */
    overlay_table =
        g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, overlay_record_deleter);

    /*
     * Set up a GLib main loop for our application. More external event sources may of course also
     * be hooked into the same loop depending on the application's needs.
     */
    main_loop = g_main_loop_new(NULL, FALSE);

    /* Set up a simple timer to advance the overlay animation (period in milliseconds) */
    g_timeout_add(tick_period_us / 1000, animation_tick_callback, NULL);

    /*
     * Stream 0 in VDO is a magic pseudo-stream which is always present in the system. We can use
     * stream 0 to get events about all other streams.
     */
    vdo_event_stream = vdo_stream_get(0, &error);
    if (!vdo_event_stream) {
        syslog(LOG_ERR, "Failed to open vdo stream 0: %s", error->message);
        goto out;
    }

    /*
     * Set up VDO filter to disregard streams that do not want overlays. Drawing overlays for a
     * stream that will not use them is wasteful.
     */
    stream_filter = vdo_map_new();
    vdo_map_set_string(stream_filter, "filter", "overlay");

    if (!vdo_stream_attach(vdo_event_stream, stream_filter, &error)) {
        syslog(LOG_ERR, "Failed to attach filter to vdo stream 0: %s", error->message);
        goto out;
    }

    int stream_event_fd = vdo_stream_get_event_fd(vdo_event_stream, &error);
    if (stream_event_fd < 0) {
        syslog(LOG_ERR, "Failed to get stream 0 event fd: %s", error->message);
        goto out;
    }

    /* Hook VDO stream event fd into our GLib event loop via a GIOChannel */
    vdo_channel  = g_io_channel_unix_new(stream_event_fd);
    vdo_watch_id = g_io_add_watch(vdo_channel,
                                  G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
                                  stream_event_callback,
                                  NULL);
    if (!vdo_watch_id) {
        syslog(LOG_ERR, "Failed to add stream event fd to event loop");
        goto out;
    }

    /* Set up signal handling to gracefully stop the main loop */
    g_unix_signal_add(SIGINT, signal_callback, main_loop);
    g_unix_signal_add(SIGTERM, signal_callback, main_loop);

    /* Enter main loop */
    g_main_loop_run(main_loop);

out:
    if (vdo_watch_id)
        g_source_remove(vdo_watch_id);
    if (vdo_channel)
        g_io_channel_unref(vdo_channel);
    if (axo_running)
        axo_stop(NULL);
    g_clear_object(&vdo_event_stream);
    g_clear_object(&stream_filter);
    g_clear_error(&error);
    if (main_loop)
        g_main_loop_unref(main_loop);
    if (overlay_table)
        g_hash_table_unref(overlay_table);

    return ret;
}

static void overlay_record_deleter(void* overlay_void) {
    struct overlay* overlay = overlay_void;

    cairo_surface_destroy(overlay->surface);
    g_free(overlay);
}

static int signal_callback(void* userdata) {
    (void)userdata;

    g_main_loop_quit(main_loop);
    return G_SOURCE_REMOVE;
}

/*
 * Callback issued at a regular interval. This is a placeholder, representing your application's
 * external data sources that would trigger the overlay to be updated.
 *
 * Returning G_SOURCE_CONTINUE keeps the timer running.
 */
static int animation_tick_callback(void* userdata) {
    (void)userdata;

    /* Move the animation forward to the next frame */
    animation_state++;

    /* Process next frame for each existing overlay */
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, overlay_table);

    void *key, *value;
    while (g_hash_table_iter_next(&iter, &key, &value)) process_next_frame(value);

    return G_SOURCE_CONTINUE;
}

/*
 * Callback issued when VDO tells us there is a new stream event. We are interested in stream
 * connection and disconnection events.
 *
 * Returning G_SOURCE_CONTINUE keeps the watch active; G_SOURCE_REMOVE tears it down.
 */
static int stream_event_callback(GIOChannel* channel, GIOCondition condition, void* userdata) {
    (void)channel;
    (void)userdata;

    GError* error         = NULL;
    VdoMap* vdo_event     = NULL;
    VdoStream* vdo_stream = NULL;
    VdoMap* stream_info   = NULL;
    int ret               = G_SOURCE_CONTINUE;

    if (condition & (G_IO_ERR | G_IO_HUP)) {
        syslog(LOG_ERR, "Connection to vdo was broken, condition=0x%04x", condition);
        g_main_loop_quit(main_loop);
        ret = G_SOURCE_REMOVE;
        goto out;
    }

    vdo_event = vdo_stream_get_event(vdo_event_stream, &error);
    if (!vdo_event) {
        if (g_error_matches(error, VDO_ERROR, VDO_ERROR_NO_EVENT))
            goto out;

        syslog(LOG_ERR, "Failed to get vdo stream event: %s", error->message);
        g_main_loop_quit(main_loop);
        ret = G_SOURCE_REMOVE;
        goto out;
    }

    unsigned event_type = vdo_map_get_uint32(vdo_event, "event", 0);
    unsigned stream_id  = vdo_map_get_uint32(vdo_event, "id", 0);

    if (event_type == VDO_STREAM_EVENT_EXISTING || event_type == VDO_STREAM_EVENT_CREATED) {
        /*
         * A new stream or a stream which already existed at the time our application started. In
         * this example we want to add one overlay to every such stream.
         */
        vdo_stream = vdo_stream_get(stream_id, &error);
        if (!vdo_stream) {
            syslog(LOG_ERR, "Failed to get stream information from vdo: %s", error->message);
            g_main_loop_quit(main_loop);
            ret = G_SOURCE_REMOVE;
            goto out;
        }

        stream_info = vdo_stream_get_info(vdo_stream, NULL);
        if (!stream_info) {
            syslog(LOG_ERR, "Vdo stream is missing info");
            g_main_loop_quit(main_loop);
            ret = G_SOURCE_REMOVE;
            goto out;
        }

        if (debug)
            vdo_map_dump(stream_info);

        unsigned width  = vdo_map_get_uint32(stream_info, "width", 0);
        unsigned height = vdo_map_get_uint32(stream_info, "height", 0);
        if (!width || !height) {
            syslog(LOG_ERR, "Vdo reported invalid stream size %ux%u", width, height);
            g_main_loop_quit(main_loop);
            ret = G_SOURCE_REMOVE;
            goto out;
        }

        create_overlay(stream_id, width, height);
    } else if (event_type == VDO_STREAM_EVENT_CLOSED) {
        /* A stream closed down, so we need to clean up the overlay on that stream */
        remove_overlay(stream_id);
    }

out:
    g_clear_error(&error);
    g_clear_object(&vdo_event);
    g_clear_object(&vdo_stream);
    g_clear_object(&stream_info);
    return ret;
}

/*
 * Create an overlay on the specified stream.
 *
 * Note that it is possible to create multiple overlays per stream, or we could filter out streams
 * further based on the properties we get from VDO. In this example we just create one overlay on
 * every stream.
 */
static void create_overlay(unsigned stream_id, unsigned stream_width, unsigned stream_height) {
    axo_err* axo_error = NULL;
    axo_props* props   = NULL;
    axo_match* match   = NULL;

    /* In this example we scale the overlay to a given fraction of the stream size */
    unsigned overlay_size = MIN(stream_width, stream_height) / 8;

    /*
     * For streams of high resolution, the overlay will become very big. This can require excessive
     * memory and CPU/GPU time.
     *
     * In such cases it is useful to enable the built-in upscaling function. The upscaling function
     * lets us draw in half resolution compared to what will be visible in the stream.
     *
     * Here we use a threshold of 4 megapixel for when to enable upscaling.
     */
    bool use_upscale = stream_width * stream_height > 4000000;

    if (use_upscale)
        overlay_size /= 2;

    unsigned overlay_used_width = overlay_size, overlay_used_height = overlay_size;

    /*
     * It is important to ensure that the size is properly aligned. The calculations above can
     * result in odd numbers and other dimensions which are not supported by the overlay system.
     * This utility function shall always be used to calculate the required padding.
     */
    unsigned overlay_full_width, overlay_full_height;
    if (!axo_get_aligned_size(AXO_FORMAT_ARGB32,
                              overlay_used_width,
                              overlay_used_height,
                              &overlay_full_width,
                              &overlay_full_height,
                              &axo_error)) {
        syslog(LOG_ERR, "Failed to get aligned overlay size: %s", axo_err_get_message(axo_error));
        goto out;
    }

    /* Create the overlay */
    props = axo_props_new();
    axo_props_set_format(props, AXO_FORMAT_ARGB32);
    axo_props_set_size(props, overlay_full_width, overlay_full_height);
    axo_props_set_upscale_x2(props, use_upscale);

    match = axo_match_new();
    axo_match_stream_id(match, stream_id);

    int overlay_id = axo_create_overlay(props, match, &axo_error);
    if (overlay_id < 0) {
        /*
         * It can happen that the stream closes down before we have time to create an overlay on
         * it. This is not an error. The condition is indicated by a special code. In this case we
         * will soon receive a disconnect event from VDO, so just ignore it and keep going.
         */
        if (axo_err_get_code(axo_error) != AXO_ERR_NO_STREAM)
            syslog(LOG_ERR,
                   "Failed to create overlay on stream %d: %s",
                   stream_id,
                   axo_err_get_message(axo_error));

        goto out;
    }

    syslog(LOG_INFO,
           "Created overlay %u on stream %u, stream_size=%ux%u overlay_used_size=%ux%u "
           "overlay_full_size=%ux%u",
           overlay_id,
           stream_id,
           stream_width,
           stream_height,
           overlay_used_width,
           overlay_used_height,
           overlay_full_width,
           overlay_full_height);

    /* Create a re-usable cairo surface. This is further explained in the render function */
    cairo_surface_t* surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, overlay_full_width, overlay_full_height);

    /* Check that cairo didn't add any extra padding. This should never happen */
    assert(overlay_full_width * sizeof(uint32_t) ==
           (unsigned)cairo_image_surface_get_stride(surface));

    /* Create and store a record of this overlay in the overlay table */
    struct overlay* overlay = g_malloc(sizeof(*overlay));
    *overlay                = (struct overlay){
                       .overlay_id  = overlay_id,
                       .stream_id   = stream_id,
                       .used_width  = overlay_used_width,
                       .used_height = overlay_used_height,
                       .full_width  = overlay_full_width,
                       .full_height = overlay_full_height,
                       .surface     = surface,
    };

    g_hash_table_insert(overlay_table, GINT_TO_POINTER(stream_id), overlay);

    /* Process the first frame for this new overlay */
    process_next_frame(overlay);

out:
    axo_err_clear(&axo_error);

    if (props)
        axo_props_free(props);

    if (match)
        axo_match_free(match);
}

/*
 * Remove the overlay on the specified stream, if one existed.
 */
static void remove_overlay(unsigned stream_id) {
    axo_err* axo_error            = NULL;
    const struct overlay* overlay = g_hash_table_lookup(overlay_table, GINT_TO_POINTER(stream_id));

    if (!overlay)
        goto out;

    if (!axo_remove_overlay(overlay->overlay_id, &axo_error)) {
        syslog(LOG_ERR,
               "Failed to remove overlay %u on stream %u: %s",
               overlay->overlay_id,
               stream_id,
               axo_err_get_message(axo_error));
        goto out;
    }

    syslog(LOG_INFO, "Removed overlay %u from stream %u", overlay->overlay_id, stream_id);

out:
    g_hash_table_remove(overlay_table, GINT_TO_POINTER(stream_id));

    axo_err_clear(&axo_error);
}

/*
 * Draw and submit a new frame of animation for this overlay.
 */
static void process_next_frame(struct overlay* overlay) {
    axo_err* axo_error = NULL;

    /* Get a free buffer to draw into from the overlay system */
    axo_buffer* buffer = axo_get_buffer(overlay->overlay_id, NULL, &axo_error);
    if (!buffer) {
        axo_err_code code = axo_err_get_code(axo_error);

        /*
         * There are some situations where a buffer is not available within a reasonable time. This
         * happens during normal camera usage. All clients must handle these conditions by checking
         * for specific error codes. Please see the API documentation for axo_get_buffer.
         */
        if (code == AXO_ERR_NO_STREAM || code == AXO_ERR_WAIT)
            goto out;

        syslog(LOG_ERR,
               "Failed to get buffer for overlay %u: %s",
               overlay->overlay_id,
               axo_err_get_message(axo_error));
        goto out;
    }

    char* target_buffer = axo_buffer_get_data(buffer, &axo_error);
    if (!target_buffer) {
        syslog(LOG_ERR, "Failed to get buffer data: %s", axo_err_get_message(axo_error));
        goto out;
    }

    /* Draw graphics into the buffer */
    render_frame(overlay, target_buffer);

    /* Submit the buffer to be shown on the stream */
    if (!axo_submit_buffer(buffer, NULL, &axo_error)) {
        syslog(LOG_ERR,
               "Failed to submit buffer for overlay %u: %s",
               overlay->overlay_id,
               axo_err_get_message(axo_error));
        goto out;
    }

out:
    axo_err_clear(&axo_error);
}

/*
 * Render a frame of animation. This is just a placeholder for demo purposes. It should be replaced
 * with your own rendering logic.
 *
 * In this example we use the Cairo graphics library. Another suitable graphics toolkit could be
 * used instead. For details on how Cairo works, please consult the Cairo documentation:
 * https://www.cairographics.org/manual/index.html.
 *
 * The are two things to keep in mind here:
 *
 * - It may not be possible to draw directly into the target buffer using a CPU-based toolkit like
 *   Cairo. Overlay buffers are a type of device memory and the caches may not be set up in a way
 *   that is compatible with direct drawing from the CPU.
 *
 *   The solution to this is to draw into a separate buffer (overlay->surface here) and then copy
 *   the final result into the target buffer. For efficiency, a single surface can be re-used
 *   across frames.
 *
 * - Due to alignment there may be some extra padding pixels at the right and bottom sides of the
 *   buffer. We need to clear these pixels to transparency, but during the actual drawing we do not
 *   normally touch them.
 */
static void render_frame(struct overlay* overlay, char* target_buffer) {
    cairo_t* cr = cairo_create(overlay->surface);

    /* Clear the separate buffer (all of it, not just the used area) */
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    /*
     * Rescale coordinates so that (1.0, 1.0) is the bottom-right edge of the used overlay area.
     * The padding may extend slightly past this.
     */
    cairo_scale(cr, overlay->used_width, overlay->used_height);

    /* Animate the overlay with a simple rotation around the centre of the used area */
    double t = M_PI * animation_state / 180.0;

    cairo_translate(cr, 0.5, 0.5);
    cairo_rotate(cr, t);
    cairo_translate(cr, -0.5, -0.5);

    /* Animate colour selection */
    double r = sin(t) * sin(t), g = cos(t) * cos(t);

    /* Draw an example icon */
    cairo_set_line_width(cr, 0.04);
    cairo_set_source_rgb(cr, r, g, 0.0);

    cairo_arc(cr, 0.5, 0.5, 0.45, 0.0, 2.0 * M_PI);
    cairo_stroke(cr);

    cairo_arc(cr, 0.4, 0.4, 0.05, 0.0, 2.0 * M_PI);
    cairo_stroke(cr);

    cairo_arc(cr, 0.6, 0.4, 0.05, 0.0, 2.0 * M_PI);
    cairo_stroke(cr);

    cairo_arc(cr, 0.5, 0.5, 0.30, 0.0, M_PI);
    cairo_stroke(cr);

    /* Finish and copy the resulting image to the overlay target buffer */
    cairo_destroy(cr);

    unsigned byte_size = overlay->full_width * overlay->full_height * sizeof(uint32_t);
    memcpy(target_buffer, cairo_image_surface_get_data(overlay->surface), byte_size);
}
