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

#include "larod-vdo-utils.h"

#include <glib.h>
#include <larod.h>
#include <vdo-buffer.h>
#include <vdo-stream.h>
#include <vdo-types.h>

G_LOCK_DEFINE_STATIC(mtxMap);

static GHashTable* map = NULL;

static void destroyTensor(gpointer data) {
    larodTensor** tensors = (larodTensor**) data;

    larodDestroyTensors(&tensors, 1);
}

static bool setupTensorMetadata(larodTensor* tensor, uint32_t width,
                                uint32_t height, uint32_t pitch,
                                VdoFrameType format, const char* subformat,
                                larodError** error) {
    larodTensorDataType dataType = LAROD_TENSOR_DATA_TYPE_UNSPECIFIED;
    larodTensorLayout layout = LAROD_TENSOR_LAYOUT_UNSPECIFIED;
    larodTensorDims dims = {0};
    larodTensorPitches pitches = {0};

    // Figure out data type, layout, dims and pitches.
    switch (format) {
    case VDO_FRAME_TYPE_YUV:
        if (g_ascii_strcasecmp(subformat, "nv12") == 0) {
            dataType = LAROD_TENSOR_DATA_TYPE_UINT8;
            layout = LAROD_TENSOR_LAYOUT_420SP;

            if (width && height && pitch) {
                dims.len = 3;
                dims.dims[0] = 3;
                dims.dims[1] = height;
                dims.dims[2] = width;

                pitches.len = 3;
                pitches.pitches[2] = pitch;
                pitches.pitches[1] = height * pitches.pitches[2];
                pitches.pitches[0] = 3 * pitches.pitches[1] / 2;
            }
        }
        break;
    case VDO_FRAME_TYPE_RGB:
        dataType = LAROD_TENSOR_DATA_TYPE_UINT8;
        layout = LAROD_TENSOR_LAYOUT_NHWC;

        if (width && height && pitch) {
            dims.len = 4;
            dims.dims[0] = 1;
            dims.dims[1] = height;
            dims.dims[2] = width;
            dims.dims[3] = 3;

            pitches.len = 4;
            pitches.pitches[3] = 3;
            pitches.pitches[2] = pitch * pitches.pitches[3];
            pitches.pitches[1] = height * pitches.pitches[2];
            pitches.pitches[0] = 1 * pitches.pitches[1];
        }
        break;
    case VDO_FRAME_TYPE_PLANAR_RGB:
        dataType = LAROD_TENSOR_DATA_TYPE_UINT8;
        layout = LAROD_TENSOR_LAYOUT_NCHW;

        if (width && height && pitch) {
            dims.len = 4;
            dims.dims[0] = 1;
            dims.dims[1] = 3;
            dims.dims[2] = height;
            dims.dims[3] = width;

            pitches.len = 4;
            pitches.pitches[3] = pitch;
            pitches.pitches[2] = height * pitches.pitches[3];
            pitches.pitches[1] = 3 * pitches.pitches[2];
            pitches.pitches[0] = 1 * pitches.pitches[1];
        }
        break;
    default:
        break;
    }

    if (!larodSetTensorDataType(tensor, dataType, error)) {
        return false;
    }

    if (!larodSetTensorLayout(tensor, layout, error)) {
        return false;
    }

    if (!larodSetTensorDims(tensor, &dims, error)) {
        return false;
    }

    if (!larodSetTensorPitches(tensor, &pitches, error)) {
        return false;
    }

    return true;
}

static bool setupTensor(larodTensor* tensor, VdoStream* stream,
                        VdoBuffer* buffer, larodError** error) {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t pitch = 0;
    const char* subformat = "";
    VdoFrameType format = VDO_FRAME_TYPE_NONE;
    const char* bufferType = "";

    // Grab stream info.
    g_autoptr(VdoMap) info = vdo_stream_get_info(stream, NULL);
    if (info) {
        width = vdo_map_get_uint32(info, "width", 0);
        height = vdo_map_get_uint32(info, "height", 0);
        pitch = vdo_map_get_uint32(info, "pitch", width);
        subformat = vdo_map_get_string(info, "subformat", NULL, "");
        bufferType = vdo_map_get_string(info, "buffer.type", NULL, "");
    }

    VdoFrame* frame = vdo_buffer_get_frame(buffer);
    if (frame) {
        format = vdo_frame_get_frame_type(frame);
    }

    if (!setupTensorMetadata(tensor, width, height, pitch, format, subformat,
                             error)) {
        return false;
    }

    // Move over fd from VdoBuffer.
    int fd = vdo_buffer_get_fd(buffer);
    if (!larodSetTensorFd(tensor, fd, error)) {
        return false;
    }

    // Move over offset from VdoBuffer.
    int64_t off = vdo_buffer_get_offset(buffer);
    if (!larodSetTensorFdOffset(tensor, off, error)) {
        return false;
    }

    // Move over size from VdoBuffer. Capacity in vdo is the size of the buffer,
    // while size in larod is size of the buffer plus offset, so we have to pass
    // offset + cap as size. Because of different signedness we have to check
    // for underflow and overflow.
    uint64_t cap = vdo_buffer_get_capacity(buffer);
    if (cap <= INT64_MAX) {
        int64_t cap2 = (int64_t) cap;
        if (off + cap2 >= 0) {
            uint64_t size = (uint64_t)(off + cap2);
            if (size <= SIZE_MAX) {
                size_t size2 = (size_t) size;
                if (!larodSetTensorFdSize(tensor, size2, error)) {
                    return false;
                }
            }
        }
    }

    // Set props.
    uint32_t props = LAROD_FD_PROP_MAP;
    if (g_ascii_strcasecmp(bufferType, "dmabuf") == 0) {
        props |= LAROD_FD_PROP_DMABUF;
    } else if (g_ascii_strcasecmp(bufferType, "memfd") == 0) {
        props |= LAROD_FD_PROP_READWRITE;
    }
    if (!larodSetTensorFdProps(tensor, props, error)) {
        return false;
    }

    return true;
}

larodTensor* larodVdoUtilsBufferToTensor(larodConnection* conn,
                                         VdoStream* stream, VdoBuffer* buffer,
                                         larodError** error) {
    G_LOCK(mtxMap);

    if (!map) {
        map = g_hash_table_new_full(NULL, NULL, NULL, destroyTensor);
    }

    larodTensor** tensors = g_hash_table_lookup(map, buffer);
    if (!tensors) {
        tensors = larodCreateTensors(1, error);
        if (!tensors) {
            goto error;
        }

        // Fill the tensor with data and metadata from the VdoBuffer.
        if (!setupTensor(tensors[0], stream, buffer, error)) {
            larodDestroyTensors(&tensors, 1);

            goto error;
        }

        // Give this tensor an ID so larod can cache it.
        if (!larodTrackTensor(conn, tensors[0], error)) {
            larodDestroyTensors(&tensors, 1);

            goto error;
        }

        // Make buffer <-> tensor mapping. This mapping is kept until vdo calls
        // larodVdoUtilsDestroyTensor.
        g_hash_table_insert(map, buffer, tensors);
    }

error:
    G_UNLOCK(mtxMap);

    if (!tensors) {
        return NULL;
    }

    return tensors[0];
}

void larodVdoUtilsDestroyTensor(VdoBuffer* buffer) {
    G_LOCK(mtxMap);

    if (map) {
        // Remove buffer <-> tensor mapping.
        g_hash_table_remove(map, buffer);
    }

    G_UNLOCK(mtxMap);
}
