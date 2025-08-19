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

#include "civetweb.h"
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#define PORT "2001"
volatile sig_atomic_t application_running = 1;

__attribute__((noreturn)) __attribute__((format(printf, 1, 2))) static void
panic(const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    vsyslog(LOG_ERR, format, arg);
    va_end(arg);
    exit(1);
}

static void stop_application(int status) {
    (void)status;
    application_running = 0;
}

static int root_handler(struct mg_connection* conn, void* cb_data __attribute__((unused))) {
    mg_send_file(conn, "html/index.html");
    return 1;
}

int main(void) {
    signal(SIGTERM, stop_application);
    signal(SIGINT, stop_application);

    mg_init_library(0);

    struct mg_callbacks callbacks = {0};
    const char* options[] =
        {"listening_ports", PORT, "request_timeout_ms", "10000", "error_log_file", "error.log", 0};

    struct mg_context* context = mg_start(&callbacks, 0, options);
    if (!context) {
        panic("Something went wrong when starting the web server");
    }

    syslog(LOG_INFO, "Server has started");

    mg_set_request_handler(context, "/", root_handler, 0);

    while (application_running) {
        sleep(1);
    }

    mg_stop(context);
    mg_exit_library();

    return EXIT_SUCCESS;
}
