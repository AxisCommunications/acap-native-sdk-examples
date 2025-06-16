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

static void stop_application(int status) {
    (void)status;
    application_running = 0;
}

static int RootHandler(struct mg_connection* conn, void* cb_data __attribute__((unused))) {
    mg_printf(conn,
              "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
              "close\r\n\r\n");
    FILE* html_file   = fopen("html/index.html", "r");
    int FILE_STR_SIZE = 128;
    char file_str[FILE_STR_SIZE];
    while (fgets(file_str, FILE_STR_SIZE, html_file)) {
        mg_printf(conn, "%s", file_str);
    }
    fclose(html_file);
    return 1;
}

int main(void) {
    signal(SIGTERM, stop_application);
    signal(SIGINT, stop_application);

    const char* options[] =
        {"listening_ports", PORT, "request_timeout_ms", "10000", "error_log_file", "error.log", 0};

    struct mg_callbacks callbacks;
    struct mg_context* context;

    mg_init_library(0);

    memset(&callbacks, 0, sizeof(callbacks));

    context = mg_start(&callbacks, 0, options);
    syslog(LOG_INFO, "Server has started");

    mg_set_request_handler(context, "/", RootHandler, 0);

    if (context == NULL) {
        syslog(LOG_INFO, "Something went wrong when starting the web server.\n");
        return EXIT_FAILURE;
    }

    while (application_running) {
        sleep(1);
    }

    return EXIT_SUCCESS;
}
