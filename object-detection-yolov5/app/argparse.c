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
 * This file parses the arguments to the application.
 */

#include "argparse.h"

#include "panic.h"

#include <argp.h>
#include <stdlib.h>

#define KEY_USAGE (127)

static int parse_opt(int key, char* arg, struct argp_state* state);

const struct argp_option opts[] = {
    {"device",
     'c',
     "DEVICE",
     0,
     "Chooses device DEVICE to run on, where DEVICE is the enum type larodDevice "
     "from the library. If not specified, the default device for a new "
     "connection will be used.",
     0},
    {"help", 'h', NULL, 0, "Print this help text and exit.", 0},
    {"usage", KEY_USAGE, NULL, 0, "Print short usage message and exit.", 0},
    {0}};
const struct argp argp = {opts, parse_opt, "MODELFILE LABELSFILE", NULL, NULL, NULL, NULL};

void parse_args(int argc, char** argv, args_t* args) {
    if (argp_parse(&argp, argc, argv, ARGP_NO_HELP, NULL, args)) {
        panic("%s: Could not parse arguments", __func__);
    }
}

int parse_opt(int key, char* arg, struct argp_state* state) {
    args_t* args = state->input;

    switch (key) {
        case 'c':
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
                args->labels_file = arg;
            } else {
                argp_error(state, "Too many arguments given");
            }
            break;
        case ARGP_KEY_INIT:
            args->device_name = NULL;
            args->model_file  = NULL;
            args->labels_file = NULL;
            break;
        case ARGP_KEY_END:
            if (state->arg_num != 2) {
                argp_error(state, "Invalid number of arguments given");
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}