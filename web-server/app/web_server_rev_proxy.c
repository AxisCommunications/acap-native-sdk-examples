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

#include <glib-unix.h>
#include <glib.h>
#include <libmonkey.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#define APP_NAME "web_server_rev_proxy"

static char html_buffer[8192];

static gboolean signal_handler(gpointer loop) {
    g_main_loop_quit((GMainLoop*)loop);
    syslog(LOG_INFO, "Application was stopped by SIGTERM or SIGINT.");
    return G_SOURCE_REMOVE;
}

// Print an error to syslog and exit the application if a fatal error occurs.
__attribute__((noreturn)) __attribute__((format(printf, 1, 2))) static void
panic(const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    vsyslog(LOG_ERR, format, arg);
    va_end(arg);
    exit(1);
}

// Compose a simple HTML page
static int serve_web_page(const mklib_session* session,
                          const char* vhost,
                          const char* cgi_path,
                          const char* get,
                          unsigned long get_len,
                          const char* post,
                          unsigned long post_len,
                          unsigned int* status,
                          const char** content,
                          unsigned long* content_len,
                          char* header) {
    (void)session;
    (void)vhost;
    (void)get;
    (void)get_len;
    (void)post;
    (void)post_len;
    (void)status;
    time_t t;

    // Register page type
    sprintf(header, "Content-type: text/html");

    // Compose page content start
    sprintf(html_buffer, "<html><body>");

    // Compose page body
    strcat(html_buffer, "<h1>ACAP application with reverse proxy web server</h1>");
    strcat(html_buffer, "<pre>");
    strcat(html_buffer, "<br>Application name:      ");
    strcat(html_buffer, APP_NAME);
    strcat(html_buffer, "<br>Reverse proxy path:    ");
    strcat(html_buffer, cgi_path);
    strcat(html_buffer, "<br>Request timestamp:     ");
    time(&t);
    strcat(html_buffer, ctime(&t));

    // Compose page content end
    strcat(html_buffer, "</pre>");
    strcat(html_buffer, "</body></html>");

    // Write buffer to callback pointer
    *content     = html_buffer;
    *content_len = strlen(html_buffer);

    return MKLIB_TRUE;
}

static cb_data callback_function = serve_web_page;

int main(void) {
    GMainLoop* loop = NULL;

    openlog(APP_NAME, LOG_PID, LOG_USER);
    loop = g_main_loop_new(NULL, FALSE);

    mklib_ctx context = mklib_init(NULL, 0, 0, NULL);
    if (!context)
        panic("Could not crete web server context, mklib_init failed.");

    mklib_callback_set(context, MKCB_DATA, callback_function);
    syslog(LOG_INFO, "Callback has been registered");

    mklib_start(context);
    syslog(LOG_INFO, "Server has started");

    g_unix_signal_add(SIGTERM, signal_handler, loop);
    g_unix_signal_add(SIGINT, signal_handler, loop);
    g_main_loop_run(loop);

    mklib_stop(context);
    g_main_loop_unref(loop);
}
