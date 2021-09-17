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

#pragma once

#include "larod.h"
#include "vdo-buffer.h"
#include "vdo-stream.h"

/**
 * @brief Create a larodTensor from VdoBuffer.
 *
 * This function returns a larodTensor that contains the data and metadata from
 * a VdoBuffer. The returned larodTensor is owned by this library and should not
 * be destroyed. Multiple calls to this function with the same @p buffer will
 * return the same larodTensor object.
 *
 * @param conn larodConnection The larodConnection to associate this larodTensor
 * with.
 * @param stream VdoStream The VdoStream the @p buffer is fetched from.
 * @param buffer VdoBuffer The VdoBuffer to create the larodTensor from.
 * @param error An uninitialized handle to an error. @p error can also be NULL
 * if one does not want any error information. In case of errors (when return
 * value is NULL), it must later be deallocated with @c larodClearError().
 * @return NULL if error occurred, otherwise pointer to a @c larodTensor.
 */
larodTensor* larodVdoUtilsBufferToTensor(larodConnection* conn,
                                         VdoStream* stream, VdoBuffer* buffer,
                                         larodError** error);

/**
 * @brief Destroy a larodTensor that has been created by @c
 * larodVdoUtilsBufferToTensor().
 *
 * This function is intended to be used as the frame finalizer parameter to
 * @c vdo_stream_new(). It should not be called from other code.
 *
 * @param buffer The larodTensor's matching VdoBuffer.
 */
void larodVdoUtilsDestroyTensor(VdoBuffer* buffer);
