// Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.

#pragma once

#include <curl/curl.h>
#include <stdbool.h>

typedef struct websocket_client websocket_client;

websocket_client* websocket_client_create(void);

void websocket_client_destroy(websocket_client* client);

bool send_continuous_move_command(websocket_client* client,
                                  float pan_speed_deg,
                                  float tilt_speed_deg,
                                  float zoom_speed_normalized);
