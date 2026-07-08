// Copyright (C) 2026 Axis Communications AB, Lund, Sweden
// Licensed under the MIT License. See LICENSE file for details.

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#define _cleanup_(func) __attribute__((cleanup(func)))

#define RECORDING_NOTIFY_BUS_NAME    "com.axis.RecordingNotify1"
#define RECORDING_NOTIFY_OBJECT_PATH "/com/axis/RecordingNotify1"
#define RECORDING_NOTIFY_INTERFACE   "com.axis.RecordingNotify1.Notify1"

#define RECORDING_SEARCH_BUS_NAME    "com.axis.RecordingSearch1"
#define RECORDING_SEARCH_OBJECT_PATH "/com/axis/RecordingSearch1"
#define RECORDING_SEARCH_INTERFACE   "com.axis.RecordingSearch1.Search1"

#define RECORDING_PLAYBACK_BUS_NAME    "com.axis.RecordingPlayback1"
#define RECORDING_PLAYBACK_OBJECT_PATH "/com/axis/RecordingPlayback1"
#define RECORDING_PLAYBACK_INTERFACE   "com.axis.RecordingPlayback1.Playback1"

struct string_list {
    char** items;
    size_t len;
    size_t cap;
};

__attribute__((noreturn)) __attribute__((format(printf, 1, 2))) static void
panic(const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    vsyslog(LOG_ERR, format, arg);
    va_end(arg);
    exit(1);
}

static char* strdupp(const char* s) {
    char* dup = strdup(s);
    if (!dup) {
        panic("strdup failed");
    }

    return dup;
}

static void freep(void* p) {
    free(*(void**)p);
}

static void string_list_append(struct string_list* list, const char* s) {
    if (list->len == list->cap) {
        size_t new_cap   = list->cap == 0 ? 8 : list->cap * 2;
        char** new_items = realloc(list->items, new_cap * sizeof(char*));
        if (!new_items) {
            panic("realloc failed");
        }
        list->items = new_items;
        list->cap   = new_cap;
    }

    list->items[list->len] = strdupp(s);
    ++list->len;
}

static void string_list_free(struct string_list* list) {
    for (size_t i = 0; i < list->len; ++i) {
        free(list->items[i]);
    }

    free(list->items);
    list->items = NULL;
    list->len   = 0;
    list->cap   = 0;
}

// Calls the ListSpans method and returns the reply.
static sd_bus_message* call_list_spans(sd_bus* bus, const char* storage_id) {
    const uint32_t max_results = 1000;

    _cleanup_(sd_bus_error_free) sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message* reply                           = NULL;

    int r = sd_bus_call_method(bus,
                               RECORDING_SEARCH_BUS_NAME,
                               RECORDING_SEARCH_OBJECT_PATH,
                               RECORDING_SEARCH_INTERFACE,
                               "ListSpans",
                               &error,
                               &reply,
                               "ssssssu",
                               storage_id,
                               "", /* group_id */
                               "", /* start_time */
                               "", /* stop_time */
                               "", /* video_channel */
                               "", /* audio_channel */
                               max_results);
    if (r < 0)
        panic("ListSpans(storage_id='%s') failed: %s",
              storage_id,
              error.message ? error.message : strerror(-r));

    return reply;
}

// Calls the GetSpanAttributes method and returns the reply.
static sd_bus_message*
call_get_span_attributes(sd_bus* bus, const char* storage_id, const char* span_id) {
    _cleanup_(sd_bus_error_free) sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message* reply                           = NULL;

    int r = sd_bus_call_method(bus,
                               RECORDING_SEARCH_BUS_NAME,
                               RECORDING_SEARCH_OBJECT_PATH,
                               RECORDING_SEARCH_INTERFACE,
                               "GetSpanAttributes",
                               &error,
                               &reply,
                               "ss",
                               storage_id,
                               span_id);
    if (r < 0)
        panic("GetSpanAttributes(storage_id='%s', span_id='%s') failed: %s",
              storage_id,
              span_id,
              error.message ? error.message : strerror(-r));

    return reply;
}

// Calls the GetSegment method and returns the reply.
static sd_bus_message* call_get_segment(sd_bus* bus,
                                        const char* storage_id,
                                        const char* span_id,
                                        const char* container_id,
                                        const char* search_time) {
    _cleanup_(sd_bus_error_free) sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message* reply                           = NULL;

    int r = sd_bus_call_method(bus,
                               RECORDING_PLAYBACK_BUS_NAME,
                               RECORDING_PLAYBACK_OBJECT_PATH,
                               RECORDING_PLAYBACK_INTERFACE,
                               "GetSegment",
                               &error,
                               &reply,
                               "ssss",
                               storage_id,
                               span_id,
                               container_id,
                               search_time);
    if (r < 0) {
        if (!sd_bus_error_has_name(&error, "com.axis.RecordingPlayback1.Error.SegmentNotFound"))
            panic(
                "GetSegment(storage_id='%s', span_id='%s', container_id='%s', search_time='%s') "
                "failed: %s",
                storage_id,
                span_id,
                container_id,
                search_time,
                error.message ? error.message : strerror(-r));
    }

    return reply;
}

// Parses a ListSpans reply and returns a list of span IDs.
static struct string_list parse_list_spans_reply(sd_bus_message* reply) {
    int r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_ARRAY, "(ssss)");
    if (r < 0)
        panic("failed to enter span array: %s", strerror(-r));

    struct string_list span_ids = {0};
    for (;;) {
        const char* group_id   = NULL;
        const char* span_id    = NULL;
        const char* start_time = NULL;
        const char* stop_time  = NULL;
        r = sd_bus_message_read(reply, "(ssss)", &group_id, &span_id, &start_time, &stop_time);
        if (r < 0)
            panic("failed to read span struct: %s", strerror(-r));
        if (r == 0)
            break;

        syslog(LOG_INFO,
               "Span '%s': start_time='%s', stop_time='%s', group_id='%s'",
               span_id,
               start_time,
               stop_time,
               group_id);

        string_list_append(&span_ids, span_id);
    }

    r = sd_bus_message_exit_container(reply);
    if (r < 0)
        panic("failed to exit span array: %s", strerror(-r));

    return span_ids;
}

// Parses an attribute value with signature sv.
static void parse_attribute(sd_bus_message* reply) {
    const char* attr_key = NULL;
    int r                = sd_bus_message_read_basic(reply, SD_BUS_TYPE_STRING, &attr_key);
    if (r < 0)
        panic("failed to read attribute key: %s", strerror(-r));

    const char* contents = NULL;
    r                    = sd_bus_message_peek_type(reply, NULL, &contents);
    if (r < 0)
        panic("failed to peek attribute type: %s", strerror(-r));

    r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_VARIANT, contents);
    if (r < 0)
        panic("failed to enter attribute variant: %s", strerror(-r));

    switch (contents[0]) {
        case SD_BUS_TYPE_STRING: {
            const char* value = NULL;
            r                 = sd_bus_message_read_basic(reply, SD_BUS_TYPE_STRING, &value);
            if (r < 0)
                panic("failed to read attribute value: %s", strerror(-r));

            syslog(LOG_INFO, "      %s='%s'", attr_key, value);
            break;
        }
        case SD_BUS_TYPE_INT32: {
            int32_t value = 0;
            r             = sd_bus_message_read_basic(reply, SD_BUS_TYPE_INT32, &value);
            if (r < 0)
                panic("failed to read attribute value: %s", strerror(-r));

            syslog(LOG_INFO, "      %s=%d", attr_key, value);
            break;
        }
        default: {
            r = sd_bus_message_skip(reply, NULL);
            if (r < 0)
                panic("failed to skip unsupported attribute type '%c': %s",
                      contents[0],
                      strerror(-r));

            syslog(LOG_INFO, "      %s: unsupported type '%c'", attr_key, contents[0]);
            break;
        }
    }

    r = sd_bus_message_exit_container(reply);
    if (r < 0)
        panic("failed to exit attribute variant: %s", strerror(-r));
}

// Parses a tracks value with signature aa{sv}.
// Each array entry is a dict with the track attributes.
static void parse_tracks(sd_bus_message* reply) {
    int r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_ARRAY, "a{sv}");
    if (r < 0)
        panic("failed to enter tracks array: %s", strerror(-r));

    int track_index = 0;
    for (;;) {
        r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_ARRAY, "{sv}");
        if (r < 0)
            panic("failed to enter track attributes dict: %s", strerror(-r));
        if (r == 0)
            break;

        syslog(LOG_INFO, "    Track %d:", track_index++);

        for (;;) {
            r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_DICT_ENTRY, "sv");
            if (r < 0)
                panic("failed to enter track attributes dict entry: %s", strerror(-r));
            if (r == 0)
                break;

            parse_attribute(reply);

            r = sd_bus_message_exit_container(reply);
            if (r < 0)
                panic("failed to exit track attributes dict entry: %s", strerror(-r));
        }

        r = sd_bus_message_exit_container(reply);
        if (r < 0)
            panic("failed to exit track attributes dict: %s", strerror(-r));
    }

    r = sd_bus_message_exit_container(reply);
    if (r < 0)
        panic("failed to exit tracks array: %s", strerror(-r));
}

// Parses a containers value with signature a{s(saa{sv})}.
// Each dict entry maps a container ID to a struct with the container type and an array of tracks.
// Returns a list of container IDs.
static struct string_list parse_containers(sd_bus_message* reply) {
    int r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_ARRAY, "{s(saa{sv})}");
    if (r < 0)
        panic("failed to enter containers dict: %s", strerror(-r));

    struct string_list container_ids = {0};
    for (;;) {
        r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_DICT_ENTRY, "s(saa{sv})");
        if (r < 0)
            panic("failed to enter containers dict entry: %s", strerror(-r));
        if (r == 0)
            break;

        const char* container_id = NULL;
        r = sd_bus_message_read_basic(reply, SD_BUS_TYPE_STRING, &container_id);
        if (r < 0)
            panic("failed to read container id: %s", strerror(-r));

        r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_STRUCT, "saa{sv}");
        if (r < 0)
            panic("failed to enter container struct: %s", strerror(-r));

        const char* container_type = NULL;
        r = sd_bus_message_read_basic(reply, SD_BUS_TYPE_STRING, &container_type);
        if (r < 0)
            panic("failed to read container type: %s", strerror(-r));

        syslog(LOG_INFO, "  Container '%s': type='%s'", container_id, container_type);

        string_list_append(&container_ids, container_id);

        parse_tracks(reply);

        r = sd_bus_message_exit_container(reply);
        if (r < 0)
            panic("failed to exit container struct: %s", strerror(-r));

        r = sd_bus_message_exit_container(reply);
        if (r < 0)
            panic("failed to exit containers dict entry: %s", strerror(-r));
    }

    r = sd_bus_message_exit_container(reply);
    if (r < 0)
        panic("failed to exit containers dict: %s", strerror(-r));

    return container_ids;
}

// Parses the GetSpanAttributes reply and returns a list of container IDs.
static struct string_list parse_get_span_attributes_reply(sd_bus_message* reply) {
    const char* start_time = NULL;
    const char* stop_time  = NULL;
    const char* event_id   = NULL;

    int r = sd_bus_message_read(reply, "sss", &start_time, &stop_time, &event_id);
    if (r < 0)
        panic("failed to read span attributes: %s", strerror(-r));

    syslog(LOG_INFO, "Span attributes: event_id='%s'", event_id);

    return parse_containers(reply);
}

// Parses the GetSegment reply and returns the stop time.
static char* parse_get_segment_reply(sd_bus_message* reply) {
    int fd                 = -1;
    const char* start_time = NULL;
    const char* stop_time  = NULL;
    uint64_t size          = 0;

    int r = sd_bus_message_read(reply, "hsst", &fd, &start_time, &stop_time, &size);
    if (r < 0)
        panic("failed to read segment reply: %s", strerror(-r));

    syslog(LOG_INFO,
           "Segment: start_time='%s', stop_time='%s', size=%" PRIu64,
           start_time,
           stop_time,
           size);

    return strdupp(stop_time);
}

static int on_segment_completed(sd_bus_message* m, void* userdata, sd_bus_error* ret_error) {
    (void)userdata;
    (void)ret_error;

    const char* storage_id   = NULL;
    const char* span_id      = NULL;
    const char* group_id     = NULL;
    const char* container_id = NULL;
    const char* start_time   = NULL;
    const char* stop_time    = NULL;
    uint64_t size            = 0;

    int r = sd_bus_message_read(m,
                                "sssssst",
                                &storage_id,
                                &span_id,
                                &group_id,
                                &container_id,
                                &start_time,
                                &stop_time,
                                &size);
    if (r < 0) {
        syslog(LOG_ERR, "failed to read SegmentCompleted signal: %s", strerror(-r));
        return r;
    }

    syslog(LOG_INFO,
           "Segment: storage_id='%s', span_id='%s', group_id='%s', container_id='%s', "
           "start_time='%s', stop_time='%s', size=%" PRIu64,
           storage_id,
           span_id,
           group_id,
           container_id,
           start_time,
           stop_time,
           size);

    return 0;
}

static struct string_list get_all_span_ids(sd_bus* bus, const char* storage_id) {
    syslog(LOG_INFO, "Get all recording spans for storage '%s' with ListSpans", storage_id);

    _cleanup_(sd_bus_message_unrefp) sd_bus_message* reply = call_list_spans(bus, storage_id);
    struct string_list span_ids                            = parse_list_spans_reply(reply);

    syslog(LOG_INFO, "ListSpans returned %zu span(s)", span_ids.len);

    return span_ids;
}

static struct string_list
get_container_ids(sd_bus* bus, const char* storage_id, const char* span_id) {
    syslog(LOG_INFO, "Get containers for span '%s' with GetSpanAttributes", span_id);

    _cleanup_(sd_bus_message_unrefp) sd_bus_message* reply =
        call_get_span_attributes(bus, storage_id, span_id);
    struct string_list container_ids = parse_get_span_attributes_reply(reply);

    syslog(LOG_INFO, "GetSpanAttributes returned %zu container(s)", container_ids.len);

    return container_ids;
}

static void get_all_segments(sd_bus* bus,
                             const char* storage_id,
                             const char* span_id,
                             const char* container_id) {
    syslog(LOG_INFO,
           "Get all segments for span '%s' and container '%s' with GetSegment",
           span_id,
           container_id);

    _cleanup_(freep) char* search_time = strdupp("");

    for (;;) {
        _cleanup_(sd_bus_message_unrefp) sd_bus_message* reply =
            call_get_segment(bus, storage_id, span_id, container_id, search_time);
        if (!reply)
            break;

        char* stop_time = parse_get_segment_reply(reply);
        if (stop_time[0] == '\0') {
            free(stop_time);
            break;
        }

        free(search_time);
        search_time = stop_time;
    }
}

int main(void) {
    const char* storage_id = "SD_DISK";

    _cleanup_(sd_event_unrefp) sd_event* event = NULL;
    _cleanup_(sd_bus_unrefp) sd_bus* bus       = NULL;

    int r = sd_event_default(&event);
    if (r < 0)
        panic("failed to create event loop: %s", strerror(-r));

    r = sd_bus_open_system(&bus);
    if (r < 0)
        panic("failed to connect to system bus: %s", strerror(-r));

    r = sd_bus_attach_event(bus, event, SD_EVENT_PRIORITY_NORMAL);
    if (r < 0)
        panic("failed to attach bus to event loop: %s", strerror(-r));

    _cleanup_(string_list_free) struct string_list span_ids = get_all_span_ids(bus, storage_id);
    for (size_t i = 0; i < span_ids.len; ++i) {
        _cleanup_(string_list_free) struct string_list container_ids =
            get_container_ids(bus, storage_id, span_ids.items[i]);
        for (size_t j = 0; j < container_ids.len; ++j) {
            get_all_segments(bus, storage_id, span_ids.items[i], container_ids.items[j]);
        }
    }

    syslog(LOG_INFO, "Listening for SegmentCompleted signals");

    r = sd_bus_match_signal(bus,
                            NULL,
                            RECORDING_NOTIFY_BUS_NAME,
                            RECORDING_NOTIFY_OBJECT_PATH,
                            RECORDING_NOTIFY_INTERFACE,
                            "SegmentCompleted",
                            on_segment_completed,
                            NULL);
    if (r < 0)
        panic("failed to add SegmentCompleted signal match: %s", strerror(-r));

    r = sd_event_loop(event);
    if (r < 0)
        panic("failed to run event loop: %s", strerror(-r));

    return 0;
}
