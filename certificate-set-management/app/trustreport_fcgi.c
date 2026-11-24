// Copyright (C) 2026 Axis Communications AB, Lund, Sweden
// Licensed under the MIT License. See LICENSE file for details.

#include <fcgi_stdio.h>
#include <glib.h>
#include <limits.h>
#include <openssl/cms.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#include "certset.h"
#include "trustreport_fcgi.h"
#include "trustreport_templates.h"

#define APPNAME               "trustreport"
#define SETNAME               "TrustReport"
#define FCGI_SOCKET_NAME      "FCGI_SOCKET_NAME"
#define MAX_POST_SIZE         4096
#define FILE_SRC              "/proc/cpuinfo"
#define SIGNED_FILENAME       "cpuinfo.pem"
#define ACTION_CREATE_CERTSET "create_certificate_set"
#define ACTION_UPDATE_CERTSET "update_certificate_set"
#define ACTION_DELETE_CERTSET "delete_certificate_set"
#define ACTION_SIGN_CPUINFO   "sign_cpuinfo"

/* =========================================================================
 * Forward declarations for helpers implemented later in the file.
 * ========================================================================= */
static char* parse_form_field(const char* body, const char* field_name);
static void render_current_certset_summary(FCGX_Request* request);
static void render_certset_name_options(FCGX_Stream* out, const char* selected_name);
static void render_main_page(FCGX_Request* request, const char* status_msg, int operation_ok);

__attribute__((noreturn)) __attribute__((format(__printf__, 1, 2))) static void
panic(const char* format, ...) {
    va_list arg;

    va_start(arg, format);
    vsyslog(LOG_ERR, format, arg);
    va_end(arg);
    exit(1);
}

/* =========================================================================
 * Section: OpenSSL signing and CPU info packaging
 * ========================================================================= */

/**
 * Load signing certificate and private key for a given certificate set.
 * Returns 1 on success, 0 when no certificate is configured, and -1 on
 * loading/parsing errors.
 */
static int load_signing_credentials(const char* certset_name,
                                    char** cert_path_out,
                                    X509** cert_out,
                                    EVP_PKEY** pkey_out) {
    char* cert_path = NULL;
    X509* cert      = NULL;
    EVP_PKEY* pkey  = NULL;

    *cert_path_out = NULL;
    *cert_out      = NULL;
    *pkey_out      = NULL;

    if (certset_get_cert_path(certset_name, &cert_path) != 0 || !cert_path)
        return 0;

    /* Use BIO-based reading to avoid the fcgi_stdio.h fopen/FILE* replacement
     * which would otherwise pass an FCGI_FILE* into OpenSSL and crash. */
    BIO* pem_bio = BIO_new_file(cert_path, "r");
    if (!pem_bio) {
        syslog(LOG_ERR, "sign_cpuinfo: cannot open %s", cert_path);
        free(cert_path);
        return -1;
    }

    cert = PEM_read_bio_X509(pem_bio, NULL, NULL, NULL);
    (void)BIO_reset(pem_bio);
    pkey = PEM_read_bio_PrivateKey(pem_bio, NULL, NULL, NULL);
    BIO_free(pem_bio);

    if (!cert || !pkey) {
        syslog(LOG_ERR, "sign_cpuinfo: failed to load cert or key from %s", cert_path);
        EVP_PKEY_free(pkey);
        X509_free(cert);
        free(cert_path);
        return -1;
    }

    *cert_path_out = cert_path;
    *cert_out      = cert;
    *pkey_out      = pkey;
    return 1;
}

/**
 * Read /proc/cpuinfo into a heap buffer.
 */
static int load_cpuinfo(char** cpuinfo_buf_out, size_t* cpuinfo_len_out) {
    FILE* slog         = fopen(FILE_SRC, "r");
    char* cpuinfo      = NULL;
    size_t cpuinfo_len = 0;

    *cpuinfo_buf_out = NULL;
    *cpuinfo_len_out = 0;

    if (!slog) {
        syslog(LOG_ERR, "sign_cpuinfo: cannot open %s", FILE_SRC);
        return 0;
    }

    {
        char tmp[4096];
        size_t n;
        while ((n = fread(tmp, 1, sizeof(tmp), slog)) > 0) {
            char* newbuf = realloc(cpuinfo, cpuinfo_len + n);
            if (!newbuf) {
                free(cpuinfo);
                fclose(slog);
                syslog(LOG_ERR, "sign_cpuinfo: out of memory while reading %s", FILE_SRC);
                return 0;
            }
            cpuinfo = newbuf;
            memcpy(cpuinfo + cpuinfo_len, tmp, n);
            cpuinfo_len += n;
        }
    }

    if (ferror(slog)) {
        free(cpuinfo);
        fclose(slog);
        syslog(LOG_ERR, "sign_cpuinfo: read error while reading %s", FILE_SRC);
        return 0;
    }

    fclose(slog);
    *cpuinfo_buf_out = cpuinfo;
    *cpuinfo_len_out = cpuinfo_len;
    return 1;
}

/**
 * Build a SignedData CMS object from cpuinfo using cert+private key.
 */
static int sign_cpuinfo(X509* cert,
                        EVP_PKEY* pkey,
                        const char* cpuinfo_buf,
                        size_t cpuinfo_len,
                        CMS_ContentInfo** cms_out) {
    BIO* data            = NULL;
    CMS_ContentInfo* cms = NULL;

    *cms_out = NULL;

    if (cpuinfo_len > (size_t)INT_MAX) {
        syslog(LOG_ERR, "sign_cpuinfo: cpuinfo data too large");
        return 0;
    }

    /* Use BIO_new_mem_buf (read-only, re-readable) so CMS can consume
     * content for both digest generation and payload embedding. */
    data = BIO_new_mem_buf(cpuinfo_buf ? (void*)cpuinfo_buf : (void*)"", (int)cpuinfo_len);
    if (!data) {
        return 0;
    }

    /* Build the CMS SignedData in three steps so we can pin the digest to
     * SHA-256 explicitly — CMS_sign alone cannot infer a default digest for
     * all key types (e.g. EC keys on this platform). */
    cms = CMS_sign(NULL, NULL, NULL, NULL, CMS_PARTIAL | CMS_BINARY | CMS_NOSMIMECAP);
    if (!cms) {
        unsigned long ssl_err = ERR_get_error();
        syslog(LOG_ERR,
               "sign_cpuinfo: CMS_sign (partial) failed: %s",
               ERR_reason_error_string(ssl_err));
        BIO_free(data);
        return 0;
    }
    if (!CMS_add1_signer(cms, cert, pkey, EVP_sha256(), CMS_NOSMIMECAP)) {
        unsigned long ssl_err = ERR_get_error();
        syslog(LOG_ERR,
               "sign_cpuinfo: CMS_add1_signer failed: %s",
               ERR_reason_error_string(ssl_err));
        CMS_ContentInfo_free(cms);
        BIO_free(data);
        return 0;
    }
    if (!CMS_final(cms, data, NULL, CMS_BINARY | CMS_NOSMIMECAP)) {
        unsigned long ssl_err = ERR_get_error();
        syslog(LOG_ERR, "sign_cpuinfo: CMS_final failed: %s", ERR_reason_error_string(ssl_err));
        CMS_ContentInfo_free(cms);
        BIO_free(data);
        return 0;
    }

    BIO_free(data);
    *cms_out = cms;
    return 1;
}

/**
 * Sign the device CPU info with a CMS SignedData structure and write the
 * S/MIME-encoded result to the FastCGI response as a downloadable file.
 */
static void handle_sign_cpuinfo(FCGX_Request* request, const char* certset_name) {
    const char* set_for_signing =
        (certset_name && certset_name[0] != '\0') ? certset_name : SETNAME;
    char* cert_path      = NULL;
    X509* cert           = NULL;
    EVP_PKEY* pkey       = NULL;
    BIO* out_bio         = NULL;
    CMS_ContentInfo* cms = NULL;
    char* cpuinfo_buf    = NULL;
    size_t cpuinfo_len   = 0;
    int ok               = 0;

    int cert_status = load_signing_credentials(set_for_signing, &cert_path, &cert, &pkey);
    if (cert_status == 0) {
        syslog(LOG_ERR, "sign_cpuinfo: failed to get cert path for certset '%s'", set_for_signing);
        FCGX_FPrintF(request->out, "Content-Type: text/html\n\n");
        FCGX_FPrintF(request->out,
                     "<html><body><p style=\"color:red\">"
                     "Error: no certificate configured. "
                     "Select a certificate set with a certificate "
                     "alias first."
                     "</p></body></html>");
        return;
    }
    if (cert_status < 0)
        goto cleanup_path;

    if (!load_cpuinfo(&cpuinfo_buf, &cpuinfo_len))
        goto cleanup_path;

    if (!sign_cpuinfo(cert, pkey, cpuinfo_buf, cpuinfo_len, &cms))
        goto cleanup_path;

    /* Serialize as S/MIME so the file can be verified directly with
     * "openssl cms -verify -in cpuinfo.pem" without any -inform flag */
    out_bio = BIO_new(BIO_s_mem());
    if (!out_bio || !SMIME_write_CMS(out_bio, cms, NULL, CMS_BINARY | CMS_NOSMIMECAP)) {
        syslog(LOG_ERR, "sign_cpuinfo: S/MIME serialization failed");
        goto cleanup_path;
    }

    {
        char* der_data = NULL;
        long der_len   = BIO_get_mem_data(out_bio, &der_data);

        FCGX_FPrintF(request->out,
                     "Content-Type: application/pkcs7-mime\r\n"
                     "Content-Disposition: attachment; filename=\"" SIGNED_FILENAME
                     "\"\r\n"
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
                     "Error: failed to produce signed CPU info. "
                     "Check the application log for details."
                     "</p></body></html>");
    }
    CMS_ContentInfo_free(cms);
    BIO_free(out_bio);
    EVP_PKEY_free(pkey);
    X509_free(cert);
    free(cert_path);
    free(cpuinfo_buf);
}

/* =========================================================================
 * Section: Certificate set handling
 * ========================================================================= */

/**
 * Read the POST body into a caller-provided buffer.
 */
static int read_post_body(FCGX_Request* request, char* body, size_t body_len) {
    const char* content_length_str = FCGX_GetParam("CONTENT_LENGTH", request->envp);
    long content_length            = content_length_str ? atol(content_length_str) : 0;

    if (content_length <= 0 || content_length >= (long)body_len)
        return 0;

    int bytes_read = FCGX_GetStr(body, (int)content_length, request->in);
    if (bytes_read != content_length)
        return 0;

    body[bytes_read] = '\0';
    return 1;
}

/**
 * Update the certificate alias from a POST body.
 */
static void update_certificate_set(const char* body, const char** status_msg, int* operation_ok) {
    char* certset_name = parse_form_field(body, "certset_name");
    char* cert_alias   = parse_form_field(body, "cert_alias");
    if (certset_name && certset_name[0] != '\0' && cert_alias && cert_alias[0] != '\0') {
        syslog(LOG_INFO, "Updating certset '%s' with alias: %s", certset_name, cert_alias);
        const char* empty_ca_aliases[] = {NULL};
        int ret = certset_update_set(certset_name, cert_alias, empty_ca_aliases);
        if (ret == 0) {
            *status_msg   = "Certificate set updated successfully.";
            *operation_ok = 1;
            syslog(LOG_INFO, "Certset updated successfully");
        } else {
            *status_msg = "Failed to update certificate set. Check the alias and try again.";
            syslog(LOG_ERR, "certset_update_set failed: %d", ret);
        }
    } else if (!certset_name || certset_name[0] == '\0') {
        *status_msg = "Certificate set name must not be empty.";
    } else {
        *status_msg = "Certificate alias must not be empty.";
    }
    free(certset_name);
    free(cert_alias);
}

/**
 * Create an empty certificate set.
 */
static void create_certificate_set(const char* body, const char** status_msg, int* operation_ok) {
    char* certset_name = parse_form_field(body, "certset_name");
    if (certset_name && certset_name[0] != '\0') {
        int ret = certset_create_empty_set(certset_name);
        if (ret == 0) {
            *status_msg   = "Certificate set created successfully.";
            *operation_ok = 1;
            syslog(LOG_INFO, "Certset created successfully: %s", certset_name);
        } else {
            *status_msg = "Failed to create certificate set.";
            syslog(LOG_ERR, "certset_create_empty_set failed: %d", ret);
        }
    } else {
        *status_msg = "Certificate set name must not be empty.";
    }
    free(certset_name);
}

/**
 * Delete a certificate set selected from a POST body.
 */
static void delete_certificate_set(const char* body, const char** status_msg, int* operation_ok) {
    char* certset_name = parse_form_field(body, "certset_name");
    if (certset_name && certset_name[0] != '\0') {
        int ret = certset_delete_set(certset_name);
        if (ret == 0) {
            *status_msg   = "Certificate set deleted successfully.";
            *operation_ok = 1;
            syslog(LOG_INFO, "Certset deleted successfully: %s", certset_name);
        } else {
            *status_msg = "Failed to delete certificate set.";
            syslog(LOG_ERR, "certset_delete_set failed: %d", ret);
        }
    } else {
        *status_msg = "Certificate set name must not be empty.";
    }
    free(certset_name);
}

/* =========================================================================
 * Section: FastCGI request loop
 * ========================================================================= */

static void run_request_loop(FCGX_Request* request) {
    while (FCGX_Accept_r(request) == 0) {
        syslog(LOG_INFO, "Received FastCGI request");

        const char* method     = FCGX_GetParam("REQUEST_METHOD", request->envp);
        const char* query      = FCGX_GetParam("QUERY_STRING", request->envp);
        const char* status_msg = NULL;
        int operation_ok       = 0;

        if (method && strcmp(method, "GET") == 0 && query) {
            char* action = parse_form_field(query, "action");
            if (action && strcmp(action, ACTION_SIGN_CPUINFO) == 0) {
                char* certset_name = parse_form_field(query, "certset_name");
                free(action);
                handle_sign_cpuinfo(request, certset_name);
                free(certset_name);
                FCGX_Finish_r(request);
                continue;
            }
            free(action);
        }

        if (method && strcmp(method, "POST") == 0) {
            char post_body[MAX_POST_SIZE];
            char* action = NULL;

            if (read_post_body(request, post_body, sizeof(post_body))) {
                action = parse_form_field(post_body, "action");
                if (!action || strcmp(action, ACTION_UPDATE_CERTSET) == 0) {
                    update_certificate_set(post_body, &status_msg, &operation_ok);
                } else if (strcmp(action, ACTION_CREATE_CERTSET) == 0) {
                    create_certificate_set(post_body, &status_msg, &operation_ok);
                } else if (strcmp(action, ACTION_DELETE_CERTSET) == 0) {
                    delete_certificate_set(post_body, &status_msg, &operation_ok);
                } else {
                    status_msg = "Unsupported action.";
                }
            } else {
                status_msg = "Invalid request body.";
            }
            free(action);
        }

        render_main_page(request, status_msg, operation_ok);

        FCGX_Finish_r(request);
    }
}

/**
 * Initialize fastcgi and request handling.
 *
 * Set up fastcgi and define how an HTTP request should be handled.
 *
 * return EXIT_FAILURE if errors occur, otherwise EXIT_SUCCESS.
 */
int trustreport_fcgi_run(void) {
    int sock;
    FCGX_Request request;
    char* socket_path = NULL;
    int status;

    socket_path = getenv(FCGI_SOCKET_NAME);

    if (!socket_path) {
        panic("Failed to get environment variable FCGI_SOCKET_NAME");
    }

    syslog(LOG_INFO, "FastCGI Socket: %s", socket_path);

    status = FCGX_Init();
    if (status != 0) {
        panic("FCGX_Init failed: %d", status);
    }

    sock = FCGX_OpenSocket(socket_path, 5);
    if (sock < 0) {
        panic("FCGX_OpenSocket failed for %s", socket_path);
    }

    if (chmod(socket_path, S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
        panic("Failed to set permissions on FastCGI socket %s", socket_path);
    }

    status = FCGX_InitRequest(&request, sock, 0);
    if (status != 0) {
        panic("FCGX_InitRequest failed: %d", status);
    }

    syslog(LOG_INFO, "FastCGI server starting loop");

    run_request_loop(&request);

    return EXIT_SUCCESS;
}

/* =========================================================================
 * Section: HTML rendering and form helpers
 * ========================================================================= */

static void render_main_page(FCGX_Request* request, const char* status_msg, int operation_ok) {
    // Write the HTTP header
    FCGX_FPrintF(request->out, "Content-Type: text/html\n\n");

    // Write the HTML body
    FCGX_FPrintF(request->out, "%s", TRUSTREPORT_HTML_DOC_START);
    render_current_certset_summary(request);
    FCGX_FPrintF(request->out, TRUSTREPORT_HTML_HEADING_FMT, APPNAME);

    if (status_msg) {
        FCGX_FPrintF(request->out,
                     TRUSTREPORT_HTML_STATUS_FMT,
                     operation_ok ? "green" : "red",
                     status_msg);
    }

    FCGX_FPrintF(request->out,
                 TRUSTREPORT_HTML_CREATE_UPDATE_PREFIX_FMT,
                 ACTION_CREATE_CERTSET,
                 ACTION_UPDATE_CERTSET);
    render_certset_name_options(request->out, SETNAME);
    FCGX_FPrintF(request->out, TRUSTREPORT_HTML_UPDATE_DOWNLOAD_PREFIX_FMT, ACTION_SIGN_CPUINFO);
    render_certset_name_options(request->out, SETNAME);
    FCGX_FPrintF(request->out, TRUSTREPORT_HTML_DOWNLOAD_DELETE_PREFIX_FMT, ACTION_DELETE_CERTSET);
    render_certset_name_options(request->out, NULL);
    FCGX_FPrintF(request->out, "%s", TRUSTREPORT_HTML_DELETE_SUFFIX);
}

static void render_html_escaped_text(FCGX_Stream* out, const char* text) {
    char* escaped = g_markup_escape_text(text ? text : "", -1);
    FCGX_FPrintF(out, "%s", escaped);
    g_free(escaped);
}

static void render_alias_list(FCGX_Stream* out, char** aliases, size_t alias_count) {
    if (!aliases || alias_count == 0) {
        FCGX_FPrintF(out, "<em>None</em>");
        return;
    }

    for (size_t i = 0; i < alias_count; i++) {
        if (i > 0)
            FCGX_FPrintF(out, ", ");
        FCGX_FPrintF(out, "<code>");
        render_html_escaped_text(out, aliases[i]);
        FCGX_FPrintF(out, "</code>");
    }
}

static void render_certset_name_options(FCGX_Stream* out, const char* selected_name) {
    CertsetList certsets = {0};
    int ret              = certset_list(&certsets);

    if (ret != 0 || certsets.count == 0) {
        FCGX_FPrintF(out, "<option value=\"\">No certificate sets available</option>");
        certset_list_free(&certsets);
        return;
    }

    int rendered = 0;
    for (size_t i = 0; i < certsets.count; i++) {
        const char* set_name = certsets.entries[i].name;
        if (set_name) {
            FCGX_FPrintF(out, "<option value=\"");
            render_html_escaped_text(out, set_name);
            FCGX_FPrintF(out, "\"");
            if (selected_name && strcmp(selected_name, set_name) == 0)
                FCGX_FPrintF(out, " selected");
            FCGX_FPrintF(out, ">");
            render_html_escaped_text(out, set_name);
            FCGX_FPrintF(out, "</option>");
            rendered = 1;
        }
    }

    if (!rendered)
        FCGX_FPrintF(out, "<option value=\"\">No certificate sets available</option>");

    certset_list_free(&certsets);
}

static void render_certset_summary_items(FCGX_Stream* out, const CertsetList* certsets) {
    int rendered = 0;

    FCGX_FPrintF(out,
                 "<table style=\"border-collapse:collapse; width:100%%;\">"
                 "<caption style=\"caption-side:top; text-align:left; "
                 "font-weight:bold; padding-bottom:0.5rem;\">"
                 "Available certificate sets"
                 "</caption>"
                 "<tbody>");

    FCGX_FPrintF(out,
                 "<tr>"
                 "<th style=\"border:1px solid #000; padding:0.5rem; "
                 "text-align:left; background:#f2f2f2;\">Set Name</th>"
                 "<th style=\"border:1px solid #000; padding:0.5rem; "
                 "text-align:left; background:#f2f2f2;\">Coupled certificate</th>"
                 "<th style=\"border:1px solid #000; padding:0.5rem; "
                 "text-align:left; background:#f2f2f2;\">Included CAs</th>"
                 "</tr>");

    for (size_t i = 0; i < certsets->count; i++) {
        const CertsetEntry* entry = &certsets->entries[i];
        if (entry->name) {
            rendered = 1;
            FCGX_FPrintF(out, "<tr>");

            FCGX_FPrintF(out,
                         "<td style=\"border:1px solid #000; padding:0.5rem; "
                         "vertical-align:top;\">");
            render_html_escaped_text(out, entry->name);
            FCGX_FPrintF(out, "</td>");

            FCGX_FPrintF(out,
                         "<td style=\"border:1px solid #000; padding:0.5rem; "
                         "vertical-align:top;\">");
            render_alias_list(out, entry->cert_aliases, entry->cert_alias_count);
            FCGX_FPrintF(out, "</td>");

            FCGX_FPrintF(out,
                         "<td style=\"border:1px solid #000; padding:0.5rem; "
                         "vertical-align:top;\">");
            render_alias_list(out, entry->ca_aliases, entry->ca_alias_count);
            FCGX_FPrintF(out, "</td>");

            FCGX_FPrintF(out, "</tr>");
        }
    }

    if (!rendered) {
        FCGX_FPrintF(out,
                     "<tr><td colspan=\"3\" style=\"border:1px solid #000; "
                     "padding:0.5rem;\">"
                     "No certificate sets available"
                     "</td></tr>");
    }

    FCGX_FPrintF(out, "</tbody></table>");
}

static void render_current_certset_summary(FCGX_Request* request) {
    CertsetList certsets = {0};
    int ret              = certset_list(&certsets);
    size_t total_sets    = 0;

    FCGX_FPrintF(request->out, "<h2>Available certificate sets</h2>");
    if (ret != 0) {
        FCGX_FPrintF(request->out,
                     "<p style=\"color:red\">Could not list certificate sets "
                     "(certset_list returned %d).</p>",
                     ret);
        return;
    }

    total_sets = certsets.count;
    FCGX_FPrintF(request->out,
                 "<p><strong>Total sets visible:</strong> %lu</p>",
                 (unsigned long)total_sets);
    render_certset_summary_items(request->out, &certsets);

    certset_list_free(&certsets);
}

/**
 * Decode a URL-encoded string in-place (x-www-form-urlencoded).
 * Converts '+' to space and '%XX' hex sequences to their characters.
 */
static void url_decode(char* dst, const char* src, size_t max_len) {
    size_t out = 0;
    while (*src && out < max_len - 1) {
        if (*src == '+') {
            dst[out++] = ' ';
            src++;
        } else if (*src == '%' && src[1] && src[2]) {
            char hex[3] = {src[1], src[2], '\0'};
            dst[out++]  = (char)strtol(hex, NULL, 16);
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
static char* parse_form_field(const char* body, const char* field_name) {
    size_t name_len = strlen(field_name);
    const char* p   = body;

    while (p && *p) {
        if (strncmp(p, field_name, name_len) == 0 && p[name_len] == '=') {
            const char* val_start = p + name_len + 1;
            const char* val_end   = strchr(val_start, '&');
            size_t val_len        = val_end ? (size_t)(val_end - val_start) : strlen(val_start);

            char* encoded = strndup(val_start, val_len);
            if (!encoded)
                return NULL;
            char* decoded = malloc(val_len + 1);
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
