/**
 * Copyright (C) 2023 Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     <http://www.apache.org/licenses/LICENSE-2.0>
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <glib.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

/* AX Serial Port library. */
#include <axsdk/axserialport.h>

#define APP_NAME "axserialport"

/**
 * MyConfigAndData contains application configuration and data.
 */
typedef struct MyConfigAndData {
    AXSerialConfig* config;
    GIOChannel* channel;
    GTimer* timer;
    gpointer data;
} MyConfigAndData;

static GMainLoop* loop = NULL;

/**
 * @brief Signals handling
 *
 * @param signal_num Signal number
 */
static void handle_sigterm(int signo) {
    (void)signo;
    if (loop != NULL) {
        g_main_loop_quit(loop);
    } else {
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Initialize signals
 *
 * @param signo Signal id (not used)
 */
static void init_signals(void) {
    struct sigaction sa;

    sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
}

/**
 * @brief Converts a I/O status to a string
 *
 * @param status I/O status of channel
 *
 * @return Status as string
 */
static gchar* iostatus2str(GIOStatus status) {
    switch (status) {
        case G_IO_STATUS_ERROR:
            return "G_IO_STATUS_ERROR";
        case G_IO_STATUS_NORMAL:
            return "G_IO_STATUS_NORMAL";
        case G_IO_STATUS_EOF:
            return "G_IO_STATUS_EOF";
        case G_IO_STATUS_AGAIN:
            return "G_IO_STATUS_AGAIN";
        default:
            return "Unknown GIOStatus!";
    }
}

/**
 * @brief Callback function registered by g_io_add_watch(),
 *        which is triggered to handle incoming serial data
 *
 * @param channel Channel where the communication happened
 * @param cond Condition that was satisfied when calling the function
 * @param data Incoming data
 *
 * @return Result
 */
static gboolean incoming_data(GIOChannel* channel, GIOCondition cond, gpointer data) {
    MyConfigAndData* conf_data = data;
    GIOChannel* iochannel      = conf_data->channel;
    GError* error              = NULL;
    GIOStatus ret;
    gsize bytes_read;
    gchar timestamp[2];
    gchar str[128];
    (void)channel;
    (void)cond;

    /* We have incoming data, read it! */
    ret = g_io_channel_read_chars(iochannel, timestamp, sizeof(timestamp), &bytes_read, &error);
    if (ret == G_IO_STATUS_NORMAL) {
        /* All OK! write the timestamp to stdout and syslog */
        guchar min = timestamp[0];
        guchar sec = timestamp[1];
        g_snprintf(str, 128, "%s() timestamp: %02u:%02u", __FUNCTION__, min, sec);
        syslog(LOG_INFO, "%s", str);
    } else {
        if (error) { /* Report error */
            syslog(LOG_ERR, "%s", error->message);
            g_error_free(error); /* Free error */
        }
    }

    /* Return FALSE if the event source should be removed */
    return TRUE;
}

/**
 * @brief Callback function registered by g_timeout_add_seconds(),
 *        which is triggered every 10th second and sends out a two
 *        byte timestamp on the serial port
 *
 * @param data The storage to subscribe to its events
 *
 * @return Result
 */
static gboolean send_timer_data(gpointer data) {
    MyConfigAndData* conf_data = data;
    GIOChannel* iochannel      = conf_data->channel;
    GTimer* timer              = conf_data->timer;
    GError* error              = NULL;
    GIOStatus ret;
    gdouble elapsed;
    gchar min, sec;
    gsize bytes_written;
    gchar timestamp[2];

    /* time in seconds since timer started */
    elapsed      = g_timer_elapsed(timer, NULL);
    min          = elapsed / 60; /* Wraps 256->0 */
    sec          = ((guint)elapsed) % 60;
    timestamp[0] = min;
    timestamp[1] = sec;

    ret = g_io_channel_write_chars(iochannel, timestamp, sizeof(timestamp), &bytes_written, &error);
    if (ret == G_IO_STATUS_NORMAL) {
        /* Flush the write buffer */
        ret = g_io_channel_flush(iochannel, &error);
        /* Log the return status from g_io_channel_write_chars() */
        g_message("%s() wrote %ld bytes, status:'%s'",
                  __FUNCTION__,
                  (long int)bytes_written,
                  iostatus2str(ret));
    }

    if (error) { /* Report to stderr and syslog */
        syslog(LOG_ERR, "%s", error->message);
        g_error_free(error); /* Free error */
    }

    /* Return FALSE if the event source should be removed */
    return TRUE;
}

/**
 * @brief Main function
 *
 * @return Result
 */
int main(void) {
    gint fd                = 0;
    gint ret               = 0;
    gint status            = EXIT_FAILURE;
    guint port0            = 0;
    AXSerialConfig* config = NULL;
    GIOChannel* iochannel  = NULL;
    GError* error          = NULL;
    MyConfigAndData conf_data;

    /* Initialization */
    init_signals();
    loop = g_main_loop_new(NULL, FALSE);

    /* Print some startup messages */
    openlog(APP_NAME, LOG_PID, LOG_LOCAL4);
    syslog(LOG_INFO, "Starting AxSerialPort application");

    /* Create a configuration for the first port (port0) */
    config = ax_serial_init(port0, &error);
    if (!config) {
        goto error_out;
    }

    /* Config example (product dependent) see product datasheet.
     * Enable Port, Baudrate 19200, No Bias, 8 Databits, No Parity,
     * RS485 4-Wire, 1 Stopbit, No Termination */
    ax_serial_port_enable(config, AX_SERIAL_ENABLE, NULL);
    ax_serial_set_baudrate(config, AX_SERIAL_B19200, NULL);
    ax_serial_set_bias(config, AX_SERIAL_DISABLE, NULL);
    ax_serial_set_databits(config, AX_SERIAL_DATABITS_8, NULL);
    ax_serial_set_parity(config, AX_SERIAL_PARITY_NONE, NULL);
    ax_serial_set_portmode(config, AX_SERIAL_RS485_4, NULL);
    ax_serial_set_stopbits(config, AX_SERIAL_STOPBITS_1, NULL);
    ax_serial_set_termination(config, AX_SERIAL_DISABLE, NULL);

    /* Synchronize (set) configuration */
    ret = ax_serial_sync_port_settings(config, &error);
    if (!ret) {
        goto error_out;
    }

    /* Get the file descriptor associated with the configured port. */
    fd = ax_serial_get_fd(config, &error);
    if (fd < 0) {
        goto error_out;
    }

    /* Create a GIOChannel from the fd */
    iochannel = g_io_channel_unix_new(fd);
    if (!iochannel) {
        goto error_out;
    }

    /* Set IO Channel encoding to RAW (default is UTF-8) */
    if (G_IO_STATUS_NORMAL != g_io_channel_set_encoding(iochannel, NULL, &error)) {
        goto error_out;
    }

    /* Prepare the conf_data structure */
    conf_data.config  = config;
    conf_data.channel = iochannel;
    conf_data.timer   = g_timer_new(); /* Create and start a timer */

    /* Add a watch that waits for incoming data, then calls 'incoming_data()'
     * when the conditions are met. */
    g_io_add_watch(iochannel,     /* GIOChannel */
                   G_IO_IN,       /* GIOCondition: On incoming data */
                   incoming_data, /* The function to be called */
                   &conf_data     /* Data to pass to function */
    );

    /* Periodically call 'send_timer_data()' every 10 seconds */
    g_timeout_add_seconds(10, send_timer_data, &conf_data);

    /* start the main loop */
    g_main_loop_run(loop);

    /* unref the main loop when the main loop has been quit */
    g_main_loop_unref(loop);

    status = EXIT_SUCCESS;

error_out:
    if (error) {
        /* Report error */
        g_critical("%s", error->message);
        syslog(LOG_ERR, "%s", error->message);
        /* free error */
        g_error_free(error);
        status = EXIT_FAILURE;
    }

    /* close the I/O channel, no flush */
    if (iochannel != NULL) {
        g_io_channel_shutdown(iochannel, FALSE, NULL);
        g_io_channel_unref(iochannel);
    }

    /* clean up and exit */
    ax_serial_cleanup(config);
    syslog(LOG_INFO, "Finish AXSerialPort application");

    exit(status);
}
