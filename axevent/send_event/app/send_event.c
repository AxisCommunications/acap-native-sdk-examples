/**
 * Copyright (C) 2020, Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * - send_event.c -
 *
 * This example illustrates how to send a stateful ONVIF event, which is
 * changing the value every 10th second.
 *
 * Error handling has been omitted for the sake of brevity.
 */
#include <glib.h>
#include <glib-object.h>
#include <axsdk/axevent.h>
#include <string.h>
#include <syslog.h>

typedef struct
{
  AXEventHandler *event_handler;
  guint event_id;
  guint timer;
  gdouble value;
} AppData;

static AppData *app_data = NULL;

/**
 * brief Send event.
 *
 * Send the previously declared event.
 *
 * param app_data Application data containing e.g. the event declaration id.
 * return TRUE
 */
static gboolean
send_event(AppData *app_data)
{
  AXEventKeyValueSet *key_value_set = NULL;
  AXEvent *event = NULL;

  key_value_set = ax_event_key_value_set_new();

  // Add the variable elements of the event to the set
  syslog(LOG_INFO, "Add value: %lf", app_data->value);
  ax_event_key_value_set_add_key_value(key_value_set, "Value", NULL, &app_data->value,
      AX_VALUE_TYPE_DOUBLE, NULL);

  // Create the event
  // Use ax_event_new2 since ax_event_new is deprecated from 3.2
  event = ax_event_new2(key_value_set, NULL);

  // The key/value set is no longer needed
  ax_event_key_value_set_free(key_value_set);

  // Send the event
  ax_event_handler_send_event(app_data->event_handler, app_data->event_id, event, NULL);

  syslog(LOG_INFO, "Send stateful event with value: %lf", app_data->value);

  ax_event_free(event);

  // Toggle value
  app_data->value = app_data->value >= 100 ? 0 : app_data->value + 10;

  // Returning TRUE keeps the timer going
  return TRUE;
}


/**
 * brief Callback function which is called when event declaration is completed.
 *
 * This callback will be called when the declaration has been registered
 * with the event system. The event declaration can now be used to send
 * events.
 *
 * param declaration Event declaration id.
 * param value Start value of the event.
 */
static void
declaration_complete(guint declaration, gdouble *value)
{
  syslog(LOG_INFO, "Declaration complete for: %d", declaration);

  app_data->value = *value;

  // Set up a timer to be called every 10th second
  app_data->timer = g_timeout_add_seconds(10, (GSourceFunc)send_event, app_data);
}

/**
 * brief Setup a declaration of an event.
 *
 * Declare a stateful ONVIF event that looks like this,
 * which is using ONVIF namespace "tns1".
 *
 * Topic: tns1:Monitoring/ProcessorUsage
 * <tt:MessageDescription IsProperty="true">
 *  <tt:Source>
 *   <tt:SimpleItemDescription Name=”Token” Type=”tt:ReferenceToken”/>
 *  </tt:Source>
 *  <tt:Data>
 *   <tt:SimpleItemDescription Name="Value" Type="xs:float"/>
 *  </tt:Data>
 * </tt:MessageDescription>
 *
 * Value = 0 <-- The initial value will be set to 0.0
 *
 * param event_handler Event handler.
 * return declaration id as integer.
 */
static guint
setup_declaration(AXEventHandler *event_handler)
{
  AXEventKeyValueSet *key_value_set = NULL;
  guint declaration = 0;
  guint token = 0;
  gdouble start_value = 0;
  GError *error = NULL;

  // Create keys, namespaces and nice names for the event
  key_value_set = ax_event_key_value_set_new();
  ax_event_key_value_set_add_key_value(key_value_set, "topic0", "tns1",
      "Monitoring", AX_VALUE_TYPE_STRING, NULL);
  ax_event_key_value_set_add_key_value(key_value_set, "topic1", "tns1",
      "ProcessorUsage", AX_VALUE_TYPE_STRING, NULL);
  ax_event_key_value_set_add_key_value(key_value_set, "Token", NULL,
      &token, AX_VALUE_TYPE_INT, NULL);
  ax_event_key_value_set_add_key_value(key_value_set, "Value", NULL,
      &start_value, AX_VALUE_TYPE_DOUBLE, NULL);
  ax_event_key_value_set_mark_as_source(key_value_set, "Token", NULL, NULL);
  ax_event_key_value_set_mark_as_user_defined(key_value_set, "Token", NULL,
      "wstype:tt:ReferenceToken", NULL);
  ax_event_key_value_set_mark_as_data(key_value_set, "Value", NULL, NULL);
  ax_event_key_value_set_mark_as_user_defined(key_value_set, "Value", NULL,
      "wstype:xs:float", NULL);

  // Declare event
  if (!ax_event_handler_declare(event_handler, key_value_set,
      FALSE, // Indicate a property state event
      &declaration,
      (AXDeclarationCompleteCallback)declaration_complete,
      &start_value,
      &error)) {
    syslog(LOG_WARNING, "Could not declare: %s", error->message);
    g_error_free(error);
  }

  // The key/value set is no longer needed
  ax_event_key_value_set_free(key_value_set);
  return declaration;
}

/**
 * brief Main function which sends an event.
 */
gint main(void)
{
  GMainLoop *main_loop = NULL;

  // Set up the user logging to syslog
  openlog(NULL, LOG_PID, LOG_USER);
  syslog(LOG_INFO, "Started logging from send event application");

  // Event handler
  app_data = calloc(1, sizeof(AppData));
  app_data->event_handler = ax_event_handler_new();
  app_data->event_id = setup_declaration(app_data->event_handler);

  // Main loop
  main_loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(main_loop);

  // Cleanup event handler
  ax_event_handler_undeclare(app_data->event_handler, app_data->event_id, NULL);
  ax_event_handler_free(app_data->event_handler);
  free(app_data);

  // Free g_main_loop
  g_main_loop_unref(main_loop);

  closelog();

  return 0;
}
