// Copyright (C) 2026 Axis Communications AB, Lund, Sweden
// Licensed under the MIT License. See LICENSE file for details.

#include "trustreport_templates.h"

const char TRUSTREPORT_HTML_DOC_START[] = "<!DOCTYPE html><html><body>";

const char TRUSTREPORT_HTML_HEADING_FMT[] = "<hr><h1>%s - Certificate Set Operations</h1>";

const char TRUSTREPORT_HTML_STATUS_FMT[] = "<p style=\"color:%s\">%s</p>";

const char TRUSTREPORT_HTML_CREATE_UPDATE_PREFIX_FMT[] =
    "<div style=\"display:flex; gap:1rem; flex-wrap:wrap; align-items:stretch;\">"
    "<div style=\"flex:1 1 0; min-width:220px; border:1px solid #ccc; border-radius:6px; "
    "padding:1rem;\">"
    "<h2>Create certificate set</h2>"
    "<form method=\"POST\">"
    "<input type=\"hidden\" name=\"action\" value=\"%s\">"
    "<label for=\"certset_name\">Certificate Set Name:</label><br>"
    "<input id=\"certset_name\" name=\"certset_name\" type=\"text\">"
    "<br><br>"
    "<input type=\"submit\" value=\"Create\">"
    "</form>"
    "</div>"
    "<div style=\"flex:1 1 0; min-width:220px; border:1px solid #ccc; border-radius:6px; "
    "padding:1rem;\">"
    "<h2>Update certificate set</h2>"
    "<form method=\"POST\">"
    "<input type=\"hidden\" name=\"action\" value=\"%s\">"
    "<label for=\"certset_name_update\">Certificate Set:</label><br>"
    "<select id=\"certset_name_update\" name=\"certset_name\">";

const char TRUSTREPORT_HTML_UPDATE_DOWNLOAD_PREFIX_FMT[] =
    "</select><br><br>"
    "<label for=\"cert_alias\">Certificate Alias:</label><br>"
    "<textarea id=\"cert_alias\" name=\"cert_alias\" rows=\"4\" cols=\"50\"></textarea><br><br>"
    "<input type=\"submit\" value=\"Update\">"
    "</form>"
    "</div>"
    "<div style=\"flex:1 1 0; min-width:220px; border:1px solid #ccc; border-radius:6px; "
    "padding:1rem;\">"
    "<h2>Download cpuinfo</h2>"
    "<form method=\"GET\">"
    "<input type=\"hidden\" name=\"action\" value=\"%s\">"
    "<label for=\"certset_name_download\">Certificate Set:</label><br>"
    "<select id=\"certset_name_download\" name=\"certset_name\">";

const char TRUSTREPORT_HTML_DOWNLOAD_DELETE_PREFIX_FMT[] =
    "</select><br><br>"
    "<input type=\"submit\" value=\"Download CPU Info (CMS)\">"
    "</form>"
    "</div>"
    "<div style=\"flex:1 1 0; min-width:220px; border:1px solid #ccc; border-radius:6px; "
    "padding:1rem;\">"
    "<h2>Delete certificate set</h2>"
    "<form method=\"POST\">"
    "<input type=\"hidden\" name=\"action\" value=\"%s\">"
    "<label for=\"certset_name_delete\">Certificate Set:</label><br>"
    "<select id=\"certset_name_delete\" name=\"certset_name\">";

const char TRUSTREPORT_HTML_DELETE_SUFFIX[] =
    "</select><br><br>"
    "<input type=\"submit\" value=\"Delete\">"
    "</form>"
    "</div>"
    "</div>"
    "</body></html>";
