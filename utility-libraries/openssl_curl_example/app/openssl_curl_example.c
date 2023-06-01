/**
 * Copyright (C) 2022, Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

/**
 * brief This example illustrates how to use custom openssl and curl libraries
 * in an ACAP application to transfer a file
 *
 * This example shows how custom curl and openssl libraries can be compiled and
 * used in an ACAP application to retrieve a public HTML web page and store it
 * locally, using a certificate of PEM format.
 */

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <curl.h>
#include <stdio.h>
#include <syslog.h>

/***** Struct declarations ****************************************************/

struct File {
  const char *filename;
  FILE *stream;
};

/***** Callback functions *****************************************************/

/**
 * brief Callback function called when there is data to be written to file
 *
 * param buffer − The pointer to the array of elements to be written.
 * param size   − The size in bytes of each element to be written.
 * param nmemb  − The number of elements, each one with a size of size bytes.
 * param stream − The pointer to a FILE object that specifies an output stream.
 */
static size_t write_to_file(void *buffer, size_t size, size_t nmemb, void *stream)
{
  struct File *out = (struct File *)stream;
  if(!out->stream) {
    out->stream = fopen(out->filename, "wb");
    if(!out->stream) {
      syslog(LOG_INFO,"*** Could not open file to write data in callback ***");
      return -1;
    }
  }
  return fwrite(buffer, size, nmemb, out->stream);
}

/**
 * brief SSL context callback for OpenSSL in curl
 *
 * param curl   − The curl handle managing the callback
 * param sslctx − A pointer to a OpenSSL context.
 * param parm   − The pointer set by CURLOPT_SSL_CTX_DATA to pass a certificate
 *                in memory to the callback. Is not used here.
 */
static CURLcode sslctx_function(CURL *curl, void *sslctx, void *parm)
{
  FILE *fp;
  CURLcode rv = CURLE_ABORTED_BY_CALLBACK;
  X509_STORE *store;
  STACK_OF(X509_INFO) *inf;
  int i;
  (void)curl;
  (void)parm;

  // Get a pointer to the X509 certificate store
  store = SSL_CTX_get_cert_store((SSL_CTX *)sslctx);
  if(!store) {
    return rv;
  }

  // Open PEM formatted certificate file
  fp = fopen ("/usr/local/packages/openssl_curl_example/cacert.pem", "r");
  if (fp == NULL) {
    syslog(LOG_INFO,"*** Open CA-cert file failed ***");
    return rv;
  }

  // Read the certificate in to a X509_INFO object
  inf = PEM_X509_INFO_read(fp, NULL, NULL, NULL);
  if(!inf) {
    fclose(fp);
    return rv;
  }

  // Add the certificate and CRL to the store
  for(i = 0; i < sk_X509_INFO_num(inf); i++) {
    X509_INFO *itmp = sk_X509_INFO_value(inf, i);
    if(itmp->x509) {
      X509_STORE_add_cert(store, itmp->x509);
    }
    if(itmp->crl) {
      X509_STORE_add_crl(store, itmp->crl);
    }
  }

  // Cleanup
  sk_X509_INFO_pop_free(inf, X509_INFO_free);
  fclose(fp);

  rv = CURLE_OK;
  return rv;
}

/***** Main *******************************************************************/

int main(void)
{
  CURL *curl;
  CURLcode rv;
  struct File fetch_file = {
    // Path to the file which will store the fetched web content
    "/usr/local/packages/openssl_curl_example/localdata/www.example.com.txt",
    NULL
  };

  // Start logging
  openlog(NULL, LOG_PID, LOG_USER);

  // Log the curl and openssl library versions used in the code
  curl_version_info_data *ver = curl_version_info(CURLVERSION_NOW);
  syslog(LOG_INFO, "ACAP application curl version: %u.%u.%u\n",
      (ver->version_num >> 16) & 0xff,
      (ver->version_num >> 8) & 0xff,
      ver->version_num & 0xff);
  syslog(LOG_INFO, "ACAP application openssl version: %s",
      OpenSSL_version(OPENSSL_VERSION));

  // This function sets up the program environment that libcurl needs
  curl_global_init(CURL_GLOBAL_DEFAULT);

  /**
   * This function must be the first function to be called, and it returns a
   * CURL easy handle that you must use as input to other functions in the easy
   * interface. This call MUST have a corresponding call to curl_easy_cleanup
   * when the operation is complete.
   * The easy interface will make single synchronous transfers.
   */
  curl = curl_easy_init();

  if (curl) {
    syslog(LOG_INFO, "curl easy init successful - handle has been created");

    // Set web page to connect to
    curl_easy_setopt(curl, CURLOPT_URL, "https://www.example.com/");

    // Add more logging if debug is set
#ifdef APP_DEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
#endif

    // Set proxy if it's defined
#ifdef APP_PROXY
    curl_easy_setopt(curl, CURLOPT_PROXY, APP_PROXY);
#endif

    // Keep example simple - skip all signal handling
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    // Define callback to get called when there is data to be written to file
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fetch_file);

    // Set strict certificate check and certificate type
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");

    /**
     * Turn off the default CA locations, otherwise libcurl will load CA
     * certificates from the locations that were detected at build-time
     */
    curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);
    curl_easy_setopt(curl, CURLOPT_CAPATH, NULL);

    /**
     * Force a new connection to be used.
     * This option seriously impacts performance of multiple transfers and
     * should normally not be used. Persistent connections are desired for
     * performance and the normal use case is to set up a safe connection and
     * then re-use it for multiple transfers.
     *
     * This option is used here only to show the difference between
     * transferring with and without certificate.
     *
     * In the case of transferring with a certificate, a SSL CTX callback is
     * used and that will only be called before an SSL connection is
     * established, therefore it will not affect existing verified SSL
     * connections already in the connection cache associated with this handle.
     * Normally you would set the SSL CTX function before making any transfers,
     * and not use this option.
     */
    curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1L);

    /**
     * First try: Retrieve page without ca certificates -> should fail
     * Unless libcurl was built --with-ca-fallback enabled at build-time
     */
    syslog(LOG_INFO, "*** 1. Transfer requested without certificate ***");
    rv = curl_easy_perform(curl);
    if(rv == CURLE_OK)
      syslog(LOG_INFO,
          "*** 1. Transfer Passed: Unexpected result, transfer without certificate should not pass ***");
    else
      syslog(LOG_INFO,
          "*** 1. Transfer Failed: Expected result, transfer without certificate should fail ***");

    /**
     * Second try: Retrieve page using cacerts' certificate -> should succeed
     * Load the certificate by installing a function doing the necessary
     * "modifications" to the SSL CONTEXT just before link init
     */
    curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, sslctx_function);

    syslog(LOG_INFO,"*** 2. Transfer requested with CA-cert ***");
    rv = curl_easy_perform(curl);
    if(rv == CURLE_OK)
      syslog(LOG_INFO, "*** 2. Transfer Succeeded: Expected result, transfer with CA-cert should pass ***");
    else
      syslog(LOG_INFO, "*** 2. Transfer Failed: Unexpected result, error code: %d ***", rv);

    // Close file after transfer and cleanup curl easy handle
    if(fetch_file.stream)
      fclose(fetch_file.stream);
    curl_easy_cleanup(curl);
  }
  // Cleanup of curl global environment
  curl_global_cleanup();
  return 0;
}
