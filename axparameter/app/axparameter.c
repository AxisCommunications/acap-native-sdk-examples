/**
 * Copyright (C) 2024, Axis Communications AB, Lund, Sweden
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
 * - axparameter -
 *
 * This example shows how to handle system-wide and application-defined parameters using the
 * AXParameter library. Emphasis has been put on the use of callback functions and some of the
 * limitations they impose.
 */
#include <axsdk/axparameter.h>
#include <stdbool.h>
#include <syslog.h>

#define APP_NAME "axparameter"

// Structure used for passing data to the monitor_parameters() callback.
struct message {
    AXParameter* handle;
    char* name;
    char* value;
};

static GMainLoop* loop = NULL;  // Must be global, so quit_main_loop() can use it

static void quit_main_loop(__attribute__((unused)) int signal_num) {
    g_main_loop_quit(loop);
}

static void set_sigterm_and_sigint_handler(void (*handler)(int)) {
    struct sigaction sa = {0};

    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handler;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
}

// Print an error to syslog and exit the application if a fatal error occurs.
__attribute__((noreturn)) __attribute__((format(printf, 1, 2))) static void
panic(const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    vsyslog(LOG_ERR, format, arg);
    va_end(arg);
    exit(1);
}

// Iterate over all parameters in search for a specific one.
// An alternative would be to call ax_parameter_get() test if it succeeds or fails.
static bool has_parameter(AXParameter* handle, const char* needle) {
    GError* error = NULL;
    GList* list   = ax_parameter_list(handle, &error);
    if (!list)
        panic("%s", error->message);

    bool needle_found = false;
    for (GList* x = list; x != NULL; x = g_list_next(x)) {
        syslog(LOG_INFO, "App has a parameter named %s", (gchar*)x->data);
        if (strcmp(x->data, needle) == 0)
            needle_found = true;
        g_free(x->data);
    }
    g_list_free(list);
    syslog(LOG_INFO, "Parameter %s %s found", needle, needle_found ? "was" : "was not");
    return needle_found;
}

// A parameter of type "bool:no,yes" is guaranteed to contain one of those strings,
// but user code is still needed to interpret it as a Boolean type.
static bool is_parameter_yes(AXParameter* handle, const char* name) {
    GError* error = NULL;
    gchar* value;

    if (!ax_parameter_get(handle, name, &value, &error))
        panic("%s", error->message);

    bool result = strcmp(value, "yes") == 0;

    g_free(value);
    return result;
}

// Instead of specifying parameters in manifest.json, they can be added at runtime.
static void restore_custom_value_from_backup(AXParameter* handle) {
    GError* error = NULL;
    gchar* value;

    if (!ax_parameter_get(handle, "BackupValue", &value, &error) ||
        !ax_parameter_add(handle, "CustomValue", value, NULL, &error))
        panic("%s", error->message);

    syslog(LOG_INFO,
           "The parameter CustomValue was added, "
           "but won't be visible in the Settings page until the Apps page is reloaded.");

    g_free(value);
}

// Parameters can also be removed at runtime.
static void back_up_and_remove_custom_value(AXParameter* handle) {
    GError* error = NULL;
    gchar* value;

    if (!ax_parameter_get(handle, "CustomValue", &value, &error) ||
        !ax_parameter_set(handle, "BackupValue", value, TRUE, &error) ||
        !ax_parameter_remove(handle, "CustomValue", &error))
        panic("%s", error->message);

    syslog(LOG_INFO,
           "The parameter CustomValue was removed, "
           "but will be visible in the Settings page until the Apps page is reloaded.");

    g_free(value);
}

// This function is registered as a callback from g_timeout_add_seconds(),
// which means it can call ax_parameter_* functions without causing a deadlock.
static gboolean monitor_parameters(void* msg_void_ptr) {
    GError* error       = NULL;
    struct message* msg = msg_void_ptr;
    AXParameter* handle = msg->handle;

    syslog(LOG_INFO, "%s was changed to '%s' one second ago", msg->name, msg->value);

    bool has_custom_value_param = has_parameter(handle, "CustomValue");

    if (is_parameter_yes(handle, "IsCustomized")) {
        if (!has_custom_value_param)
            restore_custom_value_from_backup(handle);

        gchar* custom_value;
        if (!ax_parameter_get(handle, "CustomValue", &custom_value, &error))
            panic("%s", error->message);
        syslog(LOG_INFO, "Custom value: '%s'", custom_value);
        g_free(custom_value);
    } else {
        if (has_custom_value_param)
            back_up_and_remove_custom_value(handle);

        syslog(LOG_INFO, "Not customized");
    }

    // Free all memory of the message struct and tell GLib not to make this call again.
    free(msg->name);
    free(msg->value);
    free(msg);
    return FALSE;
}

// This function is registered as a callback using ax_parameter_register_callback().
// It must not call any ax_parameter_* functions, since that would cause a deadlock.
static void parameter_changed(const gchar* name, const gchar* value, gpointer handle_void_ptr) {
    const char* name_without_qualifiers = &name[strlen("root." APP_NAME ".")];
    syslog(LOG_INFO, "%s was changed to '%s' just now", name_without_qualifiers, value);

    // Schedule a call in one second to a function that is allowed to use ax_parameter_* functions.
    // The strings must be copied, since they are owned by the AXParameter library.
    // The message struct must be dynamically allocated, since there may be more AXParameter
    // callback calls before this message reaches its destination.

    struct message* msg = malloc(sizeof(struct message));

    msg->handle = handle_void_ptr;
    msg->name   = strdup(name_without_qualifiers);
    msg->value  = strdup(value);

    g_timeout_add_seconds(1, monitor_parameters, msg);
}

int main(void) {
    GError* error = NULL;

    openlog(APP_NAME, LOG_PID, LOG_USER);

    // Passing in APP_NAME gives access to this application's parameters without qualifiers and
    // makes it possible to add or remove parameters.
    AXParameter* handle = ax_parameter_new(APP_NAME, &error);
    if (handle == NULL)
        panic("%s", error->message);

    // Parameters outside the application's group requires qualification.
    gchar* serial_number;
    if (!ax_parameter_get(handle, "Properties.System.SerialNumber", &serial_number, &error))
        panic("%s", error->message);
    syslog(LOG_INFO, "SerialNumber: '%s'", serial_number);
    g_free(serial_number);

    // Act on changes to IsCustomized as soon as they happen.
    if (!ax_parameter_register_callback(handle, "IsCustomized", parameter_changed, handle, &error))
        panic("%s", error->message);

    // Register the same callback for CustomValue, even though that parameter does not exist yet!
    if (!ax_parameter_register_callback(handle, "CustomValue", parameter_changed, handle, &error))
        panic("%s", error->message);

    // Start listening to callbacks by launching a GLib main loop.
    loop = g_main_loop_new(NULL, FALSE);
    set_sigterm_and_sigint_handler(quit_main_loop);
    g_main_loop_run(loop);
    syslog(LOG_INFO, "Application was stopped by SIGTERM or SIGINT.");

    g_main_loop_unref(loop);
    ax_parameter_free(handle);
    return 0;
}
