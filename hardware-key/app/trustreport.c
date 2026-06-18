/**
 * Copyright (C) 2021, Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fcgi_stdio.h"
#include <glib.h>
#include <openssl/cms.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>

#include "certset.h"

#define APPNAME          "trustreport"
#define SETNAME          "TrustReport"
#define FCGI_SOCKET_NAME "FCGI_SOCKET_NAME"
#define MAX_POST_SIZE    4096
#define SIGN_CMD         "cat /proc/cpuinfo"
#define SIGNED_FILENAME  "cpuinfo.pem"

/**
 * Sign the device syslog with a CMS SignedData structure and write the
 * DER-encoded result to the FastCGI response as a downloadable file.
 *
 * The signing certificate and its private key are read from the PEM file
 * returned by certset_get_cert_path().  The syslog content is embedded
 * inside the CMS structure (non-detached signature).
 */
static void
handle_sign_syslog(FCGX_Request *request)
{
    char *cert_path = NULL;
    X509 *cert = NULL;
    EVP_PKEY *pkey = NULL;
    BIO *data = NULL;
    BIO *out_bio = NULL;
    CMS_ContentInfo *cms = NULL;
    int ok = 0;

    if (certset_get_cert_path(SETNAME, &cert_path) != 0 || !cert_path) {
        syslog(LOG_ERR, "sign_syslog: failed to get cert path");
        FCGX_FPrintF(request->out, "Content-Type: text/html\n\n");
        FCGX_FPrintF(request->out,
                     "<html><body><p style=\"color:red\">"
                     "Error: no certificate configured. "
                     "Set a certificate alias first."
                     "</p></body></html>");
        return;
    }

    /* Use BIO-based reading to avoid the fcgi_stdio.h fopen/FILE* replacement
     * which would otherwise pass an FCGI_FILE* into OpenSSL and crash. */
    {
        BIO *pem_bio = BIO_new_file(cert_path, "r");
        if (!pem_bio) {
            syslog(LOG_ERR, "sign_syslog: cannot open %s", cert_path);
            goto cleanup_path;
        }
        cert = PEM_read_bio_X509(pem_bio, NULL, NULL, NULL);
        (void)BIO_reset(pem_bio);
        pkey = PEM_read_bio_PrivateKey(pem_bio, NULL, NULL, NULL);
        BIO_free(pem_bio);
    }

    if (!cert || !pkey) {
        syslog(LOG_ERR, "sign_syslog: failed to load cert or key from %s", cert_path);
        goto cleanup_path;
    }

    /* Collect syslog lines into a buffer.
     * We use BIO_new_mem_buf (read-only, re-readable) instead of BIO_s_mem
     * so that CMS_sign can read the content twice: once for the digest and
     * once to embed it in the CMS structure. */
    char *syslog_buf = NULL;
    size_t syslog_len = 0;
    {
        FILE *slog = popen(SIGN_CMD, "r");
        if (slog) {
            char tmp[4096];
            size_t n;
            while ((n = fread(tmp, 1, sizeof(tmp), slog)) > 0) {
                char *newbuf = realloc(syslog_buf, syslog_len + n);
                if (!newbuf)
                    break;
                syslog_buf = newbuf;
                memcpy(syslog_buf + syslog_len, tmp, n);
                syslog_len += n;
            }
            pclose(slog);
        }
    }

    data = BIO_new_mem_buf(syslog_buf ? syslog_buf : (void *)"", (int)syslog_len);
    if (!data) {
        free(syslog_buf);
        goto cleanup_path;
    }

    /* Build the CMS SignedData in three steps so we can pin the digest to
     * SHA-256 explicitly — CMS_sign alone cannot infer a default digest for
     * all key types (e.g. EC keys on this platform). */
    cms = CMS_sign(NULL, NULL, NULL, NULL,
                   CMS_PARTIAL | CMS_BINARY | CMS_NOSMIMECAP);
    if (!cms) {
        unsigned long ssl_err = ERR_get_error();
        syslog(LOG_ERR, "sign_syslog: CMS_sign (partial) failed: %s",
               ERR_reason_error_string(ssl_err));
        goto cleanup_path;
    }
    if (!CMS_add1_signer(cms, cert, pkey, EVP_sha256(), CMS_NOSMIMECAP)) {
        unsigned long ssl_err = ERR_get_error();
        syslog(LOG_ERR, "sign_syslog: CMS_add1_signer failed: %s",
               ERR_reason_error_string(ssl_err));
        goto cleanup_path;
    }
    if (!CMS_final(cms, data, NULL, CMS_BINARY | CMS_NOSMIMECAP)) {
        unsigned long ssl_err = ERR_get_error();
        syslog(LOG_ERR, "sign_syslog: CMS_final failed: %s",
               ERR_reason_error_string(ssl_err));
        goto cleanup_path;
    }

    free(syslog_buf);
    syslog_buf = NULL;

    /* Serialize as S/MIME so the file can be verified directly with
     * "openssl cms -verify -in syslog.pem" without any -inform flag */
    out_bio = BIO_new(BIO_s_mem());
    if (!out_bio || !SMIME_write_CMS(out_bio, cms, NULL,
                                     CMS_BINARY | CMS_NOSMIMECAP)) {
        syslog(LOG_ERR, "sign_syslog: S/MIME serialization failed");
        goto cleanup_path;
    }

    {
        char *der_data = NULL;
        long der_len = BIO_get_mem_data(out_bio, &der_data);

        FCGX_FPrintF(request->out,
                     "Content-Type: application/pkcs7-mime\r\n"
                     "Content-Disposition: attachment; filename=\"" SIGNED_FILENAME "\"\r\n"
                     "Content-Length: %ld\r\n\r\n",
                     der_len);
        FCGX_PutStr(der_data, (int)der_len, request->out);
        ok = 1;
    }

cleanup_path:
    if (!ok) {
        /* Only send an error page if we haven't already written headers */
        FCGX_FPrintF(request->out, "Content-Type: text/html\n\n");
        FCGX_FPrintF(request->out,
                     "<html><body><p style=\"color:red\">"
                     "Error: failed to produce signed syslog. "
                     "Check the application log for details."
                     "</p></body></html>");
    }
    CMS_ContentInfo_free(cms);
    BIO_free(out_bio);
    BIO_free(data);
    EVP_PKEY_free(pkey);
    X509_free(cert);
    g_free(cert_path);
}

/**
 * Decode a URL-encoded string in-place (x-www-form-urlencoded).
 * Converts '+' to space and '%XX' hex sequences to their characters.
 */
static void
url_decode(char *dst, const char *src, size_t max_len)
{
    size_t out = 0;
    while (*src && out < max_len - 1) {
        if (*src == '+') {
            dst[out++] = ' ';
            src++;
        } else if (*src == '%' && src[1] && src[2]) {
            char hex[3] = { src[1], src[2], '\0' };
            dst[out++] = (char)strtol(hex, NULL, 16);
            src += 3;
        } else {
            dst[out++] = *src++;
        }
    }
    dst[out] = '\0';
}

/**
 * Extract the value of a form field from a URL-encoded POST body.
 * Returns a newly allocated string (caller must free), or NULL if not found.
 */
static char *
parse_form_field(const char *body, const char *field_name)
{
    size_t name_len = strlen(field_name);
    const char *p = body;

    while (p && *p) {
        if (strncmp(p, field_name, name_len) == 0 && p[name_len] == '=') {
            const char *val_start = p + name_len + 1;
            const char *val_end = strchr(val_start, '&');
            size_t val_len = val_end ? (size_t)(val_end - val_start) : strlen(val_start);

            char *encoded = strndup(val_start, val_len);
            if (!encoded)
                return NULL;
            char *decoded = malloc(val_len + 1);
            if (!decoded) {
                free(encoded);
                return NULL;
            }
            url_decode(decoded, encoded, val_len + 1);
            free(encoded);
            return decoded;
        }
        p = strchr(p, '&');
        if (p)
            p++;
    }
    return NULL;
}

/**
 * brief Initialize fastcgi and request handling.
 *
 * Set up fastcgi and define how an HTTP request should be handled.
 *
 * return EXIT_FAILURE if errors occur, otherwise EXIT_SUCCESS.
 */
static int
fcgi_run(void)
{
    int sock;
    FCGX_Request request;
    char *socket_path = NULL;
    int status;

    socket_path = getenv(FCGI_SOCKET_NAME);

    if (!socket_path) {
        syslog(LOG_ERR, "Failed to get environment variable FCGI_SOCKET_NAME");
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "FastCGI Socket: %s", socket_path);

    status = FCGX_Init();
    if (status != 0) {
        syslog(LOG_ERR, "FCGX_Init failed");
        return status;
    }

    sock = FCGX_OpenSocket(socket_path, 5);
    if (sock < 0) {
        syslog(LOG_ERR, "FCGX_OpenSocket failed");
        return EXIT_FAILURE;
    }

    chmod(socket_path, S_IRWXU | S_IRWXG | S_IRWXO);

    status = FCGX_InitRequest(&request, sock, 0);
    if (status != 0) {
        syslog(LOG_ERR, "FCGX_InitRequest failed");
        return status;
    }

    syslog(LOG_INFO, "FastCGI server starting loop");

    while (FCGX_Accept_r(&request) == 0) {
        syslog(LOG_INFO, "Received FastCGI request");

        const char *method = FCGX_GetParam("REQUEST_METHOD", request.envp);
        const char *query  = FCGX_GetParam("QUERY_STRING",   request.envp);

        /* GET ?action=sign_syslog → produce a signed CMS download */
        if (method && strcmp(method, "GET") == 0 &&
            query  && strstr(query, "action=sign_syslog")) {
            handle_sign_syslog(&request);
            FCGX_Finish_r(&request);
            continue;
        }

        const char *status_msg = NULL;
        int update_ok = 0;

        if (method && strcmp(method, "POST") == 0) {
            const char *content_length_str =
                    FCGX_GetParam("CONTENT_LENGTH", request.envp);
            long content_length = content_length_str ? atol(content_length_str) : 0;

            if (content_length > 0 && content_length < MAX_POST_SIZE) {
                char body[MAX_POST_SIZE];
                int bytes_read = FCGX_GetStr(body, (int)content_length, request.in);
                body[bytes_read] = '\0';

                char *cert_alias = parse_form_field(body, "cert_alias");
                if (cert_alias && cert_alias[0] != '\0') {
                    syslog(LOG_INFO, "Updating certset with alias: %s", cert_alias);
                    const char *empty_ca_aliases[] = { NULL };
                    int ret = certset_update_set(SETNAME, cert_alias, empty_ca_aliases);
                    if (ret == 0) {
                        status_msg = "Certset updated successfully.";
                        update_ok = 1;
                        syslog(LOG_INFO, "Certset updated successfully");
                    } else {
                        status_msg = "Failed to update certset. Check the alias and try again.";
                        syslog(LOG_ERR, "certset_update_set failed: %d", ret);
                    }
                } else {
                    status_msg = "Certificate alias must not be empty.";
                }
                free(cert_alias);
            } else {
                status_msg = "Invalid request body.";
            }
        }

        // Write the HTTP header
        FCGX_FPrintF(request.out, "Content-Type: text/html\n\n");

        // Write the HTML body
        FCGX_FPrintF(request.out, "<html><body>");
        FCGX_FPrintF(request.out,
                     "<h1>%s - Certificate Set Update</h1>",
                     APPNAME);

        if (status_msg) {
            FCGX_FPrintF(request.out,
                         "<p style=\"color:%s\">%s</p>",
                         update_ok ? "green" : "red",
                         status_msg);
        }

        FCGX_FPrintF(request.out,
                     "<form method=\"POST\">"
                     "<label for=\"cert_alias\">Certificate Alias:</label><br>"
                     "<textarea id=\"cert_alias\" name=\"cert_alias\""
                     " rows=\"4\" cols=\"50\"></textarea><br><br>"
                     "<input type=\"submit\" value=\"Update\">"
                     "</form>"
                     "<hr>"
                     "<a href=\"?action=sign_syslog\">"
                     "<button type=\"button\">Download Signed CPU Info (CMS)</button>"
                     "</a>");

        FCGX_FPrintF(request.out, "</body></html>");

        FCGX_Finish_r(&request);
    }

    return EXIT_SUCCESS;
}

int
main(void)
{
    int ret;
    openlog(APPNAME, LOG_PID | LOG_CONS, LOG_USER);

    ret = certset_create_empty_set(SETNAME);
    if (ret != 0) {
        syslog(LOG_ERR, "Failed to create empty certset: %d", ret);
        closelog();
        return EXIT_FAILURE;
    }

    ret = fcgi_run();
    closelog();
    return ret;
}
