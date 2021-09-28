/**
 * Copyright (C) 2021, Axis Communications AB, Lund, Sweden
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
 * - licensekey_handler -
 *
 * This application is a basic licensekey application which does
 * a license key check for a specific application name, application id,
 * major and minor application version.
 *
 */ 
#include <glib.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdio.h>
#include <licensekey.h>

#define APP_ID           0
#define MAJOR_VERSION    1
#define MINOR_VERSION    0

// This is a very simplistic example, checking every 5 minutes
#define CHECK_SECS       300

static gchar *glob_app_name = NULL;

// Checks licensekey status every 5th minute
gboolean
check_license_status(void *data)
{
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
int
main(int argc, char *argv[])
{
  GMainLoop *loop;

  glob_app_name = g_path_get_basename(argv[0]);
  openlog(glob_app_name, LOG_PID | LOG_CONS, LOG_USER);
  loop = g_main_loop_new(NULL, FALSE);

  check_license_status(NULL);
  g_timeout_add_seconds(CHECK_SECS, check_license_status, NULL);

  g_main_loop_run(loop);
  return EXIT_SUCCESS;
}
