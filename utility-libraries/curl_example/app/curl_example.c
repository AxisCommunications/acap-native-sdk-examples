/**
 * Copyright (C) 2021, Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

/**
 * brief This example illustrates how to use external curl library in an
 * ACAP application
 *
 * This example shows how curl library can be used to fetch a file from the
 * URL and store the content locally.
 *
 */

#include <syslog.h> 
#include <curl.h>

/***** Struct declarations ***************************************************/
struct File { 
  const char *filename;
  FILE *stream;
};

/***** Callback function  ****************************************************/

/** 
 * brief This callback function is called when there is data to be written
 *
 * param buffer − This is the pointer to the array of elements to be written.
 * param size   − This is the size in bytes of each element to be written.
 * param nmemb  − This is the number of elements, each one with a size of size
 * bytes.  param stream − This is the pointer to a FILE object that specifies
 * an output stream.
 */
static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{ 
  struct File *out = (struct File *)stream; 
  if(!out->stream) {
    // open file for writing
    out->stream = fopen(out->filename, "wb");
    if(!out->stream)
      return -1; // failure, can't open file to write
  }
  return fwrite(buffer, size, nmemb, out->stream); 
}

/***** Main ******************************************************************/

/** 
 * brief Main function which access user defined link and fetch content to
 * application directory localdata.
 *
 */ 
int main(void)
{
  CURL *curl; 
  CURLcode res;
  struct File file = {
    // name and path to store the file	
    "/usr/local/packages/curl_example/localdata/jquery.min.js",
    NULL
  };

  openlog(NULL, LOG_PID, LOG_USER);

  // This function sets up the program environment that libcurl needs
  curl_global_init(CURL_GLOBAL_DEFAULT);

  /** 
   * This function must be the first function to be called, and it
   * returns a CURL easy handle that you must use as input to other
   * functions in the easy interface. This call MUST have a
   * corresponding call to curl_easy_cleanup when the operation is
   * complete.
   */ 
  curl = curl_easy_init();

  if(curl) {
    syslog(LOG_INFO,"CURL init succesful - Curl handle is created");

#ifdef CURL_PROXY
    //Set the proxy for upcoming request, if it is defined.
    curl_easy_setopt(curl, CURLOPT_PROXY, CURL_PROXY);
#endif
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

    // Temporary URL is given, later modify with more suitable URL  
    curl_easy_setopt(curl, CURLOPT_URL,
        "https://ajax.googleapis.com/ajax/libs/jquery/3.6.0/jquery.min.js");

    // Define our callback to get called when there is data to be written
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);

    // Set a pointer to our struct to pass to the callback
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);

    /**
     * Performs the entire request in a blocking manner and returns when done,
     * or if it failed. 
     */
    res = curl_easy_perform(curl);

    // always cleanup
    curl_easy_cleanup(curl);

    if(CURLE_OK != res) {
      // Copy failed
      syslog(LOG_ERR, "curl error number : %d\n", res); 
    } else {
      syslog(LOG_INFO, "curl content copy successfull\n"); 
    }

    if(file.stream)
      fclose(file.stream); /* close the local file */

    // Once for each call you make to curl_global_init
    curl_global_cleanup();

  }
  return 0;
}
