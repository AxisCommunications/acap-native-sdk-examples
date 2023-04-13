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


#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

/**
 * brief Saves an RGB image as a PPM file.
 *
 * This function takes an RGB image stored in a buffer, along with its width and height,
 * and saves it as a PPM (Portable Pixmap) file with the specified filename.
 *
 * param rgbData Pointer to the buffer containing the RGB image data.
 * param width The width of the image in pixels.
 * param height The height of the image in pixels.
 * param filename The name of the output PPM file to be created.
 */
void saveRgbImageAsPpm(const uint8_t* rgbData, int width, int height, const char* filename);

/**
 * brief Creates a temporary fd truncated to correct size and mapped.
 *
 * This convenience function creates temp files to be used for input and output.
 *
 * param fileName Pattern for how the temp file will be named in file system.
 * param fileSize How much space needed to be allocated (truncated) in fd.
 * param mappedAddr Pointer to the address of the fd mapped for this process.
 * param Pointer to the generated fd.
 * return False if any errors occur, otherwise true.
 */
bool createAndMapTmpFile(char* fileName, size_t fileSize,
                                void** mappedAddr, int* convFd);




