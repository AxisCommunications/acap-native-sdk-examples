// Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.

#pragma once

#include <stdbool.h>

#include "websocket_client.h"

typedef struct subscriber subscriber;

subscriber* subscriber_create(websocket_client* ws_client);

void subscriber_destroy(subscriber* subscriber_instance);

bool subscribe_to_topic(subscriber* subscriber_instance, const char* topic_name);
