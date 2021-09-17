/**
 * Copyright (C) 2018-2020, Axis Communications AB, Lund, Sweden
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

/**
 * This header file parses the arguments to the application.
 */

#pragma once

#include <stddef.h>

#include "larod.h"

typedef struct args_t {
    unsigned quality;
    char* modelFile;
    char* labelsFile;
    unsigned width;
    unsigned height;
    unsigned raw_width;
    unsigned raw_height;
    unsigned threshold;
    larodChip chip;
} args_t;

bool parseArgs(int argc, char** argv, args_t* args);
