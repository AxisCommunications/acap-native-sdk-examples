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

#include <errno.h>
#include <glib-unix.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

/* AX Storage library. */
#include <axsdk/axstorage.h>

/**
 * disk_item_t represents one storage device and its values.
 */
typedef struct {
    AXStorage* storage;         /** AXStorage reference. */
    AXStorageType storage_type; /** Storage type */
    gchar* storage_id;          /** Storage device name. */
    gchar* storage_path;        /** Storage path. */
    guint subscription_id;      /** Subscription ID for storage events. */
    gboolean setup;             /** TRUE: storage was set up async, FALSE otherwise. */
    gboolean writable;          /** Storage is writable or not. */
    gboolean available;         /** Storage is available or not. */
    gboolean full;              /** Storage device is full or not. */
    gboolean exiting;           /** Storage is exiting (going to disappear) or not. */
} disk_item_t;

static GList* disks_list = NULL;

/**
 * @brief Handles the signals.
 *
 * @param loop Loop to quit
 */
static gboolean signal_handler(gpointer loop) {
    g_main_loop_quit((GMainLoop*)loop);
    syslog(LOG_INFO, "Application was stopped by SIGTERM or SIGINT.");
    return G_SOURCE_REMOVE;
}

/**
 * @brief Callback function registered by g_timeout_add_seconds(),
 *        which is triggered every 10th second and writes data to disk
 *
 * @param data The storage to subscribe to its events
 *
 * @return Result
 */
static gboolean write_data(const gchar* data) {
    static guint counter = 0;
    GList* node          = NULL;
    gboolean ret         = TRUE;
    for (node = g_list_first(disks_list); node != NULL; node = g_list_next(node)) {
        disk_item_t* item = node->data;

        /* Write data to disk when it is available, writable and has disk space
           and the setup has been done. */
        if (item->available && item->writable && !item->full && item->setup) {
            gchar* filename = g_strdup_printf("%s/%s.log", item->storage_path, data);

            FILE* file = g_fopen(filename, "a");
            if (file == NULL) {
                syslog(LOG_WARNING, "Failed to open %s. Error %s.", filename, g_strerror(errno));
                ret = FALSE;
            } else {
                g_fprintf(file, "counter: %d\n", ++counter);
                fclose(file);
                syslog(LOG_INFO, "Writing to %s", filename);
            }
            g_free(filename);
        }
    }
    return ret;
}

/**
 * @brief Find disk item in disks_list
 *
 * @param storage_id The storage to subscribe to its events
 *
 * @return Disk item
 */
static disk_item_t* find_disk_item_t(gchar* storage_id) {
    GList* node       = NULL;
    disk_item_t* item = NULL;

    for (node = g_list_first(disks_list); node != NULL; node = g_list_next(node)) {
        item = node->data;

        if (g_strcmp0(storage_id, item->storage_id) == 0) {
            return item;
        }
    }
    return NULL;
}

/**
 * @brief Callback function registered by ax_storage_release_async(),
 *        which is triggered to release the disk
 *
 * @param user_data storage_id of a disk
 * @param error Returned errors
 */
static void release_disk_cb(gpointer user_data, GError* error) {
    syslog(LOG_INFO, "Release of %s", (gchar*)user_data);
    if (error != NULL) {
        syslog(LOG_WARNING, "Error while releasing %s: %s", (gchar*)user_data, error->message);
        g_error_free(error);
    }
}

/**
 * @brief Free disk items from disks_list
 */
static void free_disk_item_t(void) {
    GList* node = NULL;

    for (node = g_list_first(disks_list); node != NULL; node = g_list_next(node)) {
        GError* error     = NULL;
        disk_item_t* item = node->data;

        if (item->setup) {
            /* NOTE: It is advised to finish all your reading/writing operations
               before releasing the storage device. */
            ax_storage_release_async(item->storage, release_disk_cb, item->storage_id, &error);
            if (error != NULL) {
                syslog(LOG_WARNING,
                       "Failed to release %s. Error: %s",
                       item->storage_id,
                       error->message);
                g_clear_error(&error);
            } else {
                syslog(LOG_INFO, "Release of %s was successful", item->storage_id);
                item->setup = FALSE;
            }
        }

        ax_storage_unsubscribe(item->subscription_id, &error);
        if (error != NULL) {
            syslog(LOG_WARNING,
                   "Failed to unsubscribe event of %s. Error: %s",
                   item->storage_id,
                   error->message);
            g_clear_error(&error);
        } else {
            syslog(LOG_INFO, "Unsubscribed events of %s", item->storage_id);
        }
        g_free(item->storage_id);
        g_free(item->storage_path);
    }
    g_list_free(disks_list);
}

/**
 * @brief Callback function registered by ax_storage_setup_async(),
 *        which is triggered to setup a disk
 *
 * @param storage storage_id of a disk
 * @param user_data
 * @param error Returned errors
 */
static void setup_disk_cb(AXStorage* storage, gpointer user_data, GError* error) {
    GError* ax_error  = NULL;
    gchar* storage_id = NULL;
    gchar* path       = NULL;
    AXStorageType storage_type;
    (void)user_data;

    if (storage == NULL || error != NULL) {
        syslog(LOG_ERR, "Failed to setup disk. Error: %s", error->message);
        g_error_free(error);
        goto free_variables;
    }

    storage_id = ax_storage_get_storage_id(storage, &ax_error);
    if (ax_error != NULL) {
        syslog(LOG_WARNING, "Failed to get storage_id. Error: %s", ax_error->message);
        g_error_free(ax_error);
        goto free_variables;
    }

    path = ax_storage_get_path(storage, &ax_error);
    if (ax_error != NULL) {
        syslog(LOG_WARNING, "Failed to get storage path. Error: %s", ax_error->message);
        g_error_free(ax_error);
        goto free_variables;
    }

    storage_type = ax_storage_get_type(storage, &ax_error);
    if (ax_error != NULL) {
        syslog(LOG_WARNING, "Failed to get storage type. Error: %s", ax_error->message);
        g_error_free(ax_error);
        goto free_variables;
    }

    disk_item_t* disk = find_disk_item_t(storage_id);
    /* The storage pointer is created in this callback, assign it to
       disk_item_t instance. */
    disk->storage      = storage;
    disk->storage_type = storage_type;
    disk->storage_path = g_strdup(path);
    disk->setup        = TRUE;

    syslog(LOG_INFO, "Disk: %s has been setup in %s", storage_id, path);
free_variables:
    g_free(storage_id);
    g_free(path);
}

/**
 * @brief Subscribe to the events of the storage
 *
 * @param storage_id The storage to subscribe to its events
 * @param user_data User data to be processed
 * @param error Returned errors
 */
static void subscribe_cb(gchar* storage_id, gpointer user_data, GError* error) {
    GError* ax_error = NULL;
    gboolean available;
    gboolean writable;
    gboolean full;
    gboolean exiting;
    (void)user_data;

    if (error != NULL) {
        syslog(LOG_WARNING, "Failed to subscribe to %s. Error: %s", storage_id, error->message);
        g_error_free(error);
        return;
    }

    syslog(LOG_INFO, "Subscribe for the events of %s", storage_id);
    disk_item_t* disk = find_disk_item_t(storage_id);

    /* Get the status of the events. */
    exiting = ax_storage_get_status(storage_id, AX_STORAGE_EXITING_EVENT, &ax_error);
    if (ax_error != NULL) {
        syslog(LOG_WARNING,
               "Failed to get EXITING event for %s. Error: %s",
               storage_id,
               ax_error->message);
        g_error_free(ax_error);
        return;
    }

    available = ax_storage_get_status(storage_id, AX_STORAGE_AVAILABLE_EVENT, &ax_error);
    if (ax_error != NULL) {
        syslog(LOG_WARNING,
               "Failed to get AVAILABLE event for %s. Error: %s",
               storage_id,
               ax_error->message);
        g_error_free(ax_error);
        return;
    }

    writable = ax_storage_get_status(storage_id, AX_STORAGE_WRITABLE_EVENT, &ax_error);
    if (ax_error != NULL) {
        syslog(LOG_WARNING,
               "Failed to get WRITABLE event for %s. Error: %s",
               storage_id,
               ax_error->message);
        g_error_free(ax_error);
        return;
    }

    full = ax_storage_get_status(storage_id, AX_STORAGE_FULL_EVENT, &ax_error);
    if (ax_error != NULL) {
        syslog(LOG_WARNING,
               "Failed to get FULL event for %s. Error: %s",
               storage_id,
               ax_error->message);
        g_error_free(ax_error);
        return;
    }

    disk->writable  = writable;
    disk->available = available;
    disk->exiting   = exiting;
    disk->full      = full;

    syslog(LOG_INFO,
           "Status of events for %s: %swritable, %savailable, %sexiting, %sfull",
           storage_id,
           writable ? "" : "not ",
           available ? "" : "not ",
           exiting ? "" : "not ",
           full ? "" : "not ");

    /* If exiting, and the disk was set up before, release it. */
    if (exiting && disk->setup) {
        /* NOTE: It is advised to finish all your reading/writing operations before
           releasing the storage device. */
        ax_storage_release_async(disk->storage, release_disk_cb, storage_id, &ax_error);

        if (ax_error != NULL) {
            syslog(LOG_WARNING, "Failed to release %s. Error %s.", storage_id, ax_error->message);
            g_error_free(ax_error);
        } else {
            syslog(LOG_INFO, "Release of %s was successful", storage_id);
            disk->setup = FALSE;
        }

        /* Writable implies that the disk is available. */
    } else if (writable && !full && !exiting && !disk->setup) {
        syslog(LOG_INFO, "Setup %s", storage_id);
        ax_storage_setup_async(storage_id, setup_disk_cb, NULL, &ax_error);

        if (ax_error != NULL) {
            /* NOTE: It is advised to try to setup again in case of failure. */
            syslog(LOG_WARNING, "Failed to setup %s, reason: %s", storage_id, ax_error->message);
            g_error_free(ax_error);
        } else {
            syslog(LOG_INFO, "Setup of %s was successful", storage_id);
        }
    }
}

/**
 * @brief Subscribes to disk events and creates new disk item
 *
 * @param storage_id storage_id of a disk
 *
 * @return The item
 */
static disk_item_t* new_disk_item_t(gchar* storage_id) {
    GError* error     = NULL;
    disk_item_t* item = NULL;
    guint subscription_id;

    /* Subscribe to disks events. */
    subscription_id = ax_storage_subscribe(storage_id, subscribe_cb, NULL, &error);
    if (subscription_id == 0 || error != NULL) {
        syslog(LOG_ERR,
               "Failed to subscribe to events of %s. Error: %s",
               storage_id,
               error->message);
        g_clear_error(&error);
        return NULL;
    }

    item                  = g_new0(disk_item_t, 1);
    item->subscription_id = subscription_id;
    item->storage_id      = g_strdup(storage_id);
    item->setup           = FALSE;

    return item;
}

/**
 * @brief Main function
 *
 * @return Result
 */
gint main(void) {
    GList* disks    = NULL;
    GList* node     = NULL;
    GError* error   = NULL;
    GMainLoop* loop = NULL;
    gint ret        = EXIT_SUCCESS;

    syslog(LOG_INFO, "Start AXStorage application");

    disks = ax_storage_list(&error);
    if (error != NULL) {
        syslog(LOG_WARNING, "Failed to list storage devices. Error: (%s)", error->message);
        g_error_free(error);
        ret = EXIT_FAILURE;
        /* Note: It is advised to get the list more than once, in case of failure.*/
        goto out;
    }

    loop = g_main_loop_new(NULL, FALSE);
    g_unix_signal_add(SIGTERM, signal_handler, loop);
    g_unix_signal_add(SIGINT, signal_handler, loop);

    /* Loop through the retrieved disks and subscribe to their events. */
    for (node = g_list_first(disks); node != NULL; node = g_list_next(node)) {
        gchar* disk_name  = (gchar*)node->data;
        disk_item_t* item = new_disk_item_t(disk_name);
        if (item == NULL) {
            syslog(LOG_WARNING, "%s is skipped", disk_name);
            g_free(node->data);
            continue;
        }
        disks_list = g_list_append(disks_list, item);
        g_free(node->data);
    }
    g_list_free(disks);

    /* Write contents to two files. */
    gchar* file1 = g_strdup("file1");
    gchar* file2 = g_strdup("file2");
    g_timeout_add_seconds(10, (GSourceFunc)write_data, file1);
    g_timeout_add_seconds(10, (GSourceFunc)write_data, file2);

    /* start the main loop */
    g_main_loop_run(loop);

    free_disk_item_t();
    g_free(file1);
    g_free(file2);
    /* unref the main loop when the main loop has been quit */
    g_main_loop_unref(loop);

out:
    syslog(LOG_INFO, "Finish AXStorage application");
    return ret;
}
