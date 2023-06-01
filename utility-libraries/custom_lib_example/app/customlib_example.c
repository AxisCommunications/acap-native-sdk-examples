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
 * - customlib_example -
 *
 * This application calls a custom library function and prints Hello World!
 *
 */

#include <hello_world.h>

/***** Main function *********************************************************/

/**
 * brief log_hello_world.
 *
 * It writes "Hello World!" to the syslog through custom lib.
 *
 */
int main(void)
{
  log_hello_world();
  return 0;
}
