/**
 * Copyright (C) 2018-2021, Axis Communications AB, Lund, Sweden
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
 * This file parses the arguments to the application.
 */

#include "argparse.h"

#include "panic.h"

#include <argp.h>
#include <stdlib.h>

#define KEY_USAGE (127)

static int parse_pos_int(char* arg, unsigned long long* i, unsigned long long limit);
static int parse_opt(int key, char* arg, struct argp_state* state);

const struct argp_option opts[] = {
    {"device",
     'd',
     "DEVICE",
     0,
     "Could be axis-a8-dlpu-tflite, a9-dlpu-tflite, google-edge-tpu-tflite or cpu-tflite",
     0},
    {"help", 'h', NULL, 0, "Print this help text and exit.", 0},
    {"usage", KEY_USAGE, NULL, 0, "Print short usage message and exit.", 0},
    {0}};
const struct argp argp = {opts,
                          parse_opt,
                          "MODEL THRESHOLD LABELSFILE",
                          "This is an example app which loads an object detection MODEL to "
                          "larod and then uses vdo to fetch frames in yuv or rgb"
                          "format which are converted if needed to rgb."
                          "and then sent to larod for inference on MODEL."
                          "THRESHOLD ranging from 0 to 100 is the "
                          "min score required to show the detected objects."
                          "LABELSFILE is the path of a txt where labes names are saved."
                          "\n\nExample call: "
                          "\n/usr/local/packages/object_detection/model/model.tflite"
                          "80 /usr/local/packages/object_detection/label/labels.txt ",
                          NULL,
                          NULL,
                          NULL};

void parse_args(int argc, char** argv, args_t* args) {
    if (argp_parse(&argp, argc, argv, ARGP_NO_HELP, NULL, args)) {
        panic("%s: Could not parse arguments", __func__);
    }
}

int parse_opt(int key, char* arg, struct argp_state* state) {
    args_t* args = state->input;

    switch (key) {
        case 'd':
            args->device_name = arg;
            break;
        case 'h':
            argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
            break;
        case KEY_USAGE:
            argp_state_help(state, stdout, ARGP_HELP_USAGE | ARGP_HELP_EXIT_OK);
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num == 0) {
                args->model_file = arg;
            } else if (state->arg_num == 1) {
                unsigned long long threshold;
                int ret = parse_pos_int(arg, &threshold, UINT_MAX);
                if (ret) {
                    argp_failure(state, EXIT_FAILURE, ret, "invalid threshold");
                }
                args->threshold = (unsigned int)threshold;
            } else if (state->arg_num == 2) {
                args->labels_file = arg;
            } else {
                argp_error(state, "Too many arguments given");
            }
            break;
        case ARGP_KEY_INIT:
            args->threshold   = 0;
            args->device_name = NULL;
            args->model_file  = NULL;
            args->labels_file = NULL;
            break;
        case ARGP_KEY_END:
            if (state->arg_num < 1 || state->arg_num > 3) {
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
static int parse_pos_int(char* arg, unsigned long long* i, unsigned long long limit) {
    char* end_p;

    *i = strtoull(arg, &end_p, 0);
    if (*end_p != '\0') {
        return EINVAL;
    } else if (arg[0] == '-' || *i == 0) {
        return EINVAL;
        // Make sure we don't overflow when casting.
    } else if (*i == ULLONG_MAX || *i > limit) {
        return ERANGE;
    }

    return 0;
}
