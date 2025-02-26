*Copyright (C) 2024, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP application calling VAPIX APIs

This example shows how an ACAP application can call
[VAPIX](https://www.axis.com/vapix-library) APIs,
Axis open APIs.

The collection of VAPIX APIs enables a wide selection of functionality to an
ACAP application. Some examples:

- Call [basic device information](https://www.axis.com/vapix-library/subjects/t10175981/section/t10132180/display?section=t10132180-t10132179)
  API to get a device's serial number and system-on-chip (SoC).
- Call [list installed applications](https://www.axis.com/vapix-library/subjects/t10102231/section/t10036126/display?section=t10036126-t10010644)
  API to get installed ACAP applications.

## Outline of example

1. Retrieve VAPIX credentials through a D-Bus API.
2. Use curl to make a HTTP POST request to VAPIX API **Basic device
   information** on dedicated local host IP `127.0.0.12`.
3. Parse out fields from the answer

## Practical information

### API to get VAPIX credentials

- The credentials should be re-fetched each time the ACAP application starts
  and should only be kept in memory by the ACAP application, not stored in any
  file.
- See more information of this feature in [ACAP documentation](https://developer.axis.com/acap/develop/VAPIX-access-for-ACAP-applications).

#### Versions

- The feature to call VAPIX APIs was introduced in AXIS OS 11.6 as Beta.
- The format of the D-Bus API to get VAPIX credentials changed in 11.8.
- The D-Bus API to get VAPIX credentials reached General Availability in 11.9.

### VAPIX APIs

- Some VAPIX APIs may work with GET, but it's recommended to use POST for all
  API calls since it should cover all use cases of GET.
- The response format varies for different VAPIX APIs, but common response formats
  are JSON, XML and text. See the VAPIX documentation of the specific API for
  information of format.

### Global proxy configuration

- If the device has set global device proxy, reaching the local virtual host
  (127.0.0.12) is only possible if 127.0.0.12 is added to the `No proxy` list,
  as described in
  [Configure global device proxy](https://developer.axis.com/acap/develop/proxy/#configure-global-device-proxy).

## Getting started

These instructions will guide you on how to execute the code. Below is the
structure and scripts used in the example:

```sh
vapix
├── app
│   ├── vapix_example.c
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── Dockerfile
└── README.md
```

- **app/vapix_example.c** - Application source code in C.
- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json** - Defines the application and its configuration. This includes additional parameters.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.

### How to run the code

Below is the step by step instructions on how to execute the program. So
basically starting with the generation of the .eap file to running it on a
device.

#### Build the application

Standing in your working directory run the following commands:

> [!NOTE]
>
> Depending on the network your local build machine is connected to, you may need to add proxy
> settings for Docker. See
> [Proxy in build time](https://developer.axis.com/acap/develop/proxy/#proxy-in-build-time).

```sh
docker build --tag <APP_IMAGE> --build-arg ARCH=<ARCH> .
```

- `<APP_IMAGE>` is the name to tag the image with, e.g., `vapix_example:1.0`
- `<ARCH>` is the SDK architecture, `armv7hf` or `aarch64`.

Copy the result from the container image to a local directory `build`:

```sh
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

The `build` directory contains the build artifacts, where the ACAP application
is found with suffix `.eap`, depending on which SDK architecture that was
chosen, one of these files should be found:

- `vapix_example_1_0_0_aarch64.eap`
- `vapix_example_1_0_0_armv7hf.eap`

#### Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

- Click on the tab `Apps` in the device GUI
- Enable `Allow unsigned apps` toggle
- Click `(+ Add app)` button to upload the application file
- Browse to the newly built ACAP application, depending on architecture:
  - `vapix_example_1_0_0_aarch64.eap`
  - `vapix_example_1_0_0_armv7hf.eap`
- Click `Install`
- Run the application by enabling the `Start` switch

#### The expected output

The application log can be found by either

- Browse to `http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=vapix_example`.
- Browse to the application page and click the `App log`.

The log shows a few parsed values from the VAPIX API response.

```text
----- Contents of SYSTEM_LOG for 'vapix_example' -----

[ INFO ] vapix_example[9731]: Curl version 8.6.0
[ INFO ] vapix_example[9731]: Jansson version 2.14
[ INFO ] vapix_example[9731]: ProdShortName: AXIS Q3536-LVE
[ INFO ] vapix_example[9731]: Soc: Axis Artpec-8
[ INFO ] vapix_example[9731]: SocSerialNumber: ABCD1234-0101ABAB
```

> [!NOTE]
>
> The curl and Jansson versions mentioned in the example log are only for representation
> purpose. They may vary according to the library version available and linked from
> AXIS OS on which the application runs.

## License

**[Apache License 2.0](../LICENSE)**
