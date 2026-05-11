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

#define TOPIC_NAME "com.axis.scene.object_track.v1"

// Global state
static volatile sig_atomic_t keep_running = true;
static DHClient* client                   = NULL;
static DHSubscriber* data_subscriber      = NULL;

// Forward declarations
static void cleanup_resources(void);
static bool handle_client_error(DHClientError* err, const char* context);
static bool initialize_client(void);
static bool setup_subscription(const char* topics[], unsigned int topics_count);
static void signal_handler(int sig);
static void on_data_received(DHTopicSample* sample, void* user_data);

// error handling
static bool handle_client_error(DHClientError* err, const char* context) {
    if (err) {
        syslog(LOG_ERR, "Error in %s: %s\n", context, dh_client_error_to_string(err));
        dh_client_destroy_error(err);
        return true;
    }
    return false;
}

// Initialize client and connect
static bool initialize_client(void) {
    DHClientOptions* opts = dh_client_options_create();
    dh_client_options_set_log_level(opts, DH_LOG_INFO);
    dh_client_options_set_log_target(opts, DH_LOG_TARGET_CONSOLE);

    client = dh_client_create("Client for consume-scene-metadata", opts);
    dh_client_options_destroy(opts);
    if (!client) {
        syslog(LOG_ERR, "Failed to create client");
        return false;
    }

    DHClientError* err = NULL;
    dh_client_connect(client, &err);
    if (handle_client_error(err, "client connect")) {
        return false;
    }

    return true;
}

// Data received callback
static void on_data_received(DHTopicSample* sample, void* user_data) {
    syslog(LOG_INFO, "User data: %s\n", (const char*)user_data);
    const DHTopicData* topic_data = dh_topic_sample_get_topic_data(sample);
    const char* data              = dh_topic_data_get_json_str(topic_data);
    if (data) {
        syslog(LOG_INFO, "Received consumer-scene-metadata: %s\n", data);
    }
}

// Subscribe to a topic
static bool setup_subscription(const char* topics[], unsigned int topics_count) {
    // Create subscriber
    DHClientError* err = NULL;
    data_subscriber =
        dh_client_create_subscriber(client, "Data subscriber for consumer-scene-metadata", &err);
    if (handle_client_error(err, "create subscriber")) {
        return false;
    }

    // Set up listener callbacks
    DHSubscriberListener* sub_listener =
        dh_subscriber_listener_create(NULL, NULL, on_data_received, "consume_scene_metadata_data");
    if (!sub_listener) {
        syslog(LOG_ERR, "Failed to create subscriber listener");
        return false;
    }

    dh_subscriber_set_listener(data_subscriber, sub_listener, NULL);
    dh_subscriber_listener_destroy(sub_listener);

    // Subscribe to topic
    DHSubscriptionConfig* updates = dh_subscription_config_create();
    if (!updates) {
        syslog(LOG_ERR, "Failed to create subscription update config");
        return false;
    }
    dh_subscription_config_set(updates, false, false, true);

    // Create instance key to only receive data for channel_id 1
    DHTopicData* instance_key           = dh_topic_data_create_from_json_str("{\"channel_id\":1}");
    const DHTopicData* instance_keys[1] = {instance_key};

    err = NULL;
    dh_subscriber_subscribe(data_subscriber,
                            topics,
                            topics_count,   // topic_count
                            instance_keys,  // instance_keys
                            1,              // instance_key_count
                            NULL,           // content_filter
                            false,          // get_history
                            updates,
                            &err);

    if (handle_client_error(err, "subscribe to topic")) {
        return false;
    }

    dh_topic_data_destroy(instance_key);
    dh_subscription_config_destroy(updates);

    return true;
}

// Cleanup the resources before exit
static void cleanup_resources(void) {
    if (data_subscriber) {
        dh_subscriber_destroy(data_subscriber);
        data_subscriber = NULL;
    }

    if (client) {
        DHClientError* err = NULL;
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

    const char* topics[]      = {TOPIC_NAME};
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
