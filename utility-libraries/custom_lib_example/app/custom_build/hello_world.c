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
 * - hello_world -
 *
 * This application writes "Hello World!" to the syslog.
 *
 */

#include "hello_world.h"

/***** Print function *********************************************************/

/**
 * brief log_hello_world.
 *
 * The function writes "Hello World!" to the syslog.
 *
 */
void log_hello_world(void)
{
  /* Open the syslog to report messages for "hello_world" */
  openlog("customlib_example", LOG_PID | LOG_CONS, LOG_USER);

  /* Choose between { LOG_INFO, LOG_CRIT, LOG_WARN, LOG_ERR }*/
  syslog(LOG_INFO, "Hello World!");

  /* Close application logging to syslog */
  closelog();
}
