// Copyright (C) 2026 Axis Communications AB, Lund, Sweden
// Licensed under the MIT License. See LICENSE file for details.

#include <syslog.h>

#include "trustreport_fcgi.h"

#define APPNAME "trustreport"

int main(void) {
    int ret;

    ret = trustreport_fcgi_run();
    if (ret != 0) {
        syslog(LOG_ERR, "FastCGI server return code: %d", ret);
    }

    return ret;
}
