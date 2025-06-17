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
 * - audiocapture -
 *
 * This application is a basic pipewire application using a pipewire mainloop to
 * process audio data.
 *
 * The application starts an audio stream and calculates the peak values for
 * all of all samples for all channels over a 5 second interval and prints them
 * to the system log. The log messages can be followed with the command:
 *
 * journalctl -t audiocapture -f
 *
 * The application expects one argument on the command line which is the name of
 * the pipewire node to capture audio from.
 *
 * Suppose that you have gone through the steps of installation. Then you can
 * also run it on your device like this:
 *
 *     /usr/local/packages/audiocapture/audiocapture \
 *         AudioDevice0Input0.Unprocessed
 *
 * and then the output will go to stderr instead of the system log.
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

PW_LOG_TOPIC_STATIC(topic, "audiocapture");
#define PW_LOG_TOPIC_DEFAULT topic

/* The state of the application, to be shared between functions. */
struct impl {
    struct pw_main_loop* loop;
    struct pw_core* core;
    struct pw_context* context;
    struct pw_registry* registry;
    struct spa_hook registry_listener;
    struct spa_list streams;
    struct spa_source* timer_source;
};

struct stream_data {
    struct spa_list link;
    struct pw_stream* stream;
    struct spa_hook stream_listener;
    uint32_t target_id;
    char target_name[64];
    struct spa_audio_info info;
    float peaks[SPA_AUDIO_MAX_CHANNELS];
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

    pw_log_info("Capturing from node %s, %d channel(s), rate %d.",
                stream_data->target_name,
                stream_data->info.info.raw.channels,
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
 * are new audio samples to process.
 */
static void on_process(void* data) {
    struct stream_data* stream_data = data;
    struct pw_buffer* b;
    struct spa_buffer* buf;
    unsigned int c;

    b = pw_stream_dequeue_buffer(stream_data->stream);
    if (b == NULL) {
        pw_log_warn("Out of buffers from %s: %m", stream_data->target_name);
        return;
    }
    buf = b->buffer;

    for (c = 0; c < stream_data->info.info.raw.channels; c++) {
        const float* samples;
        uint32_t n_samples;
        float min;
        float max;
        unsigned int i;
        float peak;

        samples = buf->datas[c].data;
        if (samples == NULL) {
            pw_log_warn("No data in buffer from %s, channel %u.", stream_data->target_name, c);
            goto out;
        }
        n_samples = buf->datas[c].chunk->size / sizeof(float);

        min = max = samples[0];
        for (i = 0; i < n_samples; i++) {
            if (samples[i] > max)
                max = samples[i];
            if (samples[i] < min)
                min = samples[i];
        }
        peak = fmaxf(fabs(min), max);
        if (peak > stream_data->peaks[c]) {
            stream_data->peaks[c] = peak;
        }
    }

out:
    pw_stream_queue_buffer(stream_data->stream, b);
}

/**
 * A timer callback function that will be called from the mainloop periodically.
 */
static void on_timeout(void* data, uint64_t expirations) {
    (void)expirations;
    struct impl* impl = data;
    struct stream_data* stream_data;
    unsigned int c;

    spa_list_for_each(stream_data, &impl->streams, link) {
        for (c = 0; c < stream_data->info.info.raw.channels; c++) {
            pw_log_info("Node %s, channel %u, peak %.1f dBFS.",
                        stream_data->target_name,
                        c,
                        20 * log10f(stream_data->peaks[c]));
            stream_data->peaks[c] = 0;
        }
    }
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
        const char* media_class;
        const char* name;
        struct pw_properties* stream_props;
        uint8_t buf[1024];
        struct spa_pod_builder builder = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
        const struct spa_pod* params[1];
        struct stream_data* stream_data;
        int res;

        media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
        name        = spa_dict_lookup(props, PW_KEY_NODE_NAME);
        pw_log_info("Found %s node %s with id %u.", media_class, name, id);

        stream_props = pw_properties_new(PW_KEY_MEDIA_TYPE,
                                         "Audio",
                                         PW_KEY_MEDIA_CATEGORY,
                                         "Capture",
                                         PW_KEY_TARGET_OBJECT,
                                         name,
                                         NULL);
        if (stream_props == NULL) {
            pw_log_warn("Could not create properties for %s.", name);
            return;
        }

        /* Set PW_KEY_STREAM_CAPTURE_SINK to monitor an output node. */
        if (media_class != NULL && strcmp(media_class, "Audio/Sink") == 0) {
            int res;

            res = pw_properties_set(stream_props, PW_KEY_STREAM_CAPTURE_SINK, "true");
            if (res < 0) {
                pw_log_warn("Could not set property for %s: %s", name, strerror(-res));
                return;
            }
        }

        /* Create a stream. */
        stream_data            = calloc(1, sizeof(struct stream_data));
        stream_data->target_id = id;
        strncpy(stream_data->target_name, name, sizeof(stream_data->target_name) - 1);
        stream_data->stream = pw_stream_new(impl->core, "Audio capture", stream_props);
        if (stream_data->stream == NULL) {
            pw_log_warn("Could not create stream for %s: %m", name);
            return;
        }
        pw_stream_add_listener(stream_data->stream,
                               &stream_data->stream_listener,
                               &stream_events,
                               stream_data);

        /* Leave rate and channels empty to accept the native device format. */
        params[0] =
            spa_format_audio_raw_build(&builder,
                                       SPA_PARAM_EnumFormat,
                                       &SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_F32P));

        /* Connect to pipewire. */
        res = pw_stream_connect(stream_data->stream,
                                PW_DIRECTION_INPUT,
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
 * Main function that starts a stream with target node as argument.
 */
int main(int argc, char* argv[]) {
    struct impl impl = {0};
    struct pw_loop* loop;
    struct timespec ts;
    int res;
    struct stream_data* stream_data;

    /* Enable all messages from the audiocapture category plus warning and error
     * level messages from all other categorys to be sent to the system log. */
    setenv("PIPEWIRE_DEBUG", "audiocapture:5,2", 1);

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

    /* Print peaks to the system log periodically every 5 seconds. */
    impl.timer_source = pw_loop_add_timer(loop, on_timeout, &impl);
    if (impl.timer_source == NULL) {
        pw_log_error("Could not create timer source.");
        return EXIT_FAILURE;
    }
    ts.tv_sec  = 5;
    ts.tv_nsec = 0;
    res        = pw_loop_update_timer(loop, impl.timer_source, NULL, &ts, false);
    if (res < 0) {
        pw_log_error("Could not update timer source: %s", strerror(-res));
        return EXIT_FAILURE;
    }

    /* Start processing. */
    pw_main_loop_run(impl.loop);

    pw_loop_destroy_source(loop, impl.timer_source);
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

    pw_log_info("Terminating.");

    return EXIT_SUCCESS;
}
