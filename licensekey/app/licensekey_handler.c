/**
 * Copyright (C) 2021, Axis Communications AB, Lund, Sweden
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
 * - licensekey_handler -
 *
 * This application is a basic licensekey application which does
 * a license key check for a specific application name, application id,
 * major and minor application version.
 *
 */
#include <glib-unix.h>
#include <glib.h>
#include <licensekey.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#define APP_ID        0
#define MAJOR_VERSION 1
#define MINOR_VERSION 0

// This is a very simplistic example, checking every 5 minutes
#define CHECK_SECS 300

static gchar* glob_app_name = NULL;

/**
 * @brief Handles the signals.
 *
 * @param loop Loop to quit
 */
static gboolean signal_handler(gpointer loop) {
    g_main_loop_quit((GMainLoop*)loop);
    syslog(LOG_INFO, "Application was stopped by SIGTERM or SIGINT.");
    return G_SOURCE_REMOVE;
}

// Checks licensekey status every 5th minute
static gboolean check_license_status(void* data) {
    (void)data;
    if (licensekey_verify(glob_app_name, APP_ID, MAJOR_VERSION, MINOR_VERSION) != 1) {
        syslog(LOG_INFO, "Licensekey is invalid");
    } else {
        syslog(LOG_INFO, "Licensekey is valid");
    }
    return TRUE;
}

/**
 * Main function
 */
int main(int argc, char* argv[]) {
    GMainLoop* loop;
    if (argc != 1)
        return EXIT_FAILURE;

    glob_app_name = g_path_get_basename(argv[0]);
    openlog(glob_app_name, LOG_PID | LOG_CONS, LOG_USER);
    loop = g_main_loop_new(NULL, FALSE);

    check_license_status(NULL);
    g_timeout_add_seconds(CHECK_SECS, check_license_status, NULL);

    g_unix_signal_add(SIGTERM, signal_handler, loop);
    g_unix_signal_add(SIGINT, signal_handler, loop);
    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    g_free(glob_app_name);

    return EXIT_SUCCESS;
}
