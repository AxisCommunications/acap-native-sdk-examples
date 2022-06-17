*Copyright (C) 2022, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# Build custom OpenSSL and curl libraries and use them in an ACAP application

This example shows you how to build custom versions of
[OpenSSL](https://www.openssl.org/) and [curl](https://curl.se/) from
source by using the SDK, and how to bundle them for use in an ACAP application.

The application uses these libraries to transfer content from a web server
`www.example.com` with HTTPS and a CA certificate to the application directory
on a device.

## Table of contents

<!-- ToC GFM -->

* [Purpose of the example](#purpose-of-the-example)
* [OpenSSL and curl APIs](#openssl-and-curl-apis)
* [Getting started](#getting-started)
* [How to run the code](#how-to-run-the-code)
  * [Build the application](#build-the-application)
  * [Install your application](#install-your-application)
  * [The expected output](#the-expected-output)
    * [Transferred file](#transferred-file)
    * [Application log](#application-log)
* [Outline of build steps](#outline-of-build-steps)
  * [Runtime shared library search path](#runtime-shared-library-search-path)
* [Check build dependencies](#check-build-dependencies)
* [Troubleshooting](#troubleshooting)
  * [Error CURLE_PEER_FAILED_VERIFICATION (60)](#error-curle_peer_failed_verification-60)
* [License](#license)

<!-- /ToC -->

## Purpose of the example

* **Cross-compilation with SDK** - It's important that the cross-compiled
  libraries use the correct SDK architecture, for example, armv7hf or aarch64.
It's also important that they build with the same `libc` version of the SDK,
with which the application code is compiled. If the libraries are compiled with
a newer `libc` version than the one used in the SDK, and get dependencies to
the latest symbols and functions, the result is a compilation error when
building the ACAP application. That is because the older `libc` from the SDK
can't find these newer symbols and functions.
* **Don't depend on all available libraries in SDK** - It's recommended to only
  use `libc` and the documented [ACAP
API](https://axiscommunications.github.io/acap-documentation/docs/api/native-api.html)
libraries in the SDK. Don't use other libraries from the SDK in an ACAP
application.  Instead, download, compile and bundle libraries such as
`libcrypto.so` and `libssl.so` (OpenSSL) and `libcurl.so` (curl) with the ACAP
application.
* **Check the runtime dependencies** - Check that the runtime dependencies, and
  the location where the built application binary and libraries look for
dependencies are correct.
* **OpenSSL and curl** - Two common libraries where curl uses OpenSSL when
  being built. This example shows how to handle such a setup.

For more details on how this is implemented in the example, see [outline of
build steps](#outline-of-build-steps), or have a look at the
[Dockerfile](./Dockerfile) which has some helpful comments.

## OpenSSL and curl APIs

Documentation of API functions used in this example:

* https://www.openssl.org/docs/man1.1.1/man3/
* https://curl.se/libcurl/c

## Getting started

These instructions show you how to execute the code. Below is the structure and
the scripts used in the example.

```bash
openssl_curl_example
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── openssl_curl_example.c
├── Dockerfile
└── README.md
```

* **app/LICENSE** - File containing the license conditions.
* **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
* **app/manifest.json** - Defines the application and its configuration.
* **app/openssl_curl_example.c** - Application source code.
* **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
* **README.md** - Step by step instructions on how to run the example.

## How to run the code

The sections below give you step-by-step instructions on how to execute the
program, starting with the generation of the .eap file, and ending with running
the application on a device.

### Build the application

> **IMPORTANT**
> Depending on the network you are connected to, you may need to add proxy
> settings. The file that needs those settings is ~/.docker/config.json.  For
> reference go to https://docs.docker.com/network/proxy/ and a [script for
> Axis device
> here](../../FAQs.md#how-can-i-set-up-network-proxy-settings-on-the-axis-device).

Standing in your working directory run:

```bash
docker build --tag <APP_IMAGE> .
```

<APP_IMAGE> is the image tag name, for example, openssl_curl_example:1.0

The default architecture is **armv7hf**. To build for **aarch64** it's possible to
update the *ARCH* variable in the Dockerfile or to set it in the docker build
command via build argument:

```bash
docker build --build-arg ARCH=aarch64 --tag <APP_IMAGE> .
```

If the device is inside a network with a proxy, then it can be passed on as a
build argument:

```bash
docker build --build-arg APP_PROXY=<MY_PROXY> --tag <APP_IMAGE> .
```

> **IMPORTANT**
> Pass proxy argument without `http://`.

To get more verbose logging from curl, pass the following build argument:

```bash
docker build --build-arg APP_DEBUG=yes --tag <APP_IMAGE> .
```

Copy the result from the container image to a local directory build:

```bash
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

The working directory now contains a build folder with the following files:

```bash
├── build
│   ├── LICENSE
│   ├── Makefile
│   ├── cacert.pem
│   ├── lib
│   │   ├── libssl.so@
│   │   ├── libssl.so.1.1*
│   │   ├── libcrypto.so@
│   │   ├── libcrypto.so.1.1*
│   │   ├── libcurl.so@
│   │   ├── libcurl.so.4@
│   │   └── libcurl.so.4.8.0*
│   ├── manifest.json
│   ├── openssl_curl_example*
│   ├── openssl_curl_example.c
│   ├── openssl_curl_example_1_0_0_LICENSE.txt
│   ├── openssl_curl_example_1_0_0_armv7hf.eap
│   ├── package.conf
│   ├── package.conf.orig
│   └── param.conf
```

* **build/cacert.pem** - CA certificate file for remote server verification.
* **build/lib** - Folder containing compiled library files for openssl and libcurl.
* **build/openssl_curl_example** - Application executable binary file.
* **build/openssl_curl_example_1_0_0_LICENSE.txt** - Copy of LICENSE file.
* **build/openssl_curl_example_1_0_0_armv7hf.eap** - Application package .eap file.
* **build/package.conf** - Defines the application and its configuration.
* **build/package.conf.orig** - Defines the application and its configuration, original file.
* **build/param.conf** - File containing application parameters.

### Install your application

To install your application on an Axis device:

1. Go to `http://<AXIS_DEVICE_IP>` where <AXIS_DEVICE_IP> is the IP number of your
Axis device.
2. Go to **Apps** in the device GUI.
3. Click **+** (Add app) or (Add), and open the newly built **openssl_curl_example_1_0_0_armv7hf.eap**.
4. Click **Install**.
5. **Start** the application.

The `openssl_curl_example` application is now available on the device.

### The expected output

#### Transferred file

The application uses the bundled CA certificate `cacert.pem` to fetch the
HTTPS content of https://www.example.com and stores the content in
`/usr/local/packages/openssl_curl_example/localdata/www.example.com.txt` on the
device.

> **IMPORTANT**
> Make sure [SSH is
> enabled](../../FAQs.md#how-can-i-enable-ssh-on-an-axis-device) on the device
> to run the following commands.

Compare the web page source code to the content of file `www.example.com.txt`.

```bash
ssh root@<AXIS_DEVICE_IP>
cat /usr/local/packages/openssl_curl_example/localdata/www.example.com.txt

(HTML content of www.example.com)
```

#### Application log

You can also check the application log, which is found directly at:

```sh
http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=openssl_curl_example
```

or by clicking the "**App log**" link in the device's web interface, or by
extracting the logs using the following commands in the terminal.

> **IMPORTANT**
> Make sure [SSH is
> enabled](../../FAQs.md#how-can-i-enable-ssh-on-an-axis-device) on the device
> to run the following commands.

```sh
ssh root@<AXIS_DEVICE_IP>
head -50 /var/log/info.log
```

```sh
----- Contents of SYSTEM_LOG for 'openssl_curl_example' -----

17:17:50.665 [ INFO ] openssl_curl_example[1687]: ACAP application curl version: 7.83.1
17:17:50.665 [ INFO ] openssl_curl_example[1687]: ACAP application openssl version: OpenSSL 1.1.1m 14 Dec 2021
17:17:50.669 [ INFO ] openssl_curl_example[1687]: curl easy init successful - handle has been created
17:17:50.669 [ INFO ] openssl_curl_example[1687]: *** 1. Transfer requested without certificate ***
17:17:50.954 [ INFO ] openssl_curl_example[1687]: *** 1. Transfer Failed: Expected result, transfer without certificate should fail ***
17:17:50.954 [ INFO ] openssl_curl_example[1687]: *** 2. Transfer requested with CA-cert ***
17:17:51.290 [ INFO ] openssl_curl_example[1687]: *** 2. Transfer Succeeded: Expected result, transfer with CA-cert should pass ***
```

## Outline of build steps

1. **Prepare build environment** - We want to build all libraries with the
   `libc` version of the SDK and avoid using `libssl` or `libcrypto`, which are
also available in the SDK, by accident.  To achieve this, we recommend removing
the OpenSSL libraries in the SDK library path. You don't need to remove curl
since it's not included in the SDK.

   Why not remove these libraries from the SDK? The [Licensekey
API](https://axiscommunications.github.io/acap-documentation/docs/api/native-api.html#license-key-api)
has a dependency on `libcrypto`.

2. **Build OpenSSL libraries** - The SDK environment is sourced to compile with
   the correct `libc` version. The build is made in two steps, first
`libcrypto.so` and then `libssl.so`, which depends on `libcrypto.so`.

   The option `--prefix` is set to the SDK library path to get the built
library, header, and pkgconfig files to be installed here. This is to
facilitate the build of curl which needs to link to both `libc` in the SDK
library path and the OpenSSL libraries.

3. **Build curl library** - The SDK environment is sourced to compile
   `libcurl.so` with the `libc` version in the SDK, and the two OpenSSL
libraries from the previous step. Since the OpenSSL libraries have been
installed to the SDK library path, no additional library path is required for
the linker to find all necessary files.

4. **Copy libraries to the application** - Copy the shared object files (.so)
   that you want to be bundled in the ACAP application are copied to a
directory `lib` under the application directory.

5. **Download CA certificate** - To get access to the web server, download a
   CA certificate using the Ubuntu `openssl` command-line tool in the container
image.

6. **Build ACAP application** - In the application directory `app`, the
   Makefile is used for defining the application library directory with `-Llib`
and the built libraries with `-lcrypto -lssl -lcurl`. The [runtime shared
library search path](#runtime-shared-library-search-path) is set to find these
libraries in the runtime application directory.  The last part of the
Dockerfile sources the SDK environment and starts the build with the build tool
`acap-build`.

### Runtime shared library search path

One consideration to make for all build steps is to define the application
runtime directory with a runtime shared library search path using the `gcc`
option `-Wl,-rpath,<PATH>`, which is where the bundled libraries will be found
when the application is installed on a device.

We recommend always using this option when building common libraries like
`libcrypto.so`, `libssl.so`, and `libcurl.so`. If you don't, the application
still runs, but with the versions used in AXIS OS, which is not recommended.

For the ACAP application it's required to set the runtime shared library search
path in the Makefile to use the bundled libraries.

If you don't set the runtime shared library search path for OpenSSL and curl
libraries, the application may still run and seem to use the correct versions
of the libraries, see the version logged in the [application
log](#the-expected-output---application-log). But it might be hard to decide
which version of `libcrypto.so` that is used since this library is also
available in AXIS OS.

By [checking the linking](#check-build-dependencies) of the libraries, you can
notice that in building without the runtime path, the libraries will link to
the version in AXIS OS. This would perhaps be more important if the libraries
were to be used as standalone instead of the ACAP application, but in this
example we make sure that both OpenSSL and curl libraries are built with a
runtime path pointing to the ACAP application library directory.

## Check build dependencies

There are different ways of checking the build dependencies of the application
binary and the built libraries (.so-files). If you're running the built
application image interactively, use for example `objdump`:

```sh
docker run --rm -it <APP_IMAGE>
objdump -p openssl_curl_example | grep -E "NEEDED|RUNPATH|RPATH"
  NEEDED               libssl.so.1.1
  NEEDED               libcrypto.so.1.1
  NEEDED               libcurl.so.4
  NEEDED               libc.so.6
  NEEDED               ld-linux-armhf.so.3
  RUNPATH              $ORIGIN/lib
```

> **IMPORTANT**
> Make sure SSH is enabled on the device before running the following commands.

For even better information on where the application binary will search for
dependencies, SSH in to the device, and check the installed application binary
with `ldd`:

```sh
ssh root@<AXIS_DEVICE_IP>
ldd /usr/local/packages/openssl_curl_example/openssl_curl_example

ldd:  libssl.so.1.1 => /usr/local/packages/openssl_curl_example/lib/libssl.so.1.1 (0x76eab000)
 libcrypto.so.1.1 => /usr/local/packages/openssl_curl_example/lib/libcrypto.so.1.1 (0x76cf9000)
 libcurl.so.4 => /usr/local/packages/openssl_curl_example/lib/libcurl.so.4 (0x76c95000)
 libc.so.6 => /usr/lib/libc.so.6 (0x76b9a000)
 /lib/ld-linux-armhf.so.3 => /usr/lib/ld-linux-armhf.so.3 (0x76f22000)
 libpthread.so.0 => /usr/lib/libpthread.so.0 (0x76b75000)
 libdl.so.2 => /usr/lib/libdl.so.2 (0x76b62000)
 libz.so.1 => /usr/lib/libz.so.1 (0x76b42000)
```

Here you can see that the application binary uses the bundled libraries.

## Troubleshooting

You can achieve basic debugging in the shape of more verbose curl output by
using the build option `--build-arg APP_DEBUG=yes`. This can give more insights
to curl error codes.

### Error CURLE_PEER_FAILED_VERIFICATION (60)

Build example with debug options to get more log output from curl. If this is
logged:

```txt
SSL certificate problem: certificate is not yet valid
```

check that your device has correct date and time settings. The created
certificate (see .pem-file in Dockerfile) in this example has a short time of
validity (should be around a week) and will not be accepted as valid on a
device with date and time outside of this range.

## License

**[Apache License 2.0](../../LICENSE)**
