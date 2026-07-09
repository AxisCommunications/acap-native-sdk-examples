// Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.

#include "utils.h"

#include <stdarg.h>
#include <stdlib.h>
#include <syslog.h>

__attribute__((noreturn)) __attribute__((format(printf, 1, 2))) void panic(const char* format,
                                                                           ...) {
    va_list arg;
    va_start(arg, format);
    vsyslog(LOG_ERR, format, arg);
    va_end(arg);
    exit(1);
}
