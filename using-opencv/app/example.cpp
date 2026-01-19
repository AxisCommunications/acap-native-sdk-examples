/**
 * Copyright (C) 2021 Axis Communications AB, Lund, Sweden
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include <opencv2/imgproc.hpp>
#pragma GCC diagnostic pop
#include <opencv2/video.hpp>
#include <stdlib.h>
#include <sys/time.h>
#include <syslog.h>

#include <vdo-error.h>
#include <vdo-stream.h>

#include <cerrno>
#include <cstdint>
#include <cstdlib>

#include <poll.h>
#include <unistd.h>

#include "panic.h"

using namespace cv;

volatile sig_atomic_t running = 1;

static void shutdown(int status) {
    (void)status;
    running = 0;
}

int main(void) {
    g_autoptr(GError) vdo_error = nullptr;
    auto failed                 = [&vdo_error] {
        // Maintenance/Installation in progress (e.g. Global-Rotation)
        if (vdo_error_is_expected(&vdo_error)) {
            syslog(LOG_INFO, "Expected vdo error %s", vdo_error->message);
            return EXIT_SUCCESS;
        } else {
            panic("Unexpected vdo error %s", vdo_error->message);
        }
    };

    // Stop main loop at signal
    signal(SIGTERM, shutdown);
    signal(SIGINT, shutdown);

    // The desired width and height of the Y800 frame
    unsigned int width         = 1024;
    unsigned int height        = 576;
    unsigned int input_channel = 1;

    syslog(LOG_INFO, "Running OpenCV example with VDO as video source");

    // From vdo-stream.h
    // AXIS OS 12.8+
    // Unlike vdo_stream_new(), this API enables automatic framerate adjustment
    // (to disable this feature, you need to explicitly specify the framerate).
    // In addition, it applies several other defaults suitable for video analytics.
    // It's still possible to override options such as 'buffer.count' or 'image.fit'.
    // This convenience API is roughly equivalent to:
    // vdo_map_set_boolean(settings, "socket.blocking", false);
    // vdo_map_set_string(settings,  "image.fit", "scale");
    // vdo_map_set_uint32(settings,  "buffer.count", 2u);
    // vdo_map_set_uint32(settings,  "format", VDO_FORMAT_YUV);
    // vdo_map_set_string(settings,  "subformat", "Y800");
    // vdo_map_set_uint32(settings,  "input", ...);
    // vdo_map_set_pair32u(settings, "resolution", ...);
    //
    // "image.fit"  "crop" clips to cover the frame and "scale" shrinks to contain the image (AXIS
    // OS 12.7+ and Artpec-7+) "crop" works on all platforms. On Ambarella CV25, only YUV and RGB
    // work with "scale".
    g_autoptr(VdoStream) vdo_stream =
        vdo_stream_y800_new(nullptr, input_channel, {width, height}, &vdo_error);
    if (!vdo_stream)
        return failed();

    g_autoptr(VdoMap) vdo_info = vdo_stream_get_info(vdo_stream, &vdo_error);
    if (!vdo_info)
        return failed();

    syslog(LOG_INFO, "Creating VDO image provider and creating stream %u x %u", width, height);

    int fd = vdo_stream_get_fd(vdo_stream, &vdo_error);
    if (fd < 0)
        return failed();

    pollfd fds = {
        .fd     = fd,
        .events = POLL_IN,
    };

    syslog(LOG_INFO, "Start fetching video frames from VDO");
    if (!vdo_stream_start(vdo_stream, &vdo_error))
        return failed();

    // Create the background subtractor
    Ptr<BackgroundSubtractorMOG2> bgsub = createBackgroundSubtractorMOG2();

    // Create the filtering element. Its size influences what is considered
    // noise, with a bigger size corresponding to more denoising
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(9, 9));

    // Handle rotation 90/270, the width and height are swapped in the info map
    // if rotation is 90/270.
    width  = vdo_map_get_uint32(vdo_info, "width", width);
    height = vdo_map_get_uint32(vdo_info, "height", height);

    // Create OpenCV Mats for the camera frame (Y800)
    // The foreground frame that is outputted by the background subtractor
    Mat gray_image  = Mat(height, width, CV_8UC1);
    gray_image.step = vdo_map_get_uint32(vdo_info, "pitch", width);

    Mat fg;

    while (running) {
        struct timeval start_ts, end_ts;
        unsigned int opencv_ms = 0;

        int status = TEMP_FAILURE_RETRY(poll(&fds, 1, -1));
        if (status < 0)
            panic("Failed to poll with status %d", status);

        // Get frame from vdo
        g_autoptr(VdoBuffer) vdo_buf = vdo_stream_get_buffer(vdo_stream, &vdo_error);
        if (!vdo_buf && g_error_matches(vdo_error, VDO_ERROR, VDO_ERROR_NO_DATA))
            continue;  // Transient Error -> Retry!

        if (!vdo_buf)
            return failed();

        gettimeofday(&start_ts, nullptr);
        // Assign the VDO image buffer to the gray_image OpenCV Mat.
        gray_image.data = static_cast<uint8_t*>(vdo_buffer_get_data(vdo_buf));

        // Perform background subtraction on the bgr image with
        // learning rate 0.005. The resulting image should have
        // pixel intensities > 0 only where changes have occurred
        bgsub->apply(gray_image, fg, 0.005);

        // Filter noise from the image with the filtering element
        morphologyEx(fg, fg, MORPH_OPEN, kernel);

        // We define movement in the image as any pixel being non-zero
        int nonzero_pixels = countNonZero(fg);
        if (nonzero_pixels > 0)
            syslog(LOG_INFO, "Motion detected: YES");
        else
            syslog(LOG_INFO, "Motion detected: NO");

        gettimeofday(&end_ts, nullptr);
        opencv_ms = static_cast<unsigned int>(((end_ts.tv_sec - start_ts.tv_sec) * 1000) +
                                              ((end_ts.tv_usec - start_ts.tv_usec) / 1000));
        syslog(LOG_INFO, "Ran opencv for %u ms", opencv_ms);
        // Check if the framerate from vdo should be changed

        // This will allow vdo to fill this buffer with data again
        if (!vdo_stream_buffer_unref(vdo_stream, &vdo_buf, &vdo_error))
            return failed();
    }
    syslog(LOG_INFO, "Exit opencv_app");
    return EXIT_SUCCESS;
}
