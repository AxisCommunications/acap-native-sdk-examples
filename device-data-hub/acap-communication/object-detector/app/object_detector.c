// Copyright (C) 2026 Axis Communications AB, Lund, Sweden
// Licensed under the MIT License. See LICENSE file for details.

#include <datahub/client.h>
#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

// Configuration flags
#define PUBLISH_INTERVAL_SEC 1
#define TOPIC_NAME           "com.example.object_detector"
#define NUM_OBJECTS          3

/* Topic definition as a JSON string (C style) */
static const char* topic_definition =
    "{\n"
    "  \"topic_name\": \"" TOPIC_NAME
    "\",\n"
    "  \"description\": \"Detected objects are written to this topic\",\n"
    "  \"version\": \"1.0.0\",\n"
    "  \"data_schema\": {\n"
    "    \"type\": \"object\",\n"
    "    \"properties\": {\n"
    "      \"object\": {\n"
    "        \"type\": \"string\"\n"
    "      },\n"
    "      \"distance\": {\n"
    "        \"type\": \"integer\"\n"
    "      }\n"
    "    },\n"
    "    \"required\": [\"object\"]\n"
    "  }\n"
    "}";

// Global state
static volatile sig_atomic_t keep_running = true;
static atomic_bool start_producing        = ATOMIC_VAR_INIT(false);
static DHClient* client                   = NULL;
static DHWriter* data_writer              = NULL;
static DHTopic* topic                     = NULL;
static dh_production_id registered_id;

// Forward declarations
static void cleanup_resources(void);
static bool handle_client_error(DHClientError* err, const char* context);
static bool initialize_client(void);
static bool setup_topic(void);
static bool setup_writer(void);
static void publish_fake_object_detections(void);

// Consumer match update callback
static void on_consumer_match_update(dh_production_id recv_id, DHConsumerMatchStatus status) {
    syslog(LOG_INFO,
           "Consumer match update - Production ID: %llu, Status: %s\n",
           (unsigned long long)recv_id,
           status == DH_CONSUMER_MATCH ? "DH_CONSUMER_MATCH" : "DH_CONSUMER_NO_MATCH");
    if (recv_id == registered_id && status == DH_CONSUMER_MATCH) {
        atomic_store_explicit(&start_producing, true, memory_order_release);
    } else if (recv_id == registered_id && status == DH_CONSUMER_NO_MATCH) {
        atomic_store_explicit(&start_producing, false, memory_order_release);
    }
}

// error handling
static bool handle_client_error(DHClientError* err, const char* context) {
    if (err) {
        syslog(LOG_ERR, "Error in %s: %s\n", context, dh_client_error_to_string(err));
        dh_client_destroy_error(err);
        return true;
    }
    return false;
}

// Cleanup all allocated resources
static void cleanup_resources(void) {
    if (data_writer) {
        dh_writer_destroy(data_writer);
        data_writer = NULL;
    }

    if (client) {
        DHClientError* err = NULL;
        dh_client_disconnect(client, &err);
        handle_client_error(err, "client disconnect");

        if (topic) {
            dh_client_destroy_topic(topic);
            topic = NULL;
        }

        dh_client_destroy(client);
        client = NULL;
    }
}

// Initialize client and connect
static bool initialize_client(void) {
    DHClientOptions* opts = dh_client_options_create();
    dh_client_options_set_log_level(opts, DH_LOG_INFO);
    dh_client_options_set_log_target(opts, DH_LOG_TARGET_CONSOLE);

    client = dh_client_create("Client for object_detector", opts);
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

    syslog(LOG_DEBUG, "Client connected successfully");
    return true;
}

// Setup topic (get existing or create new)
static bool setup_topic(void) {
    DHClientError* err = NULL;
    topic              = dh_client_get_topic(client, TOPIC_NAME, &err);

    if (!err) {
        syslog(LOG_DEBUG, "Topic is available on the server");
        return true;
    }

    if (dh_client_error_get_code(err) == DH_ERR_INVALID_TOPIC) {
        syslog(LOG_DEBUG, "Invalid Topic detected, creating the topic");
        dh_client_destroy_error(err);
        err   = NULL;
        topic = dh_client_create_topic(client, topic_definition, &err);
        if (handle_client_error(err, "create topic")) {
            return false;
        }

        syslog(LOG_DEBUG, "Created topic successfully: %s\n", dh_topic_get_name(topic));
        return true;
    }

    handle_client_error(err, "get topic");
    return false;
}

// Setup writer and topic instance
static bool setup_writer(void) {
    // Create topic datawriter
    DHClientError* err = NULL;
    data_writer        = dh_client_create_writer(client, "producer_writer", &err);
    if (handle_client_error(err, "create topic datawriter")) {
        return false;
    }

    syslog(LOG_DEBUG, "TopicDataWriter created successfully");

    // Initialize the writer with a topic
    dh_writer_initialize(data_writer, dh_topic_get_name(topic), &err);
    if (handle_client_error(err, "initialize writer")) {
        return false;
    }

    // Set up listener for consumer match updates
    DHWriterListener* writer_listener = dh_writer_listener_create(on_consumer_match_update);
    if (!writer_listener) {
        syslog(LOG_ERR, "Failed to create writer listener");
        return false;
    }
    dh_writer_set_listener(data_writer, writer_listener, NULL);
    dh_writer_listener_destroy(writer_listener);

    // Register production
    DHTopicData* data = NULL;
    data              = dh_topic_data_create_from_json_str("{}");
    if (data == NULL) {
        syslog(LOG_ERR, "Failed to create topic data for production registration");
        return false;
    }
    dh_writer_register_production(data_writer, data, &registered_id, &err);
    // Clean up data after use
    if (data) {
        dh_topic_data_destroy(data);
    }
    if (handle_client_error(err, "register production")) {
        return false;
    }

    syslog(LOG_DEBUG, "Writer setup is successfull!");
    return true;
}

static void publish_fake_object_detections(void) {
    struct Object {
        const char* type;
        int distance;
        int speed;
    };

    static struct Object objects[NUM_OBJECTS] = {
        {.type = "human", .distance = 100, .speed = -1},
        {.type = "bird", .distance = 100, .speed = 5},
        {.type = "dog", .distance = 100, .speed = 2},
    };

    if (!atomic_load_explicit(&start_producing, memory_order_acquire)) {
        return;
    }

    for (size_t i = 0; i < NUM_OBJECTS; ++i) {
        objects[i].distance += objects[i].speed;

        if (objects[i].distance <= 50 || objects[i].distance >= 1000) {
            objects[i].speed *= -1;
        }

        char json_buffer[128];
        int result = snprintf(json_buffer,
                              sizeof(json_buffer),
                              "{\"object\":\"%s\",\"distance\":%d}",
                              objects[i].type,
                              objects[i].distance);

        if (result < 0 || (size_t)result >= sizeof(json_buffer)) {
            syslog(LOG_ERR, "Failed to format object JSON (index %zu)", i);
            continue;
        }

        DHTopicData* topic_data = dh_topic_data_create_from_json_str(json_buffer);
        if (!topic_data) {
            syslog(LOG_ERR, "Failed to create topic data from JSON");
            continue;
        }

        time_t current_time = time(NULL);
        DHClientError* err  = NULL;
        dh_writer_write_data(data_writer, topic_data, &current_time, &err);
        handle_client_error(err, "write fake object data");

        dh_topic_data_destroy(topic_data);
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

    if (!initialize_client() || !setup_topic() || !setup_writer()) {
        return EXIT_FAILURE;
    }

    while (keep_running) {
        publish_fake_object_detections();
        sleep(PUBLISH_INTERVAL_SEC);
    }

    // Disconnect does not delete the topic; call DeleteTopic to remove it.
    // Subscribers with topic updates enabled will be notified.
    DHClientError* err = NULL;
    dh_client_delete_topic(client, TOPIC_NAME, &err);
    handle_client_error(err, "Topic delete failed");
    syslog(LOG_INFO, "Application terminated");
    return EXIT_SUCCESS;
}
