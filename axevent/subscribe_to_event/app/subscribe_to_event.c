/**
 * Copyright (C) 2021, Axis Communications AB, Lund, Sweden
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

/**
 * - subscribe_to_event.c -
 *
 * This example illustrates how to setup an subscription to
 * an ONVIF event.
 *
 * Error handling has been omitted for the sake of brevity.
 */

#include <axsdk/axevent.h>
#include <glib-object.h>
#include <glib.h>
#include <syslog.h>

/**
 * brief Callback function which is called when event subscription is completed.
 *
 * This callback will be called when the subscription has been registered
 * with the event system. The event subscription can now be used for
 * subscribing to events.
 *
 * param subscription Subscription id.
 * param event Subscribed event.
 * param token Token as user data to the callback function.
 */
static void subscription_callback(guint subscription, AXEvent* event, guint* token) {
    const AXEventKeyValueSet* key_value_set;
    gdouble value = 0;

    // The subscription id is not used in this example
    (void)subscription;

    // The token is not used in this example
    (void)token;

    // Extract the AXEventKeyValueSet from the event
    key_value_set = ax_event_get_key_value_set(event);

    // Get the Value of the Processor Usage
    ax_event_key_value_set_get_double(key_value_set, "Value", NULL, &value, NULL);

    // Print a helpful message
    syslog(LOG_INFO, "Received event with value: %lf", value);

    /*
     * Free the received event, n.b. AXEventKeyValueSet should not be freed
     * since it's owned by the event system until unsubscribing
     */
    ax_event_free(event);
}

/**
 * brief Setup a subscription for an event.
 *
 * Initialize a subscription for AXEventKeyValueSet that matches
 * ProcessorUsage, which is using the ONVIF namespace "tns1".
 *
 * Topic: tns1:Monitoring/ProcessorUsage
 *
 * param handler Event handler.
 * param token Token as user data to the callback function.
 * return subscription id as integer.
 */
static guint onviftrigger_subscription(AXEventHandler* event_handler, guint* token) {
    AXEventKeyValueSet* key_value_set = NULL;
    guint subscription                = 0;

    key_value_set = ax_event_key_value_set_new();

    // Set keys and namespaces for the event to be subscribed
    ax_event_key_value_set_add_key_value(key_value_set,
                                         "topic0",
                                         "tns1",
                                         "Monitoring",
                                         AX_VALUE_TYPE_STRING,
                                         NULL);
    ax_event_key_value_set_add_key_value(key_value_set,
                                         "topic1",
                                         "tns1",
                                         "ProcessorUsage",
                                         AX_VALUE_TYPE_STRING,
                                         NULL);

    /*
     * Time to setup the subscription. Use the "token" input argument as
     * input data to the callback function "subscription callback"
     */
    ax_event_handler_subscribe(event_handler,
                               key_value_set,
                               &subscription,
                               (AXSubscriptionCallback)subscription_callback,
                               token,
                               NULL);

    syslog(LOG_INFO, "And here's the token: %d", *token);

    // The key/value set is no longer needed
    ax_event_key_value_set_free(key_value_set);

    return subscription;
}

/**
 * brief Main function which subscribes for an event.
 */
int main(void) {
    GMainLoop* main_loop          = NULL;
    AXEventHandler* event_handler = NULL;
    guint subscription            = 0;
    guint token                   = 1234;

    // Set up the user logging to syslog
    openlog(NULL, LOG_PID, LOG_USER);
    syslog(LOG_INFO, "Started logging from subscribe event application");

    // Event handler
    event_handler = ax_event_handler_new();
    subscription  = onviftrigger_subscription(event_handler, &token);

    // Main loop
    main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);

    // Cleanup event handler
    ax_event_handler_unsubscribe(event_handler, subscription, NULL);
    ax_event_handler_free(event_handler);

    // Free g_main_loop
    g_main_loop_unref(main_loop);

    // Cleanup syslog
    closelog();

    return 0;
}
