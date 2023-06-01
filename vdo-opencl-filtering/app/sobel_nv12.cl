/**
 * Copyright (C) 2021, Axis Communications AB, Lund, Sweden
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

__kernel void sobel_3x1(__global const unsigned char *In_y,
                        __global const unsigned char *Out_y,
                        __global const unsigned char *Out_cbcr,
                        int width,
                        int height)
{
    int row = get_global_id(0);
    if (row > height - 1)
        return;

    /* Since we work on 8 pixels at a time, col needs to be multiplied by 8. */
    int col = (get_global_id(1) << 3);
    /* Offset of 1 is added to make sure we read from within the buffer. */
    int pix_id = (row * width) + col + 1;

    /* Interpretation here is that cbcr buffer is as wide as y buffer,
     * since for two y values in width there is one cb and one cr. However
     * the amount of rows are halved, so we write the same cbcr row twice
     * for each row of y. */
    int cbcr_id = ((row >> 1) * width) + (col);

    short8 gx = (short8)0;
    short8 gy = (short8)0;
    uchar8 mag = (uchar8)0;

    /* Previous row */
    uchar16 temp = vload16(0, (uchar*)In_y + pix_id - width - 1);
    short8 middle = convert_short8(temp.s12345678);

    gy += middle * (short8)(-2);

    /* Current row */
    temp = vload16(0, (uchar*) In_y + pix_id -1);
    short8 left = convert_short8(temp.s01234567);
    short8 right = convert_short8(temp.s23456789);

    gx += left * (short8)(-2);
    gx += right * (short8)(2);

    /* Next row */
    temp = vload16(0, (uchar*) In_y + pix_id + width - 1);
    middle = convert_short8(temp.s12345678);

    gy += middle * (short8)(2);

    mag = convert_uchar8(clamp(abs(gx) + abs(gy),1, 255));
    vstore8(mag, 0, Out_y + pix_id);

    /* Write cbcr data (128 for greyscale) */
    uchar8 cbcr = (uchar8) 128;
    vstore8(cbcr, 0, Out_cbcr + cbcr_id);
}

__kernel void sobel_3x3(__global const unsigned char *In_y,
                        __global const unsigned char *Out_y,
                        __global const unsigned char *Out_cbcr,
                        int width,
                        int height)
{
    int row = get_global_id(0);
    if (row > height - 1)
        return;

    int col = (get_global_id(1) << 3);
    int pix_id = (row * width) + col + 1;
    int cbcr_id = ((row >> 1) * width) + (col);

    short8 gx = (short8)0;
    short8 gy = (short8)0;
    uchar8 mag = (uchar8)0;

    /* Previous row */
    uchar16 temp = vload16(0, (uchar*)In_y + pix_id - width - 1);
    short8 left = convert_short8(temp.s01234567);
    short8 middle = convert_short8(temp.s12345678);
    short8 right = convert_short8(temp.s23456789);

    gx += left * (short8)(-1);
    gx += right * (short8)(1);

    gy += left * (short8)(-1);
    gy += middle * (short8)(-2);
    gy += right * (short8)(-1);

    /* Current row */
    temp = vload16(0, (uchar*) In_y + pix_id -1);
    left = convert_short8(temp.s01234567);
    right = convert_short8(temp.s23456789);

    gx += left * (short8)(-2);
    gx += right * (short8)(2);

    /* Next row */
    temp = vload16(0, (uchar*) In_y + pix_id + width - 1);
    left = convert_short8(temp.s01234567);
    middle = convert_short8(temp.s12345678);
    right = convert_short8(temp.s23456789);

    gx += left * (short8)(-1);
    gx += right * (short8)(1);

    gy += left * (short8)(1);
    gy += middle * (short8)(2);
    gy += right * (short8)(1);

    mag = convert_uchar8(clamp(abs(gx) + abs(gy),1, 255));
    vstore8(mag, 0, Out_y + pix_id);

    /* Write cbcr data (128 for greyscale) */
    uchar8 cbcr = (uchar8) 128;
    vstore8(cbcr, 0, Out_cbcr + cbcr_id);
}
