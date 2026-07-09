// Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.

#pragma once

__attribute__((noreturn)) __attribute__((format(printf, 1, 2))) void panic(const char* format, ...);
