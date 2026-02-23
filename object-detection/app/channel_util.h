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
 * This header file handles the vdo part of the application.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "vdo-error.h"
#include "vdo-stream.h"
#include "vdo-types.h"

bool channel_util_choose_stream_resolution(unsigned int channel,
                                           VdoResolution req_res,
                                           VdoResolution* chosen_req,
                                           unsigned int rotation,
                                           VdoFormat* chosen_format);

unsigned int channel_util_get_image_rotation(unsigned int input_channel);
unsigned int channel_util_get_first_input_channel(void);
VdoPair32u channel_util_get_aspect_ratio(unsigned int channel_id);
