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

#include "labelparse.h"

#include "panic.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void parse_labels(char*** labels_ptr,
                  char** label_file_buffer,
                  const char* labels_path,
                  size_t* num_labels_ptr) {
    // We cut off every row at 60 characters.
    const size_t LINE_MAX_LEN = 60;
    char* labels_data         = NULL;  // Buffer containing the label file contents.
    char** label_array        = NULL;  // Pointers to each line in the labels text.

    struct stat file_stats = {0};
    if (stat(labels_path, &file_stats) < 0) {
        panic("%s: Unable to get stats for label file %s: %s",
              __func__,
              labels_path,
              strerror(errno));
    }

    // Sanity checking on the file size - we use size_t to keep track of file
    // size and to iterate over the contents. off_t is signed and 32-bit or
    // 64-bit depending on architecture. We just check toward 10 MByte as we
    // will not encounter larger label files and both off_t and size_t should be
    // able to represent 10 megabytes on both 32-bit and 64-bit systems.
    if (file_stats.st_size > (10 * 1024 * 1024)) {
        panic("%s: failed sanity check on labels file size", __func__);
    }

    int labels_fd = open(labels_path, O_RDONLY);
    if (labels_fd < 0) {
        panic("%s: Could not open labels file %s: %s", __func__, labels_path, strerror(errno));
    }

    size_t labels_file_size = (size_t)file_stats.st_size;
    // Allocate room for a terminating NULL char after the last line.
    labels_data = malloc(labels_file_size + 1);
    if (labels_data == NULL) {
        panic("%s: Failed allocating labels text buffer: %s", __func__, strerror(errno));
    }

    ssize_t num_bytes_read  = -1;
    size_t total_bytes_read = 0;
    char* file_read_ptr     = labels_data;
    while (total_bytes_read < labels_file_size) {
        num_bytes_read = read(labels_fd, file_read_ptr, labels_file_size - total_bytes_read);

        if (num_bytes_read < 1) {
            panic("%s: Failed reading from labels file: %s", __func__, strerror(errno));
        }
        total_bytes_read += (size_t)num_bytes_read;
        file_read_ptr += num_bytes_read;
    }

    // Now count number of lines in the file - check all bytes except the last
    // one in the file.
    size_t num_lines = 0;
    for (size_t i = 0; i < (labels_file_size - 1); i++) {
        if (labels_data[i] == '\n') {
            num_lines++;
        }
    }

    // We assume that there is always a line at the end of the file, possibly
    // terminated by newline char. Either way add this line as well to the
    // counter.
    num_lines++;

    label_array = malloc(num_lines * sizeof(char*));
    if (!label_array) {
        panic("%s: Unable to allocate labels array: %s", __func__, strerror(errno));
    }

    size_t label_idx       = 0;
    label_array[label_idx] = labels_data;
    label_idx++;
    for (size_t i = 0; i < labels_file_size; i++) {
        if (labels_data[i] == '\n') {
            if (i < (labels_file_size - 1)) {
                // Register the string start in the list of labels.
                label_array[label_idx] = labels_data + i + 1;
                label_idx++;
            }
            // Replace the newline char with string-ending NULL char.
            labels_data[i] = '\0';
        }
    }

    // Make sure we always have a terminating NULL char after the label file
    // contents.
    labels_data[labels_file_size] = '\0';

    // Now go through the list of strings and cap if strings too long.
    for (size_t i = 0; i < num_lines; i++) {
        size_t string_len = strnlen(label_array[i], LINE_MAX_LEN);
        if (string_len >= LINE_MAX_LEN) {
            // Just insert capping NULL terminator to limit the string len.
            *(label_array[i] + LINE_MAX_LEN + 1) = '\0';
        }
    }

    *labels_ptr        = label_array;
    *num_labels_ptr    = num_lines;
    *label_file_buffer = labels_data;

    close(labels_fd);
}