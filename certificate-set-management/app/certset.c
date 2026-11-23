// Copyright (C) 2026 Axis Communications AB, Lund, Sweden
// Licensed under the MIT License. See LICENSE file for details.

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <systemd/sd-bus.h>

#include "certset.h"

enum {
    OK                           = 0,
    ERR_DBUS_CONNECTION          = -1,
    ERR_DBUS_METHOD_CALL         = -2,
    ERR_CERT_PATH_NOT_FOUND      = -3,
    ERR_CERT_READ_FAILED         = -4,
    ERR_INVALID_ARGUMENT         = -5,
    ERR_MEMORY_ALLOCATION_FAILED = -7,
};

static const char* const bus_name         = "com.axis.Certificate1";
static const char* const object_path      = "/com/axis/Certificate1/CertSet";
static const char* const interface_name   = "com.axis.CertSet1";
static const char* const set_exists_error = "com.axis.CertSet1.Error.SetAlreadyExists";

typedef int (*CertsetArgumentAppender)(sd_bus_message* message, const void* arguments);

static int append_certset_name(sd_bus_message* message, const void* arguments) {
    const char* certset_name = arguments;
    int r = sd_bus_message_append_basic(message, SD_BUS_TYPE_STRING, certset_name);
    if (r < 0) {
        syslog(LOG_ERR, "Error appending certset name: %s", strerror(-r));
        return ERR_DBUS_METHOD_CALL;
    }

    return OK;
}

static int append_empty_set_arguments(sd_bus_message* message, const void* arguments) {
    int err = append_certset_name(message, arguments);
    if (err != OK)
        return err;

    int r = sd_bus_message_open_container(message, SD_BUS_TYPE_ARRAY, "s");
    if (r < 0) {
        syslog(LOG_ERR, "Error opening cert array: %s", strerror(-r));
        return ERR_DBUS_METHOD_CALL;
    }
    r = sd_bus_message_close_container(message);
    if (r < 0) {
        syslog(LOG_ERR, "Error closing cert array: %s", strerror(-r));
        return ERR_DBUS_METHOD_CALL;
    }

    r = sd_bus_message_open_container(message, SD_BUS_TYPE_ARRAY, "s");
    if (r < 0) {
        syslog(LOG_ERR, "Error opening CA array: %s", strerror(-r));
        return ERR_DBUS_METHOD_CALL;
    }
    r = sd_bus_message_close_container(message);
    if (r < 0) {
        syslog(LOG_ERR, "Error closing CA array: %s", strerror(-r));
        return ERR_DBUS_METHOD_CALL;
    }

    return OK;
}

static int certset_call_method(const char* method_name,
                               CertsetArgumentAppender append_arguments,
                               const void* arguments,
                               const char* ignored_error_name,
                               const char* ignored_error_subject,
                               sd_bus_message** reply) {
    int err                  = OK;
    int r                    = 0;
    sd_bus* bus              = NULL;
    sd_bus_message* message  = NULL;
    sd_bus_message* response = NULL;
    sd_bus_error bus_error   = SD_BUS_ERROR_NULL;

    r = sd_bus_open_system(&bus);
    if (r < 0) {
        syslog(LOG_ERR, "Error connecting to D-Bus: %s", strerror(-r));
        return ERR_DBUS_CONNECTION;
    }

    r = sd_bus_message_new_method_call(bus,
                                       &message,
                                       bus_name,
                                       object_path,
                                       interface_name,
                                       method_name);
    if (r < 0) {
        syslog(LOG_ERR, "Error creating D-Bus method call: %s", strerror(-r));
        err = ERR_DBUS_METHOD_CALL;
        goto cleanup;
    }

    if (append_arguments) {
        err = append_arguments(message, arguments);
        if (err != OK)
            goto cleanup;
    }

    r = sd_bus_call(bus, message, 0, &bus_error, &response);
    if (r < 0) {
        if (ignored_error_name && bus_error.name &&
            strcmp(bus_error.name, ignored_error_name) == 0) {
            syslog(LOG_INFO,
                   "Certset '%s' already exists, skipping creation",
                   ignored_error_subject);
        } else {
            const char* msg_text = bus_error.message ? bus_error.message : strerror(-r);
            syslog(LOG_ERR, "Error invoking D-Bus method: %s", msg_text);
            err = ERR_DBUS_METHOD_CALL;
        }
    }

cleanup:
    sd_bus_error_free(&bus_error);
    if (reply) {
        *reply   = response;
        response = NULL;
    }
    sd_bus_message_unref(response);
    sd_bus_message_unref(message);
    sd_bus_unref(bus);

    return err;
}

int certset_create_empty_set(const char* certset_name) {
    return certset_call_method("Create",
                               append_empty_set_arguments,
                               certset_name,
                               set_exists_error,
                               certset_name,
                               NULL);
}

int certset_delete_set(const char* certset_name) {
    return certset_call_method("Delete", append_certset_name, certset_name, NULL, NULL, NULL);
}

typedef struct {
    const char* certset_name;
    const char* cert_alias;
    const char** ca_aliases;
} UpdateSetArguments;

static int
append_string_array(sd_bus_message* message, const char* const* values, const char* array_name) {
    int r = sd_bus_message_open_container(message, SD_BUS_TYPE_ARRAY, "s");
    if (r < 0) {
        syslog(LOG_ERR, "Error opening %s array: %s", array_name, strerror(-r));
        return ERR_DBUS_METHOD_CALL;
    }

    for (size_t i = 0; values && values[i] != NULL; i++) {
        r = sd_bus_message_append_basic(message, SD_BUS_TYPE_STRING, values[i]);
        if (r < 0) {
            syslog(LOG_ERR, "Error appending %s array value: %s", array_name, strerror(-r));
            return ERR_DBUS_METHOD_CALL;
        }
    }

    r = sd_bus_message_close_container(message);
    if (r < 0) {
        syslog(LOG_ERR, "Error closing %s array: %s", array_name, strerror(-r));
        return ERR_DBUS_METHOD_CALL;
    }

    return OK;
}

static int append_update_set_arguments(sd_bus_message* message, const void* arguments) {
    const UpdateSetArguments* update = arguments;
    const char* certs[]              = {update->cert_alias, NULL};
    int err                          = append_certset_name(message, update->certset_name);
    if (err != OK)
        return err;

    err = append_string_array(message, certs, "cert");
    if (err != OK)
        return err;

    return append_string_array(message, update->ca_aliases, "CA");
}

int certset_update_set(const char* certset_name, const char* cert_alias, const char** ca_aliases) {
    UpdateSetArguments arguments = {
        .certset_name = certset_name,
        .cert_alias   = cert_alias,
        .ca_aliases   = ca_aliases,
    };

    return certset_call_method("Update", append_update_set_arguments, &arguments, NULL, NULL, NULL);
}

int certset_get_cert_path(const char* certset_name, char** path) {
    int err               = OK;
    int r                 = 0;
    sd_bus_message* reply = NULL;
    const char* cert_path = NULL;

    err = certset_call_method("GetCertPath", append_certset_name, certset_name, NULL, NULL, &reply);
    if (err == OK) {
        r = sd_bus_message_read(reply, "s", &cert_path);
        if (r < 0) {
            syslog(LOG_ERR, "Error decoding GetCertPath response: %s", strerror(-r));
            err = ERR_DBUS_METHOD_CALL;
        }
    }

    if (err == OK) {
        *path = strdup(cert_path);
        if (!*path)
            err = ERR_MEMORY_ALLOCATION_FAILED;
    }

    sd_bus_message_unref(reply);

    return err;
}

static void certset_entry_free(CertsetEntry* entry) {
    if (!entry)
        return;

    free(entry->name);
    free(entry->path);

    for (size_t i = 0; i < entry->cert_alias_count; i++) free(entry->cert_aliases[i]);
    free(entry->cert_aliases);

    for (size_t i = 0; i < entry->ca_alias_count; i++) free(entry->ca_aliases[i]);
    free(entry->ca_aliases);

    memset(entry, 0, sizeof(*entry));
}

void certset_list_free(CertsetList* certsets) {
    if (!certsets)
        return;

    for (size_t i = 0; i < certsets->count; i++) certset_entry_free(&certsets->entries[i]);
    free(certsets->entries);
    certsets->entries = NULL;
    certsets->count   = 0;
}

static int append_strdup(char*** items, size_t* count, const char* value) {
    char* copy       = NULL;
    char** new_items = NULL;

    if (!items || !count || !value)
        return -1;

    copy = strdup(value);
    if (!copy)
        return -1;

    new_items = realloc(*items, (*count + 1) * sizeof(char*));
    if (!new_items) {
        free(copy);
        return -1;
    }

    new_items[*count] = copy;
    *items            = new_items;
    (*count)++;
    return 0;
}

static int parse_string_array(sd_bus_message* reply, char*** items, size_t* count) {
    int r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_ARRAY, "s");
    if (r < 0)
        return r;

    const char* item = NULL;
    while ((r = sd_bus_message_read_basic(reply, SD_BUS_TYPE_STRING, &item)) > 0) {
        if (append_strdup(items, count, item) < 0)
            return -ENOMEM;
    }
    if (r < 0)
        return r;

    return sd_bus_message_exit_container(reply);
}

int certset_list(CertsetList* certsets) {
    int err                    = OK;
    int r                      = 0;
    sd_bus_message* reply      = NULL;
    CertsetEntry current_entry = {0};
    int has_current_entry      = 0;

    if (!certsets)
        return ERR_INVALID_ARGUMENT;

    certset_list_free(certsets);

    err = certset_call_method("List", NULL, NULL, NULL, NULL, &reply);
    if (err != OK)
        goto cleanup;

    r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_ARRAY, "{sa{sv}}");
    if (r < 0) {
        syslog(LOG_ERR, "Error decoding List response array: %s", strerror(-r));
        err = ERR_DBUS_METHOD_CALL;
        goto cleanup;
    }

    while ((r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_DICT_ENTRY, "sa{sv}")) > 0) {
        current_entry            = (CertsetEntry){0};
        has_current_entry        = 1;
        const char* set_name_raw = NULL;

        r = sd_bus_message_read_basic(reply, SD_BUS_TYPE_STRING, &set_name_raw);
        if (r < 0) {
            syslog(LOG_ERR, "Error reading certset name: %s", strerror(-r));
            err = ERR_DBUS_METHOD_CALL;
            goto cleanup;
        }

        current_entry.name = strdup(set_name_raw);
        if (!current_entry.name) {
            err = ERR_MEMORY_ALLOCATION_FAILED;
            goto cleanup;
        }

        r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_ARRAY, "{sv}");
        if (r < 0) {
            syslog(LOG_ERR, "Error entering certset value map: %s", strerror(-r));
            err = ERR_DBUS_METHOD_CALL;
            goto cleanup;
        }

        while ((r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_DICT_ENTRY, "sv")) > 0) {
            const char* key       = NULL;
            char value_type       = 0;
            const char* value_sig = NULL;

            r = sd_bus_message_read_basic(reply, SD_BUS_TYPE_STRING, &key);
            if (r < 0) {
                syslog(LOG_ERR, "Error reading List map key: %s", strerror(-r));
                err = ERR_DBUS_METHOD_CALL;
                goto cleanup;
            }

            r = sd_bus_message_peek_type(reply, &value_type, &value_sig);
            if (r < 0 || value_type != SD_BUS_TYPE_VARIANT || !value_sig) {
                syslog(LOG_ERR, "Error inspecting List map value type");
                err = ERR_DBUS_METHOD_CALL;
                goto cleanup;
            }

            r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_VARIANT, value_sig);
            if (r < 0) {
                syslog(LOG_ERR, "Error entering List map variant value: %s", strerror(-r));
                err = ERR_DBUS_METHOD_CALL;
                goto cleanup;
            }

            if (strcmp(value_sig, "as") == 0) {
                char** items      = NULL;
                size_t item_count = 0;
                r                 = parse_string_array(reply, &items, &item_count);
                if (r < 0) {
                    syslog(LOG_ERR,
                           "Error reading string array value: %s",
                           (r == -ENOMEM) ? "Out of memory" : strerror(-r));
                    for (size_t i = 0; i < item_count; i++) free(items[i]);
                    free(items);
                    err = (r == -ENOMEM) ? ERR_MEMORY_ALLOCATION_FAILED : ERR_DBUS_METHOD_CALL;
                    goto cleanup;
                }

                if (strcmp(key, "cert") == 0) {
                    current_entry.cert_aliases     = items;
                    current_entry.cert_alias_count = item_count;
                } else if (strcmp(key, "ca") == 0) {
                    current_entry.ca_aliases     = items;
                    current_entry.ca_alias_count = item_count;
                } else {
                    for (size_t i = 0; i < item_count; i++) free(items[i]);
                    free(items);
                }
            } else if (strcmp(value_sig, "s") == 0) {
                const char* item = NULL;
                r                = sd_bus_message_read_basic(reply, SD_BUS_TYPE_STRING, &item);
                if (r < 0) {
                    syslog(LOG_ERR, "Error reading string value: %s", strerror(-r));
                    err = ERR_DBUS_METHOD_CALL;
                    goto cleanup;
                }
                if (strcmp(key, "path") == 0) {
                    free(current_entry.path);
                    current_entry.path = strdup(item);
                    if (!current_entry.path) {
                        err = ERR_MEMORY_ALLOCATION_FAILED;
                        goto cleanup;
                    }
                }
            } else {
                r = sd_bus_message_skip(reply, value_sig);
                if (r < 0) {
                    syslog(LOG_ERR,
                           "Error skipping unsupported value type '%s': %s",
                           value_sig,
                           strerror(-r));
                    err = ERR_DBUS_METHOD_CALL;
                    goto cleanup;
                }
            }

            r = sd_bus_message_exit_container(reply);
            if (r < 0) {
                syslog(LOG_ERR, "Error leaving List map variant value: %s", strerror(-r));
                err = ERR_DBUS_METHOD_CALL;
                goto cleanup;
            }

            r = sd_bus_message_exit_container(reply);
            if (r < 0) {
                syslog(LOG_ERR, "Error leaving List map entry: %s", strerror(-r));
                err = ERR_DBUS_METHOD_CALL;
                goto cleanup;
            }
        }
        if (r < 0) {
            syslog(LOG_ERR, "Error decoding certset map entries: %s", strerror(-r));
            err = ERR_DBUS_METHOD_CALL;
            goto cleanup;
        }

        r = sd_bus_message_exit_container(reply);
        if (r < 0) {
            syslog(LOG_ERR, "Error leaving certset value map: %s", strerror(-r));
            err = ERR_DBUS_METHOD_CALL;
            goto cleanup;
        }

        r = sd_bus_message_exit_container(reply);
        if (r < 0) {
            syslog(LOG_ERR, "Error leaving certset entry: %s", strerror(-r));
            err = ERR_DBUS_METHOD_CALL;
            goto cleanup;
        }

        CertsetEntry* new_entries =
            realloc(certsets->entries, (certsets->count + 1) * sizeof(CertsetEntry));
        if (!new_entries) {
            certset_entry_free(&current_entry);
            has_current_entry = 0;
            err               = ERR_MEMORY_ALLOCATION_FAILED;
            goto cleanup;
        }
        certsets->entries                  = new_entries;
        certsets->entries[certsets->count] = current_entry;
        certsets->count++;
        has_current_entry = 0;
    }
    if (r < 0) {
        syslog(LOG_ERR, "Error decoding List response entries: %s", strerror(-r));
        err = ERR_DBUS_METHOD_CALL;
        goto cleanup;
    }

    r = sd_bus_message_exit_container(reply);
    if (r < 0) {
        syslog(LOG_ERR, "Error leaving List response array: %s", strerror(-r));
        err = ERR_DBUS_METHOD_CALL;
        goto cleanup;
    }

cleanup:
    if (has_current_entry)
        certset_entry_free(&current_entry);
    if (err != OK)
        certset_list_free(certsets);
    sd_bus_message_unref(reply);

    return err;
}
