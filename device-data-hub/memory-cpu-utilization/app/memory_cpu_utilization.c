// Copyright (C) 2026 Axis Communications AB, Lund, Sweden
// Licensed under the MIT License. See LICENSE file for details.

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include <datahub/client.h>
#include <datahub/subscriber.h>

#define MEM_UTIL_TOPIC "com.axis.device.memory-utilization.v1"
#define CPU_UTIL_TOPIC "com.axis.device.cpu-utilization.v1"

// Global state
static volatile sig_atomic_t keep_running = true;
static DHClient* client                   = NULL;
static DHSubscriber* data_subscriber      = NULL;

// Forward declarations
static void cleanup_resources(void);
static bool handle_client_error(DHError* err, const char* context);
static bool initialize_client(void);
static bool setup_subscription(const char* topics[], unsigned int topics_count);
static void signal_handler(int sig);
static void on_data_received(const DHTopicSample* sample, void* user_data);

// error handling
static bool handle_client_error(DHError* err, const char* context) {
    if (err) {
        syslog(LOG_ERR, "Error in %s: %s\n", context, dh_error_to_string(err));
        dh_error_destroy(err);
        return true;
    }
    return false;
}

// Initialize client and connect
static bool initialize_client(void) {
    DHError* conn_cb_err = NULL;
    client               = dh_client_create("Client for memory-cpu-utilization", &conn_cb_err);
    if (!client && !handle_client_error(conn_cb_err, "create client")) {
        return false;
    }

    conn_cb_err = NULL;
    if (!dh_client_set_logging(client, DH_LOG_INFO, DH_LOG_TARGET_CONSOLE, &conn_cb_err)) {
        handle_client_error(conn_cb_err, "set logging");
    }

    DHError* err = NULL;
    dh_client_connect(client, &err);
    if (handle_client_error(err, "client connect")) {
        return false;
    }

    return true;
}

// Data received callback
static void on_data_received(const DHTopicSample* sample, void* user_data) {
    syslog(LOG_INFO, "User data: %s\n", (const char*)user_data);
    const DHTopicData* topic_data = dh_topic_sample_get_data(sample);
    const char* data              = dh_topic_data_get_json_data(topic_data);
    if (data) {
        syslog(LOG_INFO, "Received mem cpu data: %s\n", data);
    }
}

// Subscribe to a topic
static bool setup_subscription(const char* topics[], unsigned int topics_count) {
    // Create subscriber
    DHError* err = NULL;
    data_subscriber =
        dh_client_create_subscriber(client, "Data subscriber for memory-cpu-utilization", &err);
    if (handle_client_error(err, "create subscriber")) {
        return false;
    }

    // Register data callback
    dh_subscriber_set_data_callback(data_subscriber,
                                    on_data_received,
                                    "memory_cpu_utilization_data",
                                    &err);
    if (handle_client_error(err, "set data callback")) {
        return false;
    }

    // Build subscription filter
    DHFilter* filter = dh_filter_create();
    if (!filter) {
        syslog(LOG_ERR, "Failed to create filter");
        return false;
    }

    // Add topic names to filter
    for (unsigned int i = 0; i < topics_count; i++) {
        dh_filter_add_topic_name(filter, topics[i], &err);
        if (handle_client_error(err, "add topic name to filter")) {
            dh_filter_destroy(filter);
            return false;
        }
    }

    // Create subscription options and add filter
    DHSubscribeOptions* options = dh_subscribe_options_create();
    if (!options) {
        syslog(LOG_ERR, "Failed to create subscription options");
        dh_filter_destroy(filter);
        return false;
    }

    dh_subscribe_options_add_filter(options, filter, &err);
    dh_filter_destroy(filter);
    if (handle_client_error(err, "add filter to options")) {
        dh_subscribe_options_destroy(options);
        return false;
    }

    dh_subscribe_options_set_enable_data_updates(options, true);

    // Subscribe with the above configured options
    dh_subscriber_subscribe(data_subscriber, options, &err);
    dh_subscribe_options_destroy(options);
    if (handle_client_error(err, "subscribe to topic")) {
        return false;
    }

    return true;
}

// Cleanup all allocated resources
static void cleanup_resources(void) {
    if (data_subscriber) {
        dh_subscriber_destroy(data_subscriber);
        data_subscriber = NULL;
    }

    if (client) {
        DHError* err = NULL;
        dh_client_disconnect(client, &err);
        handle_client_error(err, "client disconnect");
        dh_client_destroy(client);
        client = NULL;
    }
}

// Signal handler for graceful shutdown
static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        keep_running = false;
    }
}

int main(void) {
    syslog(LOG_INFO, "Application started");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    atexit(cleanup_resources);

    const char* topics[]      = {MEM_UTIL_TOPIC, CPU_UTIL_TOPIC};
    unsigned int topics_count = sizeof(topics) / sizeof(topics[0]);

    if (!initialize_client() || !setup_subscription(topics, topics_count)) {
        return EXIT_FAILURE;
    }

    while (keep_running) {
        pause();
    }

    syslog(LOG_INFO, "Application terminated");
    return EXIT_SUCCESS;
}
