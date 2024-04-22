*Copyright (C) 2024, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP application using curl and OpenSSL

This example demonstrate how to transfer content from a web server `www.example.com` to the application directory by using [OpenSSL](https://www.openssl.org/) and [curl](https://curl.se/) APIs from the SDK.

## curl and OpenSSL

Documentation of API functions used in this example:

- https://curl.se/libcurl/c
- https://www.openssl.org/docs/man3.0/man3/

## Getting started

These instructions will guide you on how to execute the code. Below is the
structure and scripts used in the example:

```sh
curl_openssl
├── app
│   ├── curl_openssl.c
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── Dockerfile
└── README.md
```

- **app/curl_openssl.c** - Application source code.
- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json** - Defines the application and its configuration. This includes additional parameters.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.

## How to run the code

Below is the step by step instructions on how to execute the program. So
basically starting with the generation of the .eap file to running it on a
device.

### Build the application

Standing in your working directory run the following commands:

> [!NOTE]
>
> Depending on the network you are connected to, you may need to add proxy settings.
> The file that needs these settings is: `~/.docker/config.json`. For reference please see
> https://docs.docker.com/network/proxy and a
> [script for Axis devices](https://axiscommunications.github.io/acap-documentation/docs/develop/build-install-run.html#configure-network-proxy-settings) in the ACAP documentation.

```sh
docker build --tag <APP_IMAGE> --build-arg ARCH=<ARCH> .
```

- `<APP_IMAGE>` is the name to tag the image with, e.g., `curl_openssl:1.0`
- `<ARCH>` is the SDK architecture, `armv7hf` or `aarch64`.

If the device is inside a network with a proxy, then it can be passed on as a
build argument `APP_PROXY`:

> [!IMPORTANT]
> Pass proxy argument without `http://`.

```sh
docker build --build-arg APP_PROXY=<MY_PROXY> --tag <APP_IMAGE> --build-arg ARCH=<ARCH> .
```

To get more verbose logging from curl, pass the build argument `APP_DEBUG=yes`:

```sh
docker build --build-arg APP_DEBUG=yes --tag <APP_IMAGE> --build-arg ARCH=<ARCH> .
```

Copy the result from the container image to a local directory `build`:

```sh
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

The `build` directory contains the build artifacts, where the ACAP application
is found with suffix `.eap`, depending on which SDK architecture that was
chosen, one of these files should be found:

- `curl_openssl_1_0_0_aarch64.eap`
- `curl_openssl_1_0_0_armv7hf.eap`

### Install your application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/#settings/apps
```

- Click on the tab `App` in the device GUI
- Click `(+)` sign to upload the application file
- Browse to the newly built ACAP application, depending on architecture:
  - `curl_openssl_1_0_0_aarch64.eap`
  - `curl_openssl_1_0_0_armv7hf.eap`
- Click `Install`
- Run the application by enabling the `Start` switch

### The expected output

#### Transferred file

The application uses the bundled CA certificate `cacert.pem` to fetch the
HTTPS content of https://www.example.com and stores the content in
`/usr/local/packages/curl_openssl/localdata/www.example.com.txt` on the
device.

> [!NOTE]
> Make sure
> [SSH is enabled](https://axiscommunications.github.io/acap-documentation/docs/get-started/set-up-developer-environment/set-up-device.html#access-the-device-through-ssh)
> on the device to run the following commands.

Compare the web page source code to the content of file `www.example.com.txt`.

```sh
ssh root@<AXIS_DEVICE_IP>
cat /usr/local/packages/curl_openssl/localdata/www.example.com.txt

(HTML content of www.example.com)
```

#### Application log

The application log can be found by either

- Browse to `http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=curl_openssl`.
- Browse to the application page and click the `App log`.

> [!NOTE]
> Make sure
> [SSH is enabled](https://axiscommunications.github.io/acap-documentation/docs/get-started/set-up-developer-environment/set-up-device.html#access-the-device-through-ssh)
> on the device to run the following commands.

```sh
ssh root@<AXIS_DEVICE_IP>
head -50 /var/log/info.log
```

> [!NOTE]
> curl and OpenSSL versions mentioned in the example log are for only representation
> purpose. They may vary according to the library version available and linked from
> AXIS OS on which the application runs.

```sh
----- Contents of SYSTEM_LOG for 'curl_openssl' -----

17:17:50.665 [ INFO ] curl_openssl[1687]: ACAP application curl version: 8.6.0
17:17:50.665 [ INFO ] curl_openssl[1687]: ACAP application openssl version: OpenSSL 3.0.13 30 Jan 2024
17:17:50.669 [ INFO ] curl_openssl[1687]: curl easy init successful - handle has been created
17:17:50.669 [ INFO ] curl_openssl[1687]: *** 1. Transfer requested without certificate ***
17:17:50.954 [ INFO ] curl_openssl[1687]: *** 1. Transfer Failed: Expected result, transfer without certificate should fail ***
17:17:50.954 [ INFO ] curl_openssl[1687]: *** 2. Transfer requested with CA-cert ***
17:17:51.290 [ INFO ] curl_openssl[1687]: *** 2. Transfer Succeeded: Expected result, transfer with CA-cert should pass ***
```

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

**[Apache License 2.0](../LICENSE)**
