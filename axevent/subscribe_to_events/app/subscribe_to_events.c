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
 * brief This example illustrates how to setup subscribe to predefined events
 * in Axis products.
 *
 * All events in this example may not be available in your product of
 * development, but subscribing to events that will never occur is perfectly
 * fine and will not give an error.
 *
 * Error handling has been omitted for the sake of brevity.
 */

#include <axsdk/axevent.h>
#include <glib-object.h>
#include <glib.h>
#include <syslog.h>

#define AUDIOTRIGGER_TOKEN  1001
#define DAYNIGHT_TOKEN      2002
#define MANUALTRIGGER_TOKEN 3003
#define PTZMOVE_TOKEN       4004
#define TAMPERING_TOKEN     5005

/***** Struct declarations ****************************************************/

typedef struct {
    guint id;
    struct {
        gint num_moves;
    } ptzchannel[8];
} ptzmove;

/***** Callback functions *****************************************************/

/**
 * brief Callback function which is called when event subscription is completed.
 *
 * This callback will be called when the subscription has been registered
 * with the event system. The event subscription can now be used for
 * subscribing to events. The callback function is shared between different
 * events and separated with token.
 *
 * param subscription Subscription id.
 * param event Subscribed event.
 * param token Token as user data to the callback function.
 */
static void common_callback(guint subscription, AXEvent* event, guint* token) {
    const AXEventKeyValueSet* key_value_set;
    gboolean day;
    gboolean state;
    gboolean triggered;
    gint channel;
    gint port;

    // The subscription id is not used in this example
    (void)subscription;

    // Extract the AXEventKeyValueSet from the event
    key_value_set = ax_event_get_key_value_set(event);

    // Handle the different events
    if (*token == AUDIOTRIGGER_TOKEN) {
        // Get the audio trigger variables of interest
        ax_event_key_value_set_get_integer(key_value_set, "channel", NULL, &channel, NULL);
        ax_event_key_value_set_get_boolean(key_value_set, "triggered", NULL, &triggered, NULL);

        // Print state of audio trigger level
        if (triggered) {
            syslog(LOG_INFO,
                   "%d:audiotrigger-event: Audio channel %d above "
                   "trigger level",
                   *token,
                   channel);
        } else {
            syslog(LOG_INFO,
                   "%d:audiotrigger-event: Audio channel %d below "
                   "trigger level",
                   *token,
                   channel);
        }

    } else if (*token == DAYNIGHT_TOKEN) {
        // Get the state of the day key
        ax_event_key_value_set_get_boolean(key_value_set, "day", NULL, &day, NULL);

        // Print if day or night
        if (day) {
            syslog(LOG_INFO, "%d:daynight-event: Day detected", *token);
        } else {
            syslog(LOG_INFO, "%d:daynight-event: Night detected", *token);
        }

    } else if (*token == MANUALTRIGGER_TOKEN) {
        // Get the manual trigger variables of interest
        ax_event_key_value_set_get_integer(key_value_set, "port", NULL, &port, NULL);
        ax_event_key_value_set_get_boolean(key_value_set, "state", NULL, &state, NULL);

        // Print state of manual trigger
        if (state) {
            syslog(LOG_INFO, "%d:manualtrigger-event: Trigger on port %d is active", *token, port);
        } else {
            syslog(LOG_INFO,
                   "%d:manualtrigger-event: Trigger on port %d is inactive",
                   *token,
                   port);
        }

    } else if (*token == TAMPERING_TOKEN) {
        // Get the tampering channel variable
        ax_event_key_value_set_get_integer(key_value_set, "channel", NULL, &channel, NULL);

        // Tampering is stateless, inform that event took place
        syslog(LOG_INFO, "%d:tampering-event: Tampering detected on channel %d", *token, channel);
    }

    /*
     * Free the received event, n.b. AXEventKeyValueSet should not be freed
     * since it's owned by the event system until unsubscribing
     */
    ax_event_free(event);
}

/**
 * brief Callback function which is called when event subscription is completed.
 *
 * This callback will be called when the subscription has been registered
 * with the event system. The event subscription can now be used for
 * subscribing to events. This callback function is only for PTZ move events.
 *
 * param subscription Subscription id.
 * param event Subscribed event.
 * param data User data of type ptzmove.
 */
static void ptzmove_callback(guint subscription, AXEvent* event, ptzmove* data) {
    const AXEventKeyValueSet* key_value_set;
    gboolean is_moving;
    gboolean val = FALSE;
    gint channel;

    // The subscription id is not used in this example
    (void)subscription;

    // Extract the AXEventKeyValueSet from the event
    key_value_set = ax_event_get_key_value_set(event);

    // Get the state of the manual trigger port
    ax_event_key_value_set_get_integer(key_value_set,
                                       "PTZConfigurationToken",
                                       NULL,
                                       &channel,
                                       NULL);
    ax_event_key_value_set_get_boolean(key_value_set, "is_moving", NULL, &is_moving, NULL);

    // Print channel and if moving or stopped
    if (is_moving) {
        data->ptzchannel[channel].num_moves += 1;
        val = (data->ptzchannel[channel].num_moves == 1);
        syslog(LOG_INFO,
               "%d:ptzmove-event: PTZ channel %d started moving (%d %s)",
               data->id,
               channel,
               data->ptzchannel[channel].num_moves,
               val ? "time" : "times");
    } else {
        if (data->ptzchannel[channel].num_moves > 0) {
            syslog(LOG_INFO, "%d:ptzmove-event: PTZ channel %d stopped moving", data->id, channel);
        }
    }

    /*
     * Free the received event, n.b. AXEventKeyValueSet should not be freed
     * since it's owned by the event system until unsubscribing
     */
    ax_event_free(event);
}

/***** Subscription functions *************************************************/

/**
 * brief Setup a subscription to an audio event.
 *
 * Initialize a subscription that matches AudioSource/TriggerLevel.
 *
 * param event_handler Event handler.
 * param token User data to callback function.
 * return Subscription id as integer.
 */
static guint audiotrigger_subscription(AXEventHandler* event_handler, guint token) {
    AXEventKeyValueSet* key_value_set;
    guint subscription;

    key_value_set = ax_event_key_value_set_new();

    /* Initialize an AXEventKeyValueSet that matches the tampering event.
     *
     *    tns1:topic0=AudioSource
     * tnsaxis:topic1=TriggerLevel
     *        channel=NULL    <-- Subscribe to all channels
     *      triggered=NULL    <-- Subscribe to all states
     */
    ax_event_key_value_set_add_key_values(key_value_set,
                                          NULL,
                                          "topic0",
                                          "tns1",
                                          "AudioSource",
                                          AX_VALUE_TYPE_STRING,
                                          "topic1",
                                          "tnsaxis",
                                          "TriggerLevel",
                                          AX_VALUE_TYPE_STRING,
                                          "channel",
                                          NULL,
                                          NULL,
                                          AX_VALUE_TYPE_INT,
                                          "triggered",
                                          NULL,
                                          NULL,
                                          AX_VALUE_TYPE_BOOL,
                                          NULL);

    // Setup subscription and connect to callback function
    ax_event_handler_subscribe(event_handler,                            // event handler
                               key_value_set,                            // key value set
                               &subscription,                            // subscription id
                               (AXSubscriptionCallback)common_callback,  // callback function
                               &token,                                   // user data
                               NULL);                                    // GError

    // Free key value set
    ax_event_key_value_set_free(key_value_set);

    // Return subscription id
    syslog(LOG_INFO, "Audio trigger subscription id: %d", subscription);
    return subscription;
}

/**
 * brief Setup a subscription to a day/night event.
 *
 * Initialize a subscription that matches VideoSource/DayNightVision.
 *
 * param event_handler Event handler.
 * param token User data to callback function.
 * return Subscription id as integer.
 */
static guint daynight_subscription(AXEventHandler* event_handler, guint token) {
    AXEventKeyValueSet* key_value_set;
    guint subscription;

    key_value_set = ax_event_key_value_set_new();

    /* Initialize an AXEventKeyValueSet that matches the day/night event.
     *
     *    tns1:topic0=VideoSource
     * tnsaxis:topic1=DayNightVision
     * VideoSource...=NULL    <-- Subscribe to all values
     *            day=NULL    <-- Subscribe to all states
     */
    ax_event_key_value_set_add_key_values(key_value_set,
                                          NULL,
                                          "topic0",
                                          "tns1",
                                          "VideoSource",
                                          AX_VALUE_TYPE_STRING,
                                          "topic1",
                                          "tnsaxis",
                                          "DayNightVision",
                                          AX_VALUE_TYPE_STRING,
                                          "VideoSourceConfigurationToken",
                                          NULL,
                                          NULL,
                                          AX_VALUE_TYPE_INT,
                                          "day",
                                          NULL,
                                          NULL,
                                          AX_VALUE_TYPE_BOOL,
                                          NULL);

    // Setup subscription and connect to callback function
    ax_event_handler_subscribe(event_handler,                            // event handler
                               key_value_set,                            // key value set
                               &subscription,                            // subscription id
                               (AXSubscriptionCallback)common_callback,  // callback function
                               &token,                                   // user data
                               NULL);                                    // GError

    // Free key value set
    ax_event_key_value_set_free(key_value_set);

    // Return subscription id
    syslog(LOG_INFO, "Day/Night subscription id: %d", subscription);
    return subscription;
}

/**
 * brief Setup a subscription to a manually triggered event.
 *
 * Initialize a subscription that matches Device/IO/VirtualPort.
 *
 * param event_handler Event handler.
 * param token User data to callback function.
 * return Subscription id as integer.
 */
static guint manualtrigger_subscription(AXEventHandler* event_handler, guint token) {
    AXEventKeyValueSet* key_value_set;
    gint port = 1;
    guint subscription;

    key_value_set = ax_event_key_value_set_new();

    /* Initialize an AXEventKeyValueSet that matches the manual trigger event.
     *
     *    tns1:topic0=Device
     * tnsaxis:topic1=IO
     * tnsaxis:topic2=VirtualPort
     *           port=&port    <-- Subscribe to port number 1
     *          state=NULL     <-- Subscribe to all states
     */
    ax_event_key_value_set_add_key_values(key_value_set,
                                          NULL,
                                          "topic0",
                                          "tns1",
                                          "Device",
                                          AX_VALUE_TYPE_STRING,
                                          "topic1",
                                          "tnsaxis",
                                          "IO",
                                          AX_VALUE_TYPE_STRING,
                                          "topic2",
                                          "tnsaxis",
                                          "VirtualPort",
                                          AX_VALUE_TYPE_STRING,
                                          "port",
                                          NULL,
                                          &port,
                                          AX_VALUE_TYPE_INT,
                                          "state",
                                          NULL,
                                          NULL,
                                          AX_VALUE_TYPE_BOOL,
                                          NULL);

    // Setup subscription and connect to callback function
    ax_event_handler_subscribe(event_handler,                            // event handler
                               key_value_set,                            // key value set
                               &subscription,                            // subscription id
                               (AXSubscriptionCallback)common_callback,  // callback function
                               &token,                                   // user data
                               NULL);                                    // GError

    // Free key value set
    ax_event_key_value_set_free(key_value_set);

    // Return subscription id
    syslog(LOG_INFO, "Manual trigger subscription id: %d", subscription);
    return subscription;
}

/**
 * brief Setup a subscription to a PTZ move event.
 *
 * Initialize a subscription that matches PTZController/Move.
 *
 * param event_handler Event handler.
 * param data User data to callback function.
 * return Subscription id as integer.
 */
static guint ptzmove_subscription(AXEventHandler* event_handler, ptzmove* data) {
    AXEventKeyValueSet* key_value_set;
    guint subscription;

    key_value_set = ax_event_key_value_set_new();

    /* Initialize an AXEventKeyValueSet that matches the tampering event.
     *
     *    tns1:topic0=PTZController
     * tnsaxis:topic1=Move
     *        channel=NULL    <-- Subscribe to all PTZ channels
     *      is_moving=NULL    <-- Subscribe to all values
     */
    ax_event_key_value_set_add_key_values(key_value_set,
                                          NULL,
                                          "topic0",
                                          "tns1",
                                          "PTZController",
                                          AX_VALUE_TYPE_STRING,
                                          "topic1",
                                          "tnsaxis",
                                          "Move",
                                          AX_VALUE_TYPE_STRING,
                                          "PTZConfigurationToken",
                                          NULL,
                                          NULL,
                                          AX_VALUE_TYPE_INT,
                                          "is_moving",
                                          NULL,
                                          NULL,
                                          AX_VALUE_TYPE_BOOL,
                                          NULL);

    // Setup subscription and connect to callback function
    ax_event_handler_subscribe(event_handler,                             // event handler
                               key_value_set,                             // key value set
                               &subscription,                             // subscription id
                               (AXSubscriptionCallback)ptzmove_callback,  // callback function
                               data,                                      // user data
                               NULL);                                     // GError

    // Free key value set
    ax_event_key_value_set_free(key_value_set);

    // Return subscription id
    syslog(LOG_INFO, "PTZ move subscription id: %d", subscription);
    return subscription;
}

/**
 * brief Setup a subscription to a tampering event.
 *
 * Initialize a subscription that matches VideoSource/Tampering.
 *
 * param event_handler Event handler.
 * param token User data to callback function.
 * return Subscription id as integer.
 */
static guint tampering_subscription(AXEventHandler* event_handler, guint token) {
    AXEventKeyValueSet* key_value_set;
    gint channel = 1;
    guint subscription;

    key_value_set = ax_event_key_value_set_new();

    /* Initialize an AXEventKeyValueSet that matches the tampering event.
     *
     *    tns1:topic0=VideoSource
     * tnsaxis:topic1=Tampering
     *        channel=&channel    <-- Subscribe to channel number 1
     *      tampering=NULL        <-- Subscribe to all values
     */
    ax_event_key_value_set_add_key_values(key_value_set,
                                          NULL,
                                          "topic0",
                                          "tns1",
                                          "VideoSource",
                                          AX_VALUE_TYPE_STRING,
                                          "topic1",
                                          "tnsaxis",
                                          "Tampering",
                                          AX_VALUE_TYPE_STRING,
                                          "channel",
                                          NULL,
                                          &channel,
                                          AX_VALUE_TYPE_INT,
                                          "tampering",
                                          NULL,
                                          NULL,
                                          AX_VALUE_TYPE_INT,
                                          NULL);

    // Setup subscription and connect to callback function
    ax_event_handler_subscribe(event_handler,                            // event handler
                               key_value_set,                            // key value set
                               &subscription,                            // subscription id
                               (AXSubscriptionCallback)common_callback,  // callback function
                               &token,                                   // user data
                               NULL);                                    // GError

    // Free key value set
    ax_event_key_value_set_free(key_value_set);

    // Return subscription id
    syslog(LOG_INFO, "Tampering subscription id: %d", subscription);
    return subscription;
}

/***** Main *******************************************************************/

/**
 * brief Main function which subscribes to multiple events.
 */
int main(void) {
    GMainLoop* main_loop;
    AXEventHandler* event_handler;
    guint audiotrigger_handle;
    guint daynight_handle;
    guint manualtrigger_handle;
    guint ptzmove_handle;
    guint tampering_handle;

    // Setup of user data
    guint audiotrigger_token  = AUDIOTRIGGER_TOKEN;
    guint daynight_token      = DAYNIGHT_TOKEN;
    guint manualtrigger_token = MANUALTRIGGER_TOKEN;
    guint tampering_token     = TAMPERING_TOKEN;
    ptzmove* ptzmove_data     = g_slice_new0(ptzmove);
    ptzmove_data->id          = PTZMOVE_TOKEN;

    // Initialize main loop
    main_loop = g_main_loop_new(NULL, FALSE);

    // Set up the user logging to syslog
    openlog(NULL, LOG_PID, LOG_USER);
    syslog(LOG_INFO, "Started logging from subscribe event application");

    // Create an event handler
    event_handler = ax_event_handler_new();

    // Subscribe to different events and get subscription id
    audiotrigger_handle  = audiotrigger_subscription(event_handler, audiotrigger_token);
    daynight_handle      = daynight_subscription(event_handler, daynight_token);
    manualtrigger_handle = manualtrigger_subscription(event_handler, manualtrigger_token);
    ptzmove_handle       = ptzmove_subscription(event_handler, ptzmove_data);
    tampering_handle     = tampering_subscription(event_handler, tampering_token);

    // Start main loop
    g_main_loop_run(main_loop);

    // Unsubscribe each subscription created
    ax_event_handler_unsubscribe(event_handler, audiotrigger_handle, NULL);
    ax_event_handler_unsubscribe(event_handler, daynight_handle, NULL);
    ax_event_handler_unsubscribe(event_handler, manualtrigger_handle, NULL);
    ax_event_handler_unsubscribe(event_handler, ptzmove_handle, NULL);
    ax_event_handler_unsubscribe(event_handler, tampering_handle, NULL);

    // Free event handler
    ax_event_handler_free(event_handler);

    // Free g_main_loop
    g_main_loop_unref(main_loop);

    // Free struct data
    g_slice_free(ptzmove, ptzmove_data);

    return 0;
}
