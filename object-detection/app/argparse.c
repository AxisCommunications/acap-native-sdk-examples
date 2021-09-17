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
 * This file parses the arguments to the application.
 */

#include "argparse.h"

#include <argp.h>
#include <stdlib.h>

#define KEY_USAGE (127)

static int parsePosInt(char* arg, unsigned long long* i,
                       unsigned long long limit);
static int parseOpt(int key, char* arg, struct argp_state* state);

const struct argp_option opts[] = {
    {"chip", 'c', "CHIP", 0,
     "Chooses chip CHIP to run on, where CHIP is the enum type larodChip "
     "from the library. If not specified, the default chip for a new "
     "connection will be used.",
     0},
    {"help", 'h', NULL, 0, "Print this help text and exit.", 0},
    {"usage", KEY_USAGE, NULL, 0, "Print short usage message and exit.", 0},
    {0}};
const struct argp argp = {
    opts,
    parseOpt,
    "MODEL WIDTH HEIGHT QUALITY RAW_WIDTH RAW_HEIGHT THRESHOLD LABELSFILE",
    "This is an example app which loads an object detection MODEL to "
    "larod and then uses vdo to fetch frames of size WIDTH x HEIGHT in yuv "
    "format which are converted to interleaved rgb format and then sent to "
    "larod for inference on MODEL. RAW_WIDTH x RAW_HEIGHT is the original "
    "resolution of frames from the camera. QUALITY denotes the desired jpeg "
    "image quality ranging from 0 to 100. THRESHOLD ranging from 0 to 100 is the "
    "min score required to show the detected objects and crop them. LABELSFILE "
    "is the path of a txt where labes names are saved. \n\nExample call: "
    "\n/usr/local/packages/object_detection/model/converted_model.tflite 300 "
    "300 80 1920 1080 50 /usr/local/packages/object_detection/label/labels.txt "
    "-c 4 \nwhere 4 here refers to the Edge TPU backend. The numbers for "
    "each type of chip can be found at the top of the file larod.h.",
    NULL,
    NULL,
    NULL};

bool parseArgs(int argc, char** argv, args_t* args) {
    if (argp_parse(&argp, argc, argv, ARGP_NO_HELP, NULL, args)) {
        return false;
    }
    return true;
}

int parseOpt(int key, char* arg, struct argp_state* state) {
    args_t* args = state->input;

    switch (key) {
    case 'c': {
        unsigned long long chip;
        int ret = parsePosInt(arg, &chip, INT_MAX);
        if (ret) {
            argp_failure(state, EXIT_FAILURE, ret, "invalid chip type");
        }
        args->chip = (larodChip) chip;
        break;
    }
    case 'h':
        argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
        break;
    case KEY_USAGE:
        argp_state_help(state, stdout, ARGP_HELP_USAGE | ARGP_HELP_EXIT_OK);
        break;
    case ARGP_KEY_ARG:
        if (state->arg_num == 0) {
            args->modelFile = arg;
        } else if (state->arg_num == 1) {
            unsigned long long width;
            int ret = parsePosInt(arg, &width, UINT_MAX);
            if (ret) {
                argp_failure(state, EXIT_FAILURE, ret, "invalid width");
            }
            args->width = (unsigned int) width;
        } else if (state->arg_num == 2) {
            unsigned long long height;
            int ret = parsePosInt(arg, &height, UINT_MAX);
            if (ret) {
                argp_failure(state, EXIT_FAILURE, ret, "invalid height");
            }
            args->height = (unsigned int) height;
        } else if (state->arg_num == 3) {
            unsigned long long quality;
            int ret = parsePosInt(arg, &quality, UINT_MAX);
            if (ret) {
                argp_failure(state, EXIT_FAILURE, ret, "invalid quality");
            }
            args->quality = (unsigned int) quality;
        } else if (state->arg_num == 4) {
            unsigned long long raw_width;
            int ret = parsePosInt(arg, &raw_width, UINT_MAX);
            if (ret) {
                argp_failure(state, EXIT_FAILURE, ret, "invalid raw_width");
            }
            args->raw_width = (unsigned int) raw_width;
        } else if (state->arg_num == 5) {
            unsigned long long raw_height;
            int ret = parsePosInt(arg, &raw_height, UINT_MAX);
            if (ret) {
                argp_failure(state, EXIT_FAILURE, ret, "invalid raw_height");
            }
            args->raw_height = (unsigned int) raw_height;
        } else if (state->arg_num == 6) {
            unsigned long long threshold;
            int ret = parsePosInt(arg, &threshold, UINT_MAX);
            if (ret) {
                argp_failure(state, EXIT_FAILURE, ret, "invalid threshold");
            }
            args->threshold = (unsigned int) threshold;
        } else if (state->arg_num == 7) {
            args->labelsFile = arg;
        } else {
            argp_error(state, "Too many arguments given");
        }
        break;
    case ARGP_KEY_INIT:
        args->width = 0;
        args->height = 0;
        args->quality = 0;
        args->raw_width = 0;
        args->raw_height = 0;
        args->threshold = 0;
        args->chip = 0;
        args->modelFile = NULL;
        args->labelsFile = NULL;
        break;
    case ARGP_KEY_END:
        if (state->arg_num != 8) {
            argp_error(state, "Invalid number of arguments given");
        }
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

/**
 * brief Parses a string as an unsigned long long
 *
 * param arg String to parse.
 * param i Pointer to the number being the result of parsing.
 * param limit Max limit for data type integer will be saved to.
 * return Positive errno style return code (zero means success).
 */
static int parsePosInt(char* arg, unsigned long long* i,
                       unsigned long long limit) {
    char* endPtr;

    *i = strtoull(arg, &endPtr, 0);
    if (*endPtr != '\0') {
        return EINVAL;
    } else if (arg[0] == '-' || *i == 0) {
        return EINVAL;
        // Make sure we don't overflow when casting.
    } else if (*i == ULLONG_MAX || *i > limit) {
        return ERANGE;
    }

    return 0;
}
