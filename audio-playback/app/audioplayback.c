/**
 * Copyright (C) 2025, Axis Communications AB, Lund, Sweden
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
 * - audioplayback -
 *
 * This application is a basic pipewire application using a pipewire mainloop to
 * process audio data.
 *
 * The application starts an audio stream for each output node that plays a sine
 * tone. The log messages can be followed with the command:
 *
 * journalctl -t audioplayback -f
 *
 * The application listens for registry events to find the nodes to play audio
 * to.
 *
 * Suppose that you have gone through the steps of installation. Then you can
 * also run it on your device like this:
 *
 *     /usr/local/packages/audioplayback/audioplayback
 *
 * and then the output will go to stderr instead of the system log.
 */

#include <math.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#pragma GCC diagnostic pop

#define FREQUENCY 440
#define VOLUME    0.5f

PW_LOG_TOPIC_STATIC(topic, "audioplayback");
#define PW_LOG_TOPIC_DEFAULT topic

/* The state of the application, to be shared between functions. */
struct impl {
    struct pw_main_loop* loop;
    struct pw_core* core;
    struct pw_context* context;
    struct pw_registry* registry;
    struct spa_hook registry_listener;
    regex_t node_name_regex;
    struct spa_list streams;
};

struct stream_data {
    struct spa_list link;
    struct pw_stream* stream;
    struct spa_hook stream_listener;
    uint32_t target_id;
    char target_name[64];
    struct spa_audio_info info;
    float angle;
};

/**
 * A callback function that will be called from the mainloop when stream
 * parameters have been set.
 */
static void on_param_changed(void* data, uint32_t id, const struct spa_pod* param) {
    struct stream_data* stream_data = data;
    int res;

    if (param == NULL || id != SPA_PARAM_Format) {
        return;
    }

    res = spa_format_parse(param, &stream_data->info.media_type, &stream_data->info.media_subtype);
    if (res < 0) {
        pw_log_warn("Failed to parse format from %s: %s", stream_data->target_name, strerror(-res));
        return;
    }

    if (stream_data->info.media_type != SPA_MEDIA_TYPE_audio ||
        stream_data->info.media_subtype != SPA_MEDIA_SUBTYPE_raw) {
        pw_log_warn("Format from %s is not raw audio.", stream_data->target_name);
        return;
    }

    spa_format_audio_raw_parse(param, &stream_data->info.info.raw);

    pw_log_info("Playing to node %s, rate %d.",
                stream_data->target_name,
                stream_data->info.info.raw.rate);
}

/**
 * A callback function that will be called from the mainloop when stream
 * state has been changed.
 */
static void on_state_changed(void* data,
                             enum pw_stream_state old,
                             enum pw_stream_state state,
                             const char* error) {
    struct stream_data* stream_data = data;

    pw_log_debug("State for stream from %s changed %s -> %s",
                 stream_data->target_name,
                 pw_stream_state_as_string(old),
                 pw_stream_state_as_string(state));

    if (state == PW_STREAM_STATE_ERROR) {
        pw_log_warn("Stream from %s got error: %s", stream_data->target_name, error);
    }
}

/**
 * A process callback function that will be called from the mainloop when there
 * is a new buffer to fill with audio data.
 */
static void on_process(void* data) {
    struct stream_data* stream_data = data;
    struct pw_buffer* b;
    struct spa_buffer* buf;
    float* samples;
    uint32_t n_samples;
    unsigned int i;

    b = pw_stream_dequeue_buffer(stream_data->stream);
    if (b == NULL) {
        pw_log_warn("Out of buffers: %m");
        return;
    }
    buf = b->buffer;

    samples = buf->datas[0].data;
    if (samples == NULL) {
        pw_log_warn("No data in buffer.");
        goto out;
    }
    n_samples = buf->datas[0].maxsize / sizeof(float);

    /* Fill the buffer with a sine wave. Remember the angle until next call. */
    for (i = 0; i < n_samples; i++) {
        samples[i] = sinf(stream_data->angle) * VOLUME;

        stream_data->angle += (float)(2 * M_PI * FREQUENCY) / stream_data->info.info.raw.rate;
        if (stream_data->angle >= 2 * M_PI) {
            stream_data->angle -= 2 * M_PI;
        }
    }

    /* Set buffer metadata. */
    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = sizeof(float);
    buf->datas[0].chunk->size   = n_samples * sizeof(float);

out:
    pw_stream_queue_buffer(stream_data->stream, b);
}

/**
 * A signal callback function that will be called from the mainloop.
 */
static void on_signal(void* data, int signal_num) {
    struct impl* impl = data;

    pw_log_info("Got signal %d, quit main loop.", signal_num);
    pw_main_loop_quit(impl->loop);
}

static const struct pw_stream_events stream_events = {PW_VERSION_STREAM_EVENTS,
                                                      .param_changed = on_param_changed,
                                                      .process       = on_process,
                                                      .state_changed = on_state_changed};

/**
 * A callback function that will be called from the mainloop when there are new
 * global objects, such as nodes, in pipewire. It will be called for all
 * existing objects when the context is connected.
 */
static void registry_event_global(void* data,
                                  uint32_t id,
                                  uint32_t permissions,
                                  const char* type,
                                  uint32_t version,
                                  const struct spa_dict* props) {
    (void)permissions;
    (void)version;
    struct impl* impl = data;

    if (spa_streq(type, PW_TYPE_INTERFACE_Node)) {
        const char* name;
        struct pw_properties* stream_props;
        uint8_t buf[1024];
        struct spa_pod_builder builder = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
        const struct spa_pod* params[1];
        struct stream_data* stream_data;
        int res;

        name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
        if (regexec(&impl->node_name_regex, name, 0, NULL, 0) == 0) {
            pw_log_info("Found node %s with id %u.", name, id);
        } else {
            pw_log_debug("Ignore node %s with id %u.", name, id);
            return;
        }

        stream_props = pw_properties_new(PW_KEY_MEDIA_TYPE,
                                         "Audio",
                                         PW_KEY_MEDIA_CATEGORY,
                                         "Playback",
                                         PW_KEY_TARGET_OBJECT,
                                         name,
                                         NULL);
        if (stream_props == NULL) {
            pw_log_warn("Could not create properties for %s.", name);
            return;
        }

        /* Create a stream. */
        stream_data            = calloc(1, sizeof(struct stream_data));
        stream_data->target_id = id;
        strncpy(stream_data->target_name, name, sizeof(stream_data->target_name) - 1);
        stream_data->stream = pw_stream_new(impl->core, "Audio playback", stream_props);
        if (stream_data->stream == NULL) {
            pw_log_warn("Could not create stream for %s: %m", name);
            return;
        }
        pw_stream_add_listener(stream_data->stream,
                               &stream_data->stream_listener,
                               &stream_events,
                               stream_data);

        /* Leave rate empty to accept the native device rate. */
        params[0] = spa_format_audio_raw_build(
            &builder,
            SPA_PARAM_EnumFormat,
            &SPA_AUDIO_INFO_RAW_INIT(.channels = 1, .format = SPA_AUDIO_FORMAT_F32P));

        /* Connect to pipewire. */
        res = pw_stream_connect(stream_data->stream,
                                PW_DIRECTION_OUTPUT,
                                PW_ID_ANY,
                                PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS,
                                params,
                                SPA_N_ELEMENTS(params));
        if (res < 0) {
            pw_log_error("Could not connect stream for %s: %s", name, strerror(-res));
            return;
        }

        spa_list_append(&impl->streams, &stream_data->link);
    }
}

/**
 * A callback function that will be called from the mainloop when a global
 * objects, such as nodes, has been removed.
 */
static void registry_event_global_remove(void* data, uint32_t id) {
    struct impl* impl = data;
    struct stream_data* stream_data;

    pw_log_debug("Removed pipewire object with id %u.", id);

    spa_list_for_each(stream_data, &impl->streams, link) {
        if (stream_data->target_id == id) {
            pw_log_info("Destroy stream from %s.", stream_data->target_name);
            spa_hook_remove(&stream_data->stream_listener);
            pw_stream_destroy(stream_data->stream);
            spa_list_remove(&stream_data->link);
            break;
        }
    }
}

static const struct pw_registry_events registry_events = {
    PW_VERSION_REGISTRY_EVENTS,
    .global        = registry_event_global,
    .global_remove = registry_event_global_remove,
};

/**
 * Main function that starts the main loop.
 */
int main(int argc, char* argv[]) {
    struct impl impl = {0};
    int res;
    struct pw_loop* loop;
    struct stream_data* stream_data;

    /* Compile a regex for node names to match. */
    res =
        regcomp(&impl.node_name_regex, "^AudioDevice[0-9]+Output[0-9]+$", REG_EXTENDED | REG_NOSUB);
    if (res != 0) {
        pw_log_error("Cannot compile regex: %d", res);
        return EXIT_FAILURE;
    }

    /* Enable all messages from the audioplayback category plus warning and
     * error level messages from all other categorys to be sent to the system
     * log. */
    setenv("PIPEWIRE_DEBUG", "audioplayback:5,2", 1);

    pw_init(&argc, &argv);

    /* Create a main loop. */
    impl.loop = pw_main_loop_new(NULL);
    if (impl.loop == NULL) {
        pw_log_error("Could not create main loop: %m");
        return EXIT_FAILURE;
    }
    loop = pw_main_loop_get_loop(impl.loop);

    pw_loop_add_signal(loop, SIGINT, on_signal, &impl);
    pw_loop_add_signal(loop, SIGTERM, on_signal, &impl);

    impl.context = pw_context_new(loop, NULL, 0);
    if (impl.context == NULL) {
        pw_log_error("Cannot get pipewire context.");
        return EXIT_FAILURE;
    }

    impl.core = pw_context_connect(impl.context, NULL, 0);
    if (impl.core == NULL) {
        pw_log_error("Cannot connect to pipewire.");
        return EXIT_FAILURE;
    }

    impl.registry = pw_core_get_registry(impl.core, PW_VERSION_REGISTRY, 0);
    pw_registry_add_listener(impl.registry, &impl.registry_listener, &registry_events, &impl);

    spa_list_init(&impl.streams);

    pw_log_info("Starting.");

    /* Start processing. */
    pw_main_loop_run(impl.loop);

    spa_hook_remove(&impl.registry_listener);
    pw_proxy_destroy((struct pw_proxy*)impl.registry);
    spa_list_consume(stream_data, &impl.streams, link) {
        pw_log_debug("Destroy stream with target node %s.", stream_data->target_name);
        spa_hook_remove(&stream_data->stream_listener);
        pw_stream_destroy(stream_data->stream);
        spa_list_remove(&stream_data->link);
    }
    pw_core_disconnect(impl.core);
    pw_context_destroy(impl.context);
    pw_main_loop_destroy(impl.loop);
    pw_deinit();
    regfree(&impl.node_name_regex);

    pw_log_info("Terminating.");

    return EXIT_SUCCESS;
}
