/**
 * Copyright (C) 2024, Axis Communications AB, Lund, Sweden
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

#include <bbox.h>

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

volatile sig_atomic_t running = 1;

static void shutdown(int status) {
    running = 0;
}

static inline void panic(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsyslog(LOG_ERR, format, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

// This example illustrates drawing on a single channel.
// The coordinate-space equals the visible area of the chosen channel.
//
//    ┏━━━━━━━━━━━━━━━━━━━━━━━━┓
//    ┃                        ┃
//    ┃ [0,0]                  ┃
//    ┃   ┏━━━━━━━━━━┓         ┃
//    ┃   ┃          ┃         ┃
//    ┃   ┃ Channel1 ┃         ┃
//    ┃   ┃          ┃         ┃
//    ┃   ┗━━━━━━━━━━┛         ┃
//    ┃            [1,1]       ┃
//    ┃                        ┃
//    ┃                        ┃
//    ┗━━━━━━━━━━━━━━━━━━━━━━━━┛
//
// The intended usecase is performing video content analytics on one channel,
// then draw bounding boxes with the same coordinate-space as was used for
// Video Content Analytics (VCA).
void example_single_channel() {
    // Draw on a single view: 1
    bbox_t* bbox = bbox_view_new(1u);
    if (!bbox)
        panic("Failed creating: %s", strerror(errno));

    bbox_clear(bbox);  // Remove all old bounding-boxes

    // Create all needed colors [These operations are slow!]
    const bbox_color_t red   = bbox_color_from_rgb(0xff, 0x00, 0x00);
    const bbox_color_t blue  = bbox_color_from_rgb(0x00, 0x00, 0xff);
    const bbox_color_t green = bbox_color_from_rgb(0x00, 0xff, 0x00);

    bbox_style_outline(bbox);                      // Switch to outline style
    bbox_thickness_thin(bbox);                     // Switch to thin lines
    bbox_color(bbox, red);                         // Switch to red [This operation is fast!]
    bbox_rectangle(bbox, 0.05, 0.05, 0.95, 0.95);  // Draw a thin red outline rectangle

    bbox_style_corners(bbox);                      // Switch to corners style
    bbox_thickness_thick(bbox);                    // Switch to thick lines
    bbox_color(bbox, blue);                        // Switch to blue [This operation is fast!]
    bbox_rectangle(bbox, 0.40, 0.40, 0.60, 0.60);  // Draw thick blue corners

    bbox_style_corners(bbox);                      // Switch to corners style
    bbox_thickness_medium(bbox);                   // Switch to medium lines
    bbox_color(bbox, blue);                        // Switch to blue [This operation is fast!]
    bbox_rectangle(bbox, 0.30, 0.30, 0.50, 0.50);  // Draw medium blue corners

    bbox_style_outline(bbox);   // Switch to outline style
    bbox_thickness_thin(bbox);  // Switch to thin lines
    bbox_color(bbox, red);      // Switch to red [This operation is fast!]

    // Draw a thin red quadrilateral
    bbox_quad(bbox, 0.10, 0.10, 0.30, 0.12, 0.28, 0.28, 0.11, 0.30);

    // Draw a green polyline
    bbox_color(bbox, green);  // Switch to green [This operation is fast!]
    bbox_move_to(bbox, 0.2, 0.2);
    bbox_line_to(bbox, 0.5, 0.5);
    bbox_line_to(bbox, 0.8, 0.4);
    bbox_draw_path(bbox);

    // Draw all queued geometry simultaneously
    if (!bbox_commit(bbox, 0u))
        panic("Failed committing: %s", strerror(errno));

    if (running)
        sleep(5);
    bbox_destroy(bbox);
}

// This example illustrates drawing on multiple channels.
// The coordinate-space equals global (entire sensor).
//
//  [0,0]
//    ┏━━━━━━━━━━━━━━━━━━━━━━━━┓
//    ┃  Channel1              ┃
//    ┃  ┏━━━━━━┓              ┃
//    ┃  ┃      ┃              ┃
//    ┃  ┃      ┃              ┃
//    ┃  ┗━━━━━━┛              ┃
//    ┃              Channel2  ┃
//    ┃              ┏━━━━━━┓  ┃
//    ┃              ┃      ┃  ┃
//    ┃              ┃      ┃  ┃
//    ┃              ┗━━━━━━┛  ┃
//    ┗━━━━━━━━━━━━━━━━━━━━━━━━┛
//                           [1,1]
//
// The intended usecase is performing video content analytics on the entire image,
// then draw bounding boxes with the same coordinate-space as was used for VCA,
// and have it appear in all chosen channels simultaneously.
//
// Note that if you instead run VCA once per channel, i.e. multiple images,
// then you need to manually translate the coordinates to the global image space,
// before they can be drawn.
void example_multiple_channels() {
    // Draw on channel 1 and 2
    bbox_t* bbox = bbox_new(2u, 1u, 2u);
    if (!bbox)
        panic("Failed creating: %s", strerror(errno));

    // If camera lacks video output, this call will succeed but not do anything.
    if (!bbox_video_output(bbox, true))
        panic("Failed enabling video-output: %s", strerror(errno));

    // Create all needed colors [These operations are slow!]
    const bbox_color_t colors[] = {
        bbox_color_from_rgb(0xff, 0u, 0u),
        bbox_color_from_rgb(0u, 0xff, 0u),
        bbox_color_from_rgb(0u, 0u, 0xff),
    };

    const size_t ncolors = sizeof(colors) / sizeof(colors[0u]);

    // Switch to thick corner style
    bbox_thickness_thick(bbox);
    bbox_style_corners(bbox);

    const float w     = 1920.f;
    const float h     = 1080.f;
    const float box_w = 100.f / w;
    const float box_h = 100.f / h;

    // Draw 32 bounding-boxes
    for (size_t i = 0; i < 32; ++i) {
        const float x = (200u) * (i % 8u) / w;
        const float y = (200u) * (i / 8u) / h;

        // Switch color [This operation is fast!]
        bbox_color(bbox, colors[i % ncolors]);

        bbox_rectangle(bbox, x, y, x + box_w, y + box_h);
    }

    // Draw all queued geometry simultaneously
    if (!bbox_commit(bbox, 0u))
        panic("Failed committing: %s", strerror(errno));

    if (running)
        sleep(5);
    bbox_destroy(bbox);
}

void example_clear() {
    // Draw on a single channel: 1
    bbox_t* bbox = bbox_new(1u, 1u);
    if (!bbox)
        panic("Failed creating: %s", strerror(errno));

    bbox_clear(bbox);  // Remove all old bounding-boxes

    // Clear everything simultaneously
    if (!bbox_commit(bbox, 0u))
        panic("Failed committing: %s", strerror(errno));

    if (running)
        sleep(5);
    bbox_destroy(bbox);
}

static void init_signals(void) {
    const struct sigaction sa = {
        .sa_handler = shutdown,
    };

    if (sigaction(SIGINT, &sa, NULL) < 0)
        panic("Failed installing SIGINT handler: %s", strerror(errno));

    if (sigaction(SIGTERM, &sa, NULL) < 0)
        panic("Failed installing SIGTERM handler: %s", strerror(errno));
}

int main(void) {
    openlog(NULL, LOG_PID, LOG_USER);

    init_signals();

    for (bool once = true; running; once = false) {
        example_single_channel();
        example_multiple_channels();
        example_clear();
        if (once)
            syslog(LOG_INFO, "All examples succeeded.");
    }

    return EXIT_SUCCESS;
}
