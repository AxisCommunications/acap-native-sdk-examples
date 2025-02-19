/**
 * Copyright (C) 2024 Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     <http://www.apache.org/licenses/LICENSE-2.0>
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * This example creates a Message Broker subscriber for the
 * analytics_scene_description topic. Streamed messages are received in the
 * Analytics Data Format (ADF) and is logged to syslog.
 */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/syslog.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <mdb/connection.h>
#include <mdb/error.h>
#include <mdb/subscriber.h>

typedef struct channel_identifier {
    char* topic;
    char* source;
} channel_identifier_t;

static void on_connection_error(const mdb_error_t* error, void* user_data) {
    (void)user_data;

    syslog(LOG_ERR, "Got connection error: %s, Aborting...", error->message);

    abort();
}

static void on_message(const mdb_message_t* message, void* user_data) {
    const struct timespec* timestamp     = mdb_message_get_timestamp(message);
    const mdb_message_payload_t* payload = mdb_message_get_payload(message);

    channel_identifier_t* channel_identifier = (channel_identifier_t*)user_data;

    syslog(LOG_INFO,
           "message received from topic: %s on source: %s: Monotonic time - "
           "%lld.%.9ld. Data - %.*s",
           channel_identifier->topic,
           channel_identifier->source,
           (long long)timestamp->tv_sec,
           timestamp->tv_nsec,
           (int)payload->size,
           (char*)payload->data);
}

static void on_done_subscriber_create(const mdb_error_t* error, void* user_data) {
    if (error != NULL) {
        syslog(LOG_ERR, "Got subscription error: %s, Aborting...", error->message);
        abort();
    }
    channel_identifier_t* channel_identifier = (channel_identifier_t*)user_data;

    syslog(LOG_INFO,
           "Subscribed to %s (%s)...",
           channel_identifier->topic,
           channel_identifier->source);
}

static void sig_handler(int signum) {
    (void)signum;
    // Do nothing, just let pause in main return.
}

int main(void) {
    syslog(LOG_INFO, "Subscriber started...");

    // For com.axis.analytics_scene_description.v0.beta source corresponds to the
    // video channel number.
    channel_identifier_t channel_identifier    = {.topic =
                                                      "com.axis.analytics_scene_description.v0.beta",
                                                  .source = "1"};
    mdb_error_t* error                         = NULL;
    mdb_subscriber_config_t* subscriber_config = NULL;
    mdb_subscriber_t* subscriber               = NULL;

    mdb_connection_t* connection = mdb_connection_create(on_connection_error, NULL, &error);
    if (error != NULL) {
        goto end;
    }

    subscriber_config = mdb_subscriber_config_create(channel_identifier.topic,
                                                     channel_identifier.source,
                                                     on_message,
                                                     &channel_identifier,
                                                     &error);
    if (error != NULL) {
        goto end;
    }

    subscriber = mdb_subscriber_create_async(connection,
                                             subscriber_config,
                                             on_done_subscriber_create,
                                             &channel_identifier,
                                             &error);
    if (error != NULL) {
        goto end;
    }

    // Add signal handler to allow for cleanup on ordered termination.
    (void)signal(SIGTERM, sig_handler);

    pause();

end:
    if (error != NULL) {
        syslog(LOG_ERR, "%s", error->message);
    }

    mdb_error_destroy(&error);
    mdb_subscriber_config_destroy(&subscriber_config);
    mdb_subscriber_destroy(&subscriber);
    mdb_connection_destroy(&connection);

    syslog(LOG_INFO, "Subscriber closed...");
}
