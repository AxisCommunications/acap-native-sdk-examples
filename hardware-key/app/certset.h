#ifndef CERTSET_H
#define CERTSET_H

typedef enum {
    CERTSET1_INTERNAL_ERROR,
    CERTSET1_INVALID_ARGUMENT,
    CERTSET1_ACCESS_DENIED,
    CERTSET1_NOT_SUPPORTED,
    CERTSET1_SET_ALREADY_EXISTS,
    CERTSET1_NO_SUCH_SET,
    CERTSET1_NO_SUCH_CERTIFICATE,
    CERTSET1_TOO_MANY_SETS
} CertSet1Error;

int certset_create_empty_set(const char *certset_name);
int certset_update_set(const char *certset_name, const char * cert_alias, const char **ca_aliases);
int certset_get_cert_path(const char *certset_name, char **path);

#endif // CERTSET_H
