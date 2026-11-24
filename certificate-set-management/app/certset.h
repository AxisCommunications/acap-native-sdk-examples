// Copyright (C) 2026 Axis Communications AB, Lund, Sweden
// Licensed under the MIT License. See LICENSE file for details.

#ifndef CERTSET_H
#define CERTSET_H

#include <stddef.h>

typedef struct {
    char* name;
    char* path;
    char** cert_aliases;
    size_t cert_alias_count;
    char** ca_aliases;
    size_t ca_alias_count;
} CertsetEntry;

typedef struct {
    CertsetEntry* entries;
    size_t count;
} CertsetList;

int certset_create_empty_set(const char* certset_name);
int certset_delete_set(const char* certset_name);
int certset_update_set(const char* certset_name, const char* cert_alias, const char** ca_aliases);
int certset_get_cert_path(const char* certset_name, char** path);
// certsets must be zero-initialized before its first use and may be reused for subsequent calls.
int certset_list(CertsetList* certsets);
void certset_list_free(CertsetList* certsets);

#endif  // CERTSET_H
