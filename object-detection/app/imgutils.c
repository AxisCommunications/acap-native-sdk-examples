/**
 * Copyright (C) 2021, Axis Communications AB, Lund, Sweden
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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <jpeglib.h>


/**
 * @brief Encode an image buffer as jpeg and store it in memory
 *
 * @param image_buffer An image buffer with interleaved (if RGB) channel layout
 * @param jpeg_conf A struct defining how the image is to be encoded
 * @param jpeg_size The output size of the jpeg
 * @param jpeg_buffer The output buffer of the jpeg
 */
void buffer_to_jpeg(unsigned char* image_buffer, struct jpeg_compress_struct* jpeg_conf,
                    unsigned long* jpeg_size, unsigned char** jpeg_buffer) {
        struct jpeg_error_mgr jerr;
        JSAMPROW row_pointer[1];

        jpeg_conf->err = jpeg_std_error(&jerr);

        jpeg_mem_dest(jpeg_conf, jpeg_buffer, jpeg_size);
        jpeg_start_compress(jpeg_conf, TRUE);

        int stride = jpeg_conf->image_width * jpeg_conf->input_components;
        while (jpeg_conf->next_scanline < jpeg_conf->image_height) {
                row_pointer[0] = &image_buffer[jpeg_conf->next_scanline * stride];
                jpeg_write_scanlines(jpeg_conf, row_pointer, 1);
        }

        jpeg_finish_compress(jpeg_conf);
        jpeg_destroy_compress(jpeg_conf);
}


/**
 * @brief Inserts common values into a jpeg configuration struct
 *
 * @param width The width of the image
 * @param height The height of the image
 * @param channels The number of channels of the image
 * @param quality The desired jpeg quality (0-100)
 * @param jpeg_conf The jpeg configuration struct to modify
 */
void set_jpeg_configuration(int width, int height, int channels, int quality,
                            struct jpeg_compress_struct* jpeg_conf)
{
        // Only supports RGB and grayscale
        jpeg_create_compress(jpeg_conf);
        jpeg_conf->image_width = width;
        jpeg_conf->image_height = height;
        jpeg_conf->input_components = channels;

        if (channels == 1) {
                jpeg_conf->in_color_space = JCS_GRAYSCALE;
        } else if(channels == 3) {
                jpeg_conf->in_color_space = JCS_RGB;
        } else {
                printf("Number of channels not supported\n");
                exit(1);
        }

        jpeg_set_defaults(jpeg_conf);
        jpeg_set_quality(jpeg_conf, quality, TRUE);
}


/**
 * @brief Writes a memory buffer to a file
 *
 * @param file_name The desired path of the output file
 * @param buffer The data to be written
 * @param buffer_size The size of the data to be written
 */
void jpeg_to_file(char* file_name, unsigned char* buffer, unsigned long buffer_size){
        FILE *fp;
        if((fp = fopen(file_name, "wb")) == NULL)
        {
                printf("Unable to open file!\n");
                exit(1);
        }
        fwrite(buffer, sizeof(unsigned char), buffer_size, fp);
        fclose(fp);
}


/**
 * @brief Crops a rectangular patch from an image buffer.
 *       The image channels are expected to be interleaved.
 *
 * @param image_buffer A buffer holding an uint8 image
 * @param image_w The input image's width in pixels
 * @param image_h The input image's height in pixels
 * @param channels The input image's number of channels
 * @param crop_x The leftmost pixel coordinate of the desired crop
 * @param crop_y The top pixel coordinate of the desired crop
 * @param crop_w The width of the desired crop in pixels
 * @param crop_h The height of the desired crop in pixels
 */
unsigned char* crop_interleaved(unsigned char* image_buffer, int image_w,
                                int image_h, int channels,
                                int crop_x, int crop_y,
                                int crop_w, int crop_h)
{
        // This function crops out a section of an image that is located at
        // (x, y, x + w, x + h).
        // The input buffer channel layout is assumed to be interleaved
        unsigned char* crop_buffer = (unsigned char*)malloc(crop_w * crop_h * channels);

        // We go over each row affected by the crop and copy a contiguous
        // crop_buffer_width sized block of memory
        int image_buffer_width = image_w * channels;
        int crop_buffer_width = crop_w * channels;
        for(int row = crop_y; row < crop_y + crop_h; row++) {
                int image_buffer_pos = image_buffer_width * row + crop_x * channels;
                int crop_buffer_pos = crop_buffer_width * (row - crop_y);
                memcpy(crop_buffer + crop_buffer_pos, image_buffer + image_buffer_pos,
                       crop_buffer_width);
        }
        return crop_buffer;
}


/**
 * @brief An example of how to use the supplied utility functions
 *
 */
void test_buffer_to_jpeg_file() {
        // An example of how to use the various utility functions
        // Generates an image buffer, crops a section of it, encodes the crop to jpeg
        // and saves the jpeg to file
        int width = 1920;
        int height = 1080;
        int channels = 3;
        unsigned char* image_buffer = (unsigned char*)malloc(width * height * channels);

        // An image buffer with interleaved layout
        // The pattern should be a yellow top-bottom gradient
        for (int i = 0; i < width * height; i++) {
                for (int channel = 0; channel < channels; channel++) {
                        int green_mask = 1;
                        if (channel == 2) green_mask = 0;
                        image_buffer[i * channels + channel] =  (double) i / (width * height) * 255 * green_mask;
                }
        }

        // A 100px wide crop along the original image's right side from top to bottom
        int crop_x = width - 100;
        int crop_y = 0;
        int crop_w = 100;
        int crop_h = height;
        unsigned char* crop_buffer = crop_interleaved(image_buffer, width, height, channels,
                                                      crop_x, crop_y, crop_w, crop_h);

        // Encode buffer to jpeg in memory
        unsigned long jpeg_size = 0;
        unsigned char* jpeg_buffer = NULL;
        struct jpeg_compress_struct jpeg_conf;
        set_jpeg_configuration(crop_w, crop_h, channels, 80, &jpeg_conf);
        buffer_to_jpeg(crop_buffer, &jpeg_conf, &jpeg_size, &jpeg_buffer);

        // Write jpeg buffer to file
        jpeg_to_file("/tmp/test.jpg", jpeg_buffer, jpeg_size);

        // Release memory
        free(image_buffer);
        free(crop_buffer);
        free(jpeg_buffer);
}

