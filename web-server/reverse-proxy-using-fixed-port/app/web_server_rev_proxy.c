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

// civetweb API
#include "civetweb.h"

// GLib core and Unix signal handling
#include <glib-unix.h>
#include <glib.h>

// ISO C standard library headers
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// POSIX / Unix system headers
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#define PORT            "2001"
#define URL_PREFIX      "/local/web_server_rev_proxy/my_web_server"
#define STATIC_FILE_DIR "html"

typedef struct {
    int request_count;
} api_example_data;

__attribute__((noreturn)) __attribute__((format(printf, 1, 2))) static void
panic(const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    vsyslog(LOG_ERR, format, arg);
    va_end(arg);
    exit(1);
}

static gboolean sys_signal_handler(gpointer loop) {
    g_main_loop_quit((GMainLoop*)loop);
    syslog(LOG_INFO, "Application was stopped by SIGTERM or SIGINT.");
    return G_SOURCE_REMOVE;
}

static int static_file_handler(struct mg_connection* conn, void* cb_data __attribute__((unused))) {
    const struct mg_request_info* req_info = mg_get_request_info(conn);
    const char* uri                        = req_info->local_uri;
    size_t prefix_len                      = strlen(URL_PREFIX);

    // Strip prefix from URI
    const char* rel_path = uri;
    if (strncmp(uri, URL_PREFIX, prefix_len) == 0) {
        rel_path = uri + prefix_len;
    }
    if (rel_path[0] == '\0') {
        rel_path = "/";
    }

    char file_path[2048];

    // 1. Try exact file
    snprintf(file_path, sizeof(file_path), "html%s", rel_path);
    struct stat st;
    if (stat(file_path, &st) == 0 && S_ISREG(st.st_mode)) {
        syslog(LOG_INFO, "Serving file: %s", file_path);
        mg_send_file(conn, file_path);
        return 1;
    }

    // 2. Try adding /index.html
    if (rel_path[strlen(rel_path) - 1] == '/') {
        snprintf(file_path, sizeof(file_path), "%s%sindex.html", STATIC_FILE_DIR, rel_path);
    } else {
        snprintf(file_path, sizeof(file_path), "%s%s/index.html", STATIC_FILE_DIR, rel_path);
    }
    if (stat(file_path, &st) == 0 && S_ISREG(st.st_mode)) {
        syslog(LOG_INFO, "Serving file: %s", file_path);
        mg_send_file(conn, file_path);
        return 1;
    }

    // 3. File not found
    return 0;
}

static int api_example_callback(struct mg_connection* conn, void* cb_data) {
    syslog(LOG_INFO, "API callback invoked");
    // Increment request count
    api_example_data* data = (api_example_data*)cb_data;
    data->request_count++;

    // Array of dynamic greetings
    const char greeting[] = "Hello, World!";

    // Build JSON response
    char json_body[128];
    snprintf(json_body,
             sizeof(json_body),
             "{\"message\":\"%s\",\"requestCount\":%d}",
             greeting,
             data->request_count);

    // Send HTTP response
    mg_printf(conn,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: application/json\r\n"
              "Content-Length: %lu\r\n"
              "Connection: close\r\n"
              "\r\n"
              "%s",
              (unsigned long)strlen(json_body),
              json_body);

    return 1;
}

int main(void) {
    GMainLoop* loop = NULL;

    mg_init_library(0);

    struct mg_callbacks callbacks = {0};
    const char* options[] =
        {"listening_ports", PORT, "request_timeout_ms", "10000", "error_log_file", "error.log", 0};

    struct mg_context* context = mg_start(&callbacks, 0, options);
    if (!context) {
        panic("Something went wrong when starting the web server");
    }

    syslog(LOG_INFO, "Server has started");

    api_example_data* data = malloc(sizeof(api_example_data));
    if (!data) {
        syslog(LOG_ERR, "Failed to allocate memory for api_example_data");
        return EXIT_FAILURE;
    }
    data->request_count = 0;
    mg_set_request_handler(context, URL_PREFIX "/api/example", api_example_callback, data);
    mg_set_request_handler(context, URL_PREFIX, static_file_handler, 0);

    // Start listening to callbacks by launching a GLib main loop.
    loop = g_main_loop_new(NULL, FALSE);
    g_unix_signal_add(SIGTERM, sys_signal_handler, loop);
    g_unix_signal_add(SIGINT, sys_signal_handler, loop);
    // Enter main loop
    g_main_loop_run(loop);

    mg_stop(context);
    free(data);
    mg_exit_library();

    return EXIT_SUCCESS;
}
