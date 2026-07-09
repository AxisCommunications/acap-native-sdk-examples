// Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.

#include "websocket_client.h"

#include <glib.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <systemd/sd-bus.h>

#include "utils.h"

#define API_VERSION "1"

// The IP that ACAP applications should use to connect to vapix APIs.
#define LOCALHOST_IP "127.0.0.12"

// The URL for the websocket connection to the PTZ WS API, with a placeholder for the session token.
#define URL_BASE "ws://" LOCALHOST_IP "/vapix/ws/v1?wssession=%s&apis=ptz-control-ws"

// Assume single channel for simplicity, in a real application this should be fetched using
// getCapabilities.
static const int channel = 1;

struct websocket_client {
    CURL* ws_curl_handle;
};

static size_t
write_to_string_callback(char* ptr, size_t size, size_t number_of_elements, void* user_data) {
    // Do not make this function a lambda in C++ since curl_easy_setopt has the signature
    // `curl_easy_setopt(CURL *curl, CURLoption option, ...)`. This means that the third argument is
    // assumed to be exactly of the correct type (in this case a function pointer), and no implicit
    // conversions will take place. So if a lambda were passed to curl_easy_setopt, it would not be
    // implicitly converted to a function pointer, but rather its bytes would be reinterpreted as a
    // function pointer, leading to undefined behavior.
    const size_t processed_bytes = size * number_of_elements;
    char** response_body         = (char**)user_data;
    free(*response_body);
    *response_body = strndup(ptr, processed_bytes);
    return processed_bytes;
}

// Returns a non-owning pointer.
static char* get_credentials(void) {
    // Cache the credentials so that the ACAP application always uses the same credentials for all
    // requests.
    static char cached_credentials[256] = "";
    if (cached_credentials[0] != '\0') {
        return cached_credentials;
    }

    // Connect to system D-Bus and call GetCredentials on
    // com.axis.HTTPConf1.VAPIXServiceAccounts1.
    sd_bus* bus = NULL;
    int result  = sd_bus_open_system(&bus);
    if (result < 0) {
        panic("Failed to connect to system D-Bus: %s", strerror(-result));
    }

    sd_bus_error bus_error = SD_BUS_ERROR_NULL;
    sd_bus_message* reply  = NULL;
    result                 = sd_bus_call_method(bus,
                                "com.axis.HTTPConf1",
                                "/com/axis/HTTPConf1/VAPIXServiceAccounts1",
                                "com.axis.HTTPConf1.VAPIXServiceAccounts1",
                                "GetCredentials",
                                &bus_error,
                                &reply,
                                "s",
                                "testuser");
    if (result < 0) {
        panic("Failed to call GetCredentials on D-Bus: %s",
              bus_error.message != NULL ? bus_error.message : strerror(-result));
    }

    // Extract a string from the method return value.
    const char* credentials_raw = NULL;
    result                      = sd_bus_message_read(reply, "s", &credentials_raw);
    if (result < 0 || credentials_raw == NULL) {
        panic("GetCredentials returned invalid credentials");
    }

    g_strlcpy(cached_credentials, credentials_raw, sizeof(cached_credentials));
    sd_bus_error_free(&bus_error);
    sd_bus_message_unref(reply);
    sd_bus_unref(bus);
    return cached_credentials;
}

// Returns an owning pointer that must be freed by the caller.
static char* get_session_token(void) {
    // Create a CURL handle for the HTTP request to get the session token. We use a separate handle
    // for this since it's a different request with different options, and we want to keep the
    // websocket handle clean for its specific use.
    CURL* http_curl_handle = curl_easy_init();
    if (http_curl_handle == NULL) {
        return NULL;
    }

    // Set the URL for the HTTP request to the endpoint that provides the session token.
    curl_easy_setopt(http_curl_handle,
                     CURLOPT_URL,
                     "http://" LOCALHOST_IP "/axis-cgi/wssession.cgi");

    // Set up HTTP basic authentication with the credentials retrieved from D-Bus.
    const char* credentials = get_credentials();
    curl_easy_setopt(http_curl_handle, CURLOPT_USERPWD, credentials);
    curl_easy_setopt(http_curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);

    // Set up an error buffer to get more detailed error messages from CURL if the request fails.
    char curl_error_buffer[CURL_ERROR_SIZE] = "";
    curl_easy_setopt(http_curl_handle, CURLOPT_ERRORBUFFER, curl_error_buffer);

    // Fetch the response body into a string so that we can extract the session token from it.
    char* response_body = NULL;
    curl_easy_setopt(http_curl_handle, CURLOPT_WRITEFUNCTION, write_to_string_callback);
    curl_easy_setopt(http_curl_handle, CURLOPT_WRITEDATA, &response_body);

    // Perform the HTTP request to get the session token. The response body should contain the token
    // if the request is successful.
    const CURLcode http_result = curl_easy_perform(http_curl_handle);
    if (http_result != CURLE_OK) {
        panic("Failed to perform HTTP request to get session token: %s",
              curl_easy_strerror(http_result));
    }

    // Check the HTTP response code to ensure that the request was successful. We expect a 200 OK
    // response with the session token in the body.
    long response_code;
    curl_easy_getinfo(http_curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code != 200) {
        panic("Failed to get session token: HTTP response code %ld", response_code);
    }

    // The response body may contain a newline character at the end, so we remove it before
    // returning the token.
    for (int i = 0; response_body[i] != '\0'; i++) {
        if (response_body[i] == '\r' || response_body[i] == '\n') {
            response_body[i] = '\0';
            break;
        }
    }

    curl_easy_cleanup(http_curl_handle);
    return response_body;
}

websocket_client* websocket_client_create(void) {
    websocket_client* client = (websocket_client*)malloc(sizeof(websocket_client));

    client->ws_curl_handle = curl_easy_init();
    if (client->ws_curl_handle == NULL) {
        free(client);
        return NULL;
    }

    // Set up the CURL handle for websocket communication. The URL and other options would be set
    // here.
    char* session_token                     = get_session_token();
    char* url                               = g_strdup_printf(URL_BASE, session_token);
    char* credentials                       = get_credentials();
    char curl_error_buffer[CURL_ERROR_SIZE] = "";
    char* response_body                     = NULL;

    syslog(LOG_INFO, "Retrieved session token: %s", session_token);

    curl_easy_setopt(client->ws_curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(client->ws_curl_handle, CURLOPT_CONNECT_ONLY, 2L);
    curl_easy_setopt(client->ws_curl_handle, CURLOPT_USERPWD, credentials);
    curl_easy_setopt(client->ws_curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    curl_easy_setopt(client->ws_curl_handle, CURLOPT_ERRORBUFFER, curl_error_buffer);
    curl_easy_setopt(client->ws_curl_handle, CURLOPT_WRITEFUNCTION, write_to_string_callback);
    curl_easy_setopt(client->ws_curl_handle, CURLOPT_WRITEDATA, &response_body);

    // Perform the connection handshake to establish the websocket connection. We will send and
    // receive messages over this connection using curl_ws_send and curl_ws_recv, which do not
    // require a separate perform call, but we need to do this initial handshake first to establish
    // the connection.
    const CURLcode connect_result = curl_easy_perform(client->ws_curl_handle);

    free(session_token);
    free(response_body);
    g_free(url);

    if (connect_result != CURLE_OK) {
        const char* error_details =
            curl_error_buffer[0] != '\0' ? curl_error_buffer : curl_easy_strerror(connect_result);
        panic("Failed to connect to websocket: %s", error_details);
    }
    return client;
}

void websocket_client_destroy(websocket_client* client) {
    if (client != NULL) {
        size_t sent;
        curl_ws_send(client->ws_curl_handle, "", 0, &sent, 0, CURLWS_CLOSE);
        curl_easy_cleanup(client->ws_curl_handle);
        free(client);
    }
}

bool send_continuous_move_command(websocket_client* client,
                                  float pan_speed_deg,
                                  float tilt_speed_deg,
                                  float zoom_speed_normalized) {
    char* command = g_strdup_printf(
        "{\"jsonrpc\":\"2.0\",\"method\":\"ptz-control-ws_v%s:continuousMove\",\"params\":{"
        "\"channel\":%d,\"panSpeedDeg\":%f,\"tiltSpeedDeg\":%f,\"zoomSpeedNormalized\":%f}}",
        API_VERSION,
        channel,
        pan_speed_deg,
        tilt_speed_deg,
        zoom_speed_normalized);

    size_t sent;
    const CURLcode send_result =
        curl_ws_send(client->ws_curl_handle, command, strlen(command), &sent, 0, CURLWS_TEXT);
    g_free(command);
    return send_result == CURLE_OK;
}
