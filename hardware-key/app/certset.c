#include <gio/gio.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "certset.h"

// Error codes
enum {
    OK = 0,
    ERR_DBUS_CONNECTION = -1,
    ERR_DBUS_METHOD_CALL = -2,
    ERR_CERT_PATH_NOT_FOUND = -3,
    ERR_CERT_READ_FAILED = -4,
    ERR_MEMORY_ALLOCATION_FAILED = -7,
};

/* Create Empty Certset */

int
certset_create_empty_set(const char *certset_name)
{
    int err = OK;
    GError *error = NULL;
    const char *empty_cert[] = { NULL };
    GDBusConnection *connection =
            g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!connection) {
        syslog(LOG_ERR, "Error connecting to D-Bus: %s", error->message);
        g_error_free(error);
        return ERR_DBUS_CONNECTION;
    }

    const char *bus_name = "com.axis.Certificate1";
    const char *object_path = "/com/axis/Certificate1/CertSet";
    const char *interface_name = "com.axis.CertSet1";
    const char *method_name = "Create";

    GVariant *result =
            g_dbus_connection_call_sync(connection,
                                        bus_name,
                                        object_path,
                                        interface_name,
                                        method_name,
                                        g_variant_new("(s^as^as)", certset_name, empty_cert, empty_cert),
                                        NULL,
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
    if (!result) {
        const gchar *remote_error = g_dbus_error_get_remote_error(error);
        if (remote_error != NULL &&
            strcmp(remote_error, "com.axis.CertSet1.Error.SetAlreadyExists") == 0) {
            syslog(LOG_INFO, "Certset '%s' already exists, skipping creation", certset_name);
        } else {
            syslog(LOG_ERR, "Error invoking D-Bus method: %s", error->message);
            err = ERR_DBUS_METHOD_CALL;
        }
        g_error_free(error);
    } else {
        g_variant_unref(result);
    }

    g_object_unref(connection);

    return err;
}

/* Update Certset */
int
certset_update_set(const char *certset_name, const char *cert_alias, const char **ca_aliases)
{
    int err = OK;
    GError *error = NULL;
    const char *certs[] = { cert_alias, NULL };
    GDBusConnection *connection =
            g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!connection) {
        syslog(LOG_ERR, "Error connecting to D-Bus: %s", error->message);
        g_error_free(error);
        return ERR_DBUS_CONNECTION;
    }

    const char *bus_name = "com.axis.Certificate1";
    const char *object_path = "/com/axis/Certificate1/CertSet";
    const char *interface_name = "com.axis.CertSet1";
    const char *method_name = "Update";

    GVariant *result =
            g_dbus_connection_call_sync(connection,
                                        bus_name,
                                        object_path,
                                        interface_name,
                                        method_name,
                                        g_variant_new("(s^as^as)", certset_name, certs, ca_aliases),
                                        NULL,
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
    if (!result) {
        syslog(LOG_ERR, "Error invoking D-Bus method: %s", error->message);
        err = ERR_DBUS_METHOD_CALL;
        g_error_free(error);
    }

    g_object_unref(connection);

    return err;
}

/* GetCertPath */
int
certset_get_cert_path(const char *certset_name, char **path)
{
    int err = OK;
    GError *error = NULL;
    GDBusConnection *connection =
            g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!connection) {
        syslog(LOG_ERR, "Error connecting to D-Bus: %s", error->message);
        g_error_free(error);
        return ERR_DBUS_CONNECTION;
    }

    const char *bus_name = "com.axis.Certificate1";
    const char *object_path = "/com/axis/Certificate1/CertSet";
    const char *interface_name = "com.axis.CertSet1";
    const char *method_name = "GetCertPath";

    GVariant *result =
            g_dbus_connection_call_sync(connection,
                                        bus_name,
                                        object_path,
                                        interface_name,
                                        method_name,
                                        g_variant_new("(s)", certset_name),
                                        NULL,
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
    if (!result) {
        syslog(LOG_ERR, "Error invoking D-Bus method: %s", error->message);
        err = ERR_DBUS_METHOD_CALL;
        g_error_free(error);
    } else {
        const char *cert_path = NULL;
        g_variant_get(result, "(&s)", &cert_path);
        *path = g_strdup(cert_path);
        g_variant_unref(result);
    }

    g_object_unref(connection);

    return err;
}

/* List */
