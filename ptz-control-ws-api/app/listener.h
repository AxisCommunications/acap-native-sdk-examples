// Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.

#pragma once

#include <datahub/client.h>

// Names of the topics to subscribe to.
#define OPTICAL_ZOOM_TOPIC_NAME "com.axis.ptz.optical-zoom-state.v1"
#define PAN_TILT_TOPIC_NAME     "com.axis.ptz.pan-tilt-state.v1"

// Method called by Device Data Hub when an event occurs.
void on_data(const DHTopicSample* sample, void* user_data);
