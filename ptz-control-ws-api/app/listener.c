// Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.

#include "listener.h"

#include <jansson.h>
#include <string.h>
#include <syslog.h>

#include "utils.h"
#include "websocket_client.h"

static void handle_optical_zoom_data(const json_t* json_data, const char* topic_name) {
    const int channelId    = json_integer_value(json_object_get(json_data, "channelId"));
    const float zoom_mag   = json_real_value(json_object_get(json_data, "zoom_mag"));
    const float hfov_deg   = json_real_value(json_object_get(json_data, "hfov_deg"));
    const float vfov_deg   = json_real_value(json_object_get(json_data, "vfov_deg"));
    const bool zoom_moving = json_is_true(json_object_get(json_data, "zoom_moving"));

    // Replace the following syslog call with your desired handling of the optical zoom
    // data. For demonstration purposes, we're logging the received data.
    syslog(LOG_INFO,
           "Received %s topic event on channel %d, zoom mag: %f, hfov: %f, vfov: %f, "
           "zoom moving: %s",
           topic_name,
           channelId,
           zoom_mag,
           hfov_deg,
           vfov_deg,
           zoom_moving ? "true" : "false");
}

static void handle_pan_tilt_data(const json_t* json_data, const char* topic_name) {
    const int channelId          = json_integer_value(json_object_get(json_data, "channelId"));
    const float pan_deg          = json_real_value(json_object_get(json_data, "pan_deg"));
    const float tilt_deg         = json_real_value(json_object_get(json_data, "tilt_deg"));
    const bool pan_moving        = json_is_true(json_object_get(json_data, "pan_moving"));
    const bool tilt_moving       = json_is_true(json_object_get(json_data, "tilt_moving"));
    const float pan_speed_deg_s  = json_real_value(json_object_get(json_data, "pan_speed_deg_s"));
    const float tilt_speed_deg_s = json_real_value(json_object_get(json_data, "tilt_speed_deg_s"));

    // Replace the following syslog call with your desired handling of the pan/tilt data.
    // For demonstration purposes, we're logging the received data.
    syslog(LOG_INFO,
           "Received %s topic event on channel %d, pan %f, tilt %f, pan moving: %s, "
           "tilt moving: %s, pan speed: %f, tilt speed: %f",
           topic_name,
           channelId,
           pan_deg,
           tilt_deg,
           pan_moving ? "true" : "false",
           tilt_moving ? "true" : "false",
           pan_speed_deg_s,
           tilt_speed_deg_s);
}

void on_data(const DHTopicSample* sample, void* user_data) {
    // Parse the incoming JSON data.
    const DHTopicData* topic_data = dh_topic_sample_get_data(sample);
    if (topic_data == NULL) {
        panic("Failed to get topic data from sample");
    }
    json_error_t error;
    json_t* json_data = json_loads(dh_topic_data_get_json_data(topic_data), 0, &error);
    if (json_data == NULL) {
        panic("Failed to parse JSON data: %s", error.text);
    }

    const char* topic_name = dh_topic_sample_get_topic_name(sample);
    if (strcmp(topic_name, OPTICAL_ZOOM_TOPIC_NAME) == 0) {
        handle_optical_zoom_data(json_data, topic_name);
    } else if (strcmp(topic_name, PAN_TILT_TOPIC_NAME) == 0) {
        handle_pan_tilt_data(json_data, topic_name);
    } else {
        // Received data for an unknown topic, log an error. This should not happen as we haven't
        // subscribed to any other topics in main.c.
        panic("Received data for unknown topic: %s", topic_name);
    }

    // Stop the move. This is just for demonstration purposes to show how you can use the websocket
    // client to send commands in response to received data. In a real application, you would
    // replace this with your desired handling of the received data.
    websocket_client* ws_client = (websocket_client*)user_data;
    send_continuous_move_command(ws_client, 0.0f, 0.0f, 0.0f);

    json_decref(json_data);
}
