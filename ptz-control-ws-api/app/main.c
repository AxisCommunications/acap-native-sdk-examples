// Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.

#include <curl/curl.h>
#include <glib-unix.h>
#include <glib.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>

#include "listener.h"
#include "subscriber.h"
#include "utils.h"
#include "websocket_client.h"

static gboolean signal_handler(gpointer user_data) {
    GMainLoop* loop = (GMainLoop*)user_data;
    g_main_loop_quit(loop);
    return FALSE;
}

int main(void) {
    // CURL must be initialized to use the websocket client. This is done here in main() since it's
    // a global initialization, and the client may be used in multiple places.
    CURLcode result = curl_global_init(CURL_GLOBAL_ALL);
    if (result != CURLE_OK) {
        panic("Failed to initialize CURL: %s", curl_easy_strerror(result));
    }

    websocket_client* ws_client = websocket_client_create();
    if (ws_client == NULL) {
        panic("Failed to create websocket client");
    }
    subscriber* subscriber_instance = subscriber_create(ws_client);
    if (subscriber_instance == NULL) {
        panic("Failed to create subscriber");
    }

    // Subscribe to the topics we want to receive messages from.
    subscribe_to_topic(subscriber_instance, PAN_TILT_TOPIC_NAME);
    subscribe_to_topic(subscriber_instance, OPTICAL_ZOOM_TOPIC_NAME);

    // Send an initial move command to trigger an event from DDH.
    send_continuous_move_command(ws_client, 100.0f, 10.0f, 0.0f);

    // Keep the program running until we receive SIGINT or SIGTERM. Here we use a GLib main loop for
    // this, but DDH doesn't require that, so you can use any method you like to keep the program
    // running and handle shutdown.
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    g_unix_signal_add(SIGTERM, signal_handler, loop);
    g_unix_signal_add(SIGINT, signal_handler, loop);
    g_main_loop_run(loop);

    // Clean up CURL global state before exiting.
    g_main_loop_unref(loop);
    websocket_client_destroy(ws_client);
    subscriber_destroy(subscriber_instance);
    curl_global_cleanup();

    return 0;
}
