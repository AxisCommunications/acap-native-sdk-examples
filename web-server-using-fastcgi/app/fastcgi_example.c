/**
 * Copyright (C) 2022 Axis Communications AB, Lund, Sweden
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

#include "fcgi_stdio.h"
#include "uriparser/Uri.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <syslog.h>

#define FCGI_SOCKET_NAME "FCGI_SOCKET_NAME"

/**
 * brief Initialize fastcgi and request handling.
 *
 * Set up fastcgi and define how an HTTP request should be handled.
 *
 * return EXIT_FAILURE if any errors occur, otherwise EXIT_SUCCESS.
 */

int fcgi_run() {
    int count = 0;  // counter of requests
    int sock;
    FCGX_Request request;
    char* socket_path = NULL;
    int status;

    openlog(NULL, LOG_PID, LOG_DAEMON);
    socket_path = getenv(FCGI_SOCKET_NAME);

    if (!socket_path) {
        syslog(LOG_ERR, "Failed to get environment variable FCGI_SOCKET_NAME");
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "Socket: %s\n", socket_path);

    status = FCGX_Init();

    if (status != 0) {
        syslog(LOG_INFO, "FCGX_Init failed");
        return status;
    }

    sock = FCGX_OpenSocket(socket_path, 5);
    chmod(socket_path, S_IRWXU | S_IRWXG | S_IRWXO);
    status = FCGX_InitRequest(&request, sock, 0);

    if (status != 0) {
        syslog(LOG_INFO, "FCGX_InitRequest failed");
        return status;
    }

    syslog(LOG_INFO, "Starting loop");

    while (FCGX_Accept_r(&request) == 0) {
        syslog(LOG_INFO, "FCGX_Accept_r OK");
        // Write the HTTP header
        FCGX_FPrintF(request.out, "Content-Type: text/html\n\n");
        // Write the HTML greeting
        FCGX_FPrintF(request.out, "<h1>Hello ");

        // Parse the uri and the query string
        char* uriString = FCGX_GetParam("REQUEST_URI", request.envp);

        UriUriA uri;
        UriQueryListA* queryList;
        int itemCount;
        const char* errorPos;

        syslog(LOG_INFO, "Parsing URI: %s", uriString);

        // Parse the URI into data structure
        if (uriParseSingleUriA(&uri, uriString, &errorPos) != URI_SUCCESS) {
            /* Failure (no need to call uriFreeUriMembersA) */
            FCGX_FPrintF(request.out, "Failed to parse URI");
        }

        syslog(LOG_INFO, "URI: %s", uriString);

        // Parse the query string into data structure
        if (uriDissectQueryMallocA(&queryList, &itemCount, uri.query.first, uri.query.afterLast) !=
            URI_SUCCESS) {
            /* Failure */
            FCGX_FPrintF(request.out, "Failed to parse query");
        }

        // Find and print the name parameter in the query string
        UriQueryListA* queryItem = queryList;

        while (queryItem) {
            if (strcmp(queryItem->key, "name") == 0 && queryItem->value != NULL) {
                FCGX_FPrintF(request.out, "%s", queryItem->value);
            }
            queryItem = queryItem->next;
        }

        // print the rest of the body
        FCGX_FPrintF(request.out, " from FastCGI</h1> Request number %d", ++count);
        FCGX_FPrintF(request.out, "<br>URI: ");
        FCGX_FPrintF(request.out, (uriString ? uriString : "NULL"));
        FCGX_FPrintF(request.out, "<br>KEY, ITEM: ");

        queryItem = queryList;

        while (queryItem) {
            if (queryItem->value != NULL) {
                FCGX_FPrintF(request.out, "<br>%s, %s", queryItem->key, queryItem->value);
            } else {
                FCGX_FPrintF(request.out, "<br>%s, Null", queryItem->key);
            }
            queryItem = queryItem->next;
        }

        FCGX_Finish_r(&request);
        uriFreeUriMembersA(&uri);
        uriFreeQueryListA(queryList);
    }

    return EXIT_SUCCESS;
}

int main() {
    int ret;
    // Program loop
    ret = fcgi_run();

    return ret;
}
