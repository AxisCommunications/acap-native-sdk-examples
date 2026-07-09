// Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.

#include "subscriber.h"

#include <datahub/client.h>
#include <datahub/subscriber.h>
#include <stdlib.h>
#include <syslog.h>

#include "listener.h"
#include "utils.h"

struct subscriber {
    DHClient* dh_client;
    DHSubscriber* topic_data_subscriber;
};

subscriber* subscriber_create(websocket_client* ws_client) {
    subscriber* subscriber_instance = (subscriber*)malloc(sizeof(subscriber));

    DHError* error                 = NULL;
    subscriber_instance->dh_client = dh_client_create("PTZ Example ACAP application", NULL);

    if (subscriber_instance->dh_client == NULL) {
        panic("Failed to create Device Data Hub client");
    }

    subscriber_instance->topic_data_subscriber =
        dh_client_create_subscriber(subscriber_instance->dh_client,
                                    "PTZ Example ACAP application",
                                    &error);

    if (subscriber_instance->topic_data_subscriber == NULL) {
        if (error != NULL) {
            panic("Failed to create Device Data Hub subscriber: %s", dh_error_to_string(error));
        } else {
            panic("Failed to create Device Data Hub subscriber: unknown error");
        }
    }

    // Connect the DDH client to the server. This is required before any operations can be
    // performed with the client.
    dh_client_connect(subscriber_instance->dh_client, &error);
    if (error != NULL) {
        panic("Failed to connect Device Data Hub client: %s", dh_error_to_string(error));
    }

    dh_subscriber_set_data_callback(subscriber_instance->topic_data_subscriber,
                                    on_data,
                                    ws_client,
                                    &error);
    if (error != NULL) {
        panic("Failed to set data callback: %s", dh_error_to_string(error));
    }

    return subscriber_instance;
}

void subscriber_destroy(subscriber* subscriber_instance) {
    if (subscriber_instance != NULL) {
        dh_subscriber_destroy(subscriber_instance->topic_data_subscriber);
        dh_client_destroy(subscriber_instance->dh_client);
        free(subscriber_instance);
    }
}

bool subscribe_to_topic(subscriber* subscriber_instance, const char* topic_name) {
    // Create a subscription options object, which is an object that holds which events should be
    // subscribed to. In this case, we want to subscribe to data updates for the specified topic, so
    // we create a filter for that and add it to the subscription options, then enable data updates
    // on the options.
    DHSubscribeOptions* options = dh_subscribe_options_create();
    if (options == NULL) {
        panic("Failed to create subscription options");
    }

    // Build subscription filter
    DHFilter* filter = dh_filter_create();
    if (filter == NULL) {
        panic("Failed to create filter");
    }

    // Add the topic name to the filter to specify that we want to subscribe to this topic.
    DHError* error = NULL;
    dh_filter_add_topic_name(filter, topic_name, &error);
    if (error != NULL) {
        panic("Failed to add topic name to filter: %s", dh_error_to_string(error));
    }

    dh_subscribe_options_add_filter(options, filter, &error);
    dh_filter_destroy(filter);
    if (error != NULL) {
        dh_subscribe_options_destroy(options);
        panic("Failed to add filter to subscription options: %s", dh_error_to_string(error));
    }

    // Enable data updates for the subscription.
    dh_subscribe_options_set_enable_data_updates(options, true);

    // Subscribe to the topic with the specified options. This will cause the data callback to be
    // invoked whenever a new data sample is received for this topic.
    dh_subscriber_subscribe(subscriber_instance->topic_data_subscriber, options, &error);
    dh_subscribe_options_destroy(options);
    if (error != NULL) {
        panic("Failed to subscribe to topic %s: %s", topic_name, dh_error_to_string(error));
    }

    syslog(LOG_INFO, "Subscribed to topic: %s", topic_name);
    return true;
}
