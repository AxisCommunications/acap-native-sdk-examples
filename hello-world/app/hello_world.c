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
 * - hello_world -
 *
 * This application writes "Hello World!" to the syslog.
 *
 */

#include <syslog.h>

/***** Main function *********************************************************/

/**
 * brief Main function.
 *
 * This main function writes "hello_world" to the syslog.
 */
int main(void) {
    /* Open the syslog to report messages for "hello_world" */
    openlog("hello_world", LOG_PID | LOG_CONS, LOG_USER);

    /* Choose between { LOG_INFO, LOG_CRIT, LOG_WARN, LOG_ERR }*/
    syslog(LOG_INFO, "Hello World!");
}
