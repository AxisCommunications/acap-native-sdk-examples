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

#include "panic.h"

#include "imgprovider.h"
using namespace cv;

int main(void) {
    openlog("opencv_app", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "Running OpenCV example with VDO as video source");
    img_provider_t* image_provider = nullptr;
    g_autoptr(GError) vdo_error    = nullptr;

    // The desired width and height of the BGR frame
    unsigned int width  = 1024;
    unsigned int height = 576;

    VdoFormat vdo_format = VDO_FORMAT_YUV;
    double vdo_framerate = 30.0;

    // Choose a valid stream resolution
    unsigned int stream_width  = 0;
    unsigned int stream_height = 0;
    if (!choose_stream_resolution(width,
                                  height,
                                  vdo_format,
                                  nullptr,
                                  "all",
                                  &stream_width,
                                  &stream_height)) {
        panic("%s: Failed choosing stream resolution", __func__);
    }

    syslog(LOG_INFO,
           "Creating VDO image provider and creating stream %u x %u",
           stream_width,
           stream_height);

    image_provider = create_img_provider(stream_width, stream_height, 2, vdo_format, vdo_framerate);
    if (!image_provider) {
        panic("%s: Could not create image provider", __func__);
    }

    syslog(LOG_INFO, "Start fetching video frames from VDO");
    if (!img_provider_start(image_provider)) {
        panic("%s: Could not start image provider", __func__);
    }

    // Create the background subtractor
    Ptr<BackgroundSubtractorMOG2> bgsub = createBackgroundSubtractorMOG2();

    // Create the filtering element. Its size influences what is considered
    // noise, with a bigger size corresponding to more denoising
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(9, 9));

    // Create OpenCV Mats for the camera frame (nv12), the converted frame (bgr)
    // and the foreground frame that is outputted by the background subtractor
    Mat bgr_mat  = Mat(height, width, CV_8UC3);
    Mat nv12_mat = Mat(height * 3 / 2, width, CV_8UC1);
    Mat fg;

    while (true) {
        struct timeval start_ts, end_ts;
        unsigned int opencv_ms = 0;
        // Get frame from vdo
        g_autoptr(VdoBuffer) vdo_buf = img_provider_get_frame(image_provider);
        if (!vdo_buf) {
            // This can only happen if it is global rotation then
            // the stream has be restarted because rotation has been changed.
            syslog(LOG_INFO, "No buffer because of global rotation");
            goto end;
        }

        gettimeofday(&start_ts, nullptr);
        // Assign the VDO image buffer to the nv12_mat OpenCV Mat.
        // This specific Mat is used as it is the one we created for NV12,
        // which has a different layout than e.g., BGR.
        nv12_mat.data = static_cast<uint8_t*>(vdo_buffer_get_data(vdo_buf));

        // Convert the NV12 data to BGR
        cvtColor(nv12_mat, bgr_mat, COLOR_YUV2BGR_NV12, 3);

        // Perform background subtraction on the bgr image with
        // learning rate 0.005. The resulting image should have
        // pixel intensities > 0 only where changes have occurred
        bgsub->apply(bgr_mat, fg, 0.005);

        // Filter noise from the image with the filtering element
        morphologyEx(fg, fg, MORPH_OPEN, kernel);

        // We define movement in the image as any pixel being non-zero
        int nonzero_pixels = countNonZero(fg);
        if (nonzero_pixels > 0) {
            syslog(LOG_INFO, "Motion detected: YES");
        } else {
            syslog(LOG_INFO, "Motion detected: NO");
        }
        gettimeofday(&end_ts, nullptr);
        opencv_ms = static_cast<unsigned int>(((end_ts.tv_sec - start_ts.tv_sec) * 1000) +
                                              ((end_ts.tv_usec - start_ts.tv_usec) / 1000));
        syslog(LOG_INFO, "Ran opencv for %u ms", opencv_ms);
        // Check if the framerate from vdo should be changed
        img_provider_update_framerate(image_provider, opencv_ms);

        // This will allow vdo to fill this buffer with data again
        if (!vdo_stream_buffer_unref(image_provider->vdo_stream, &vdo_buf, &vdo_error)) {
            if (!vdo_error_is_expected(&vdo_error)) {
                panic("%s: Unexpexted error: %s", __func__, vdo_error->message);
            }
            g_clear_error(&vdo_error);
        }
    }
end:
    if (image_provider) {
        destroy_img_provider(image_provider);
    }
    syslog(LOG_INFO, "Exit opencv_app");
    return EXIT_SUCCESS;
}
