*Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# An ACAP application using the PTZ Control Websocket API

## API specific information

The [PTZ Control WS API](https://developer.axis.com/acap/reference/supported-apis/#ptz-control-ws-api) is a VAPIX API that allows a client to perform PTZ continuous move control over a websocket connection. The client can use the API together with PTZ topics on [Device Data Hub API](https://developer.axis.com/acap/reference/supported-apis/#device-data-hub-api) to stream PTZ data.

The PTZ Control WS API is a websocket based API that creates a websocket session between the client/an ACAP application and an Axis PTZ camera. The client can use the API to list the supported versions of the API, to get the capabilities in terms of allowed functionality and ranges of pan, tilt and zoom, perform continuous move of the PTZ. The websocket session persists as long as the user wants it to send PTZ move commands.

The continuous move has a default timeout of 3 minutes, post which the move stops. The API is supported only on mechanical PTZ cameras (with some exceptions). A client can use the API discovery to know if the API is supported on a particular camera.

A client can use the [Device Data Hub API](https://developer.axis.com/acap/reference/supported-apis/#device-data-hub-api) to subscribe to PTZ specific datahub topics and stream PTZ status. The PTZ Control WS API does not support polling of PTZ data and instead provides streaming of PTZ data through the [Device Data Hub API(https://developer.axis.com/acap/reference/supported-apis/#device-data-hub-api). The PTZ datahub topics are:

- `com.axis.ptz.pan-tilt-state.v1`
- `com.axis.ptz.optical-zoom-state.v1`

### When to use the WS API

The PTZ Control WS API complements the legacy [VAPIX PTZ API (ptz.cgi)](https://developer.axis.com/vapix/network-video/pantiltzoom-api/) and is not intended to replace the existing API.

However, the VAPIX PTZ API is retained mainly for backwards compatibility. For continuous move use case, we recommend the client to use the new PTZ Control WS API. This creates an end-to-end websocket between the client and the device, removes the network layer latency and hops involved to establish a TCP and hence an HTTP connection for every couple of requests (this is how it works in VAPIX PTZ API).

If a client is interested in PTZ control for more than continuous move, they can either use the PTZ Control WS API and VAPIX PTZ API in combination or only the VAPIX PTZ API for simplicity.

### When to subscribe to the datahub PTZ topics

The datahub PTZ topics are available on mechanical PTZ cameras on ARTPEC-7 (excluding V5925 and V5938) and ARTPEC-9 architecture. A client can subscribe to these topics independent of the PTZ Control WS API. The usecase of the topics is to stream PTZ data on the datahub stream (for example, after a PTZ control move).

The PTZ Control WS API is one such usecase where a client can subscribe to the datahub PTZ topics. After a continuous move, via the PTZ Control WS API, PTZ data is published on the datahub stream, if subscribed to the PTZ topics. This removes the need for polling of PTZ status after a continuous move command (as required in ptz.cgi).

## Outline of example

The example explains how to build an ACAP application that uses the PTZ Control WS API to perform a continuous PTZ move and consume PTZ data via the [Device Data Hub API](https://developer.axis.com/acap/reference/supported-apis/#device-data-hub-api).

The example ACAP application, in the app directory, contains the application source code to call the PTZ Control WS API.

1. Global initialization of cURL as digest authorization of the client is required.
2. Create a websocket client. The client subscribes to datahub topics of interest.
3. Immediately after subscription, datahub streams an event for each topic subscribed that gives current PTZ data.
4. Send a move command in response to the first event, triggers an event from datahub.
5. Keep the application running.

Below is the structure and files used in the example:

```none
ptz-control-ws-api
├── app
│   ├── listener.c
│   ├── listener.h
│   ├── main.c
│   ├── subscriber.c
│   ├── subscriber.h
│   ├── websocket_client.c
│   ├── websocket_client.h
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── Dockerfile
└── README.md
```

- **app/listener.h/c** - Callback that receives datahub topic updates and sends events on the datahub stream
- **app/main.c** - The main() program
- **app/subscriber.h/c** - Module that connects to datahub and subscribes to topics
- **app/websocket_client.h/c** - Module to create the websocket connection and perform authentication of the user
- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application
- **app/Makefile** - Build and link instructions for the application
- **app/manifest.json** - Defines the application and its configuration. This includes additional parameters.
- **Dockerfile** - Assembles an image containing the ACAP Native SDK and builds the application using it
- **README.md** - Step by step instructions on how to run the example

### Versions

- The feature to call PTZ Control WS API was introduced in AXIS OS 12.11
- The feature [Device Data Hub API](https://developer.axis.com/acap/reference/supported-apis/#device-data-hub-api) was introduced in 12.10
- The feature to subscribe to PTZ datahub topics was introduced in 12.11

## Getting started

These instructions are guidelines on how to execute the code.

### How to run the code

Below is the step by step instructions on how to execute the program. It starts with the generation of the .eap file and ends with running it on a device.

#### Build the application

Standing in your working directory run the following commands:

> [!NOTE]
>
> Depending on the network your local build machine is connected to, you may need to add proxy
> settings for Docker. See
> [Proxy in build time](https://developer.axis.com/acap/develop/proxy/#proxy-in-build-time).

```sh
docker build --platform=linux/amd64 --tag <APP_IMAGE> --build-arg ARCH=<ARCH> .
```

- `<APP_IMAGE>` is the name to tag the image with, e.g., `ptz_control_ws_api_example:1.0`
- `<ARCH>` is the SDK architecture, `armv7hf` or `aarch64`.

Copy the result from the container image to a local directory `build`:

```sh
docker cp $(docker create --platform=linux/amd64 <APP_IMAGE>):/opt/app ./build
```

The `build` directory contains the build artifacts, where the application
is found with suffix `.eap`, depending on which SDK architecture that was
chosen, one of these files should be found:

- `ptz_control_ws_api_example_armv7hf.eap`
- `ptz_control_ws_api_example_aarch64.eap`

> [!NOTE]
>
> For detailed information on how to build, install, and run ACAP applications, refer to the official ACAP documentation: [Build, install, and run](https://developer.axis.com/acap/develop/build-install-run/).

#### Install and start the application

Browse to the application page of the Axis device:

```none
http://<AXIS_DEVICE_IP>/index.html#apps
```

1. Click on the tab **Apps** in the device GUI
2. Enable **Allow unsigned apps** toggle
3. Click **(+ Add app)** button to upload the application file
4. Select the newly built application package, depending on architecture:

    - `ptz_control_ws_api_example_armv7hf.eap`
    - `ptz_control_ws_api_example_aarch64.eap`

5. Click **Install**
6. Run the application by enabling the **Start** switch

#### The expected output

The expected output is that:

1. When the ACAP application starts, it uses the PTZ Control WS API to make a continuous move.
2. The ACAP application then uses the datahub API to listen for that move's PTZ status, via subscribed PTZ topics.
3. When the example ACAP application receives a datahub event it logs the event in the syslog and stops the move.
4. Due to hardware and software latencies, more events will be received and logged before the move stops completely.

Note that step 1 will trigger step 3, but if a move is made manually while the ACAP application is still running, that will trigger step 3 again, producing more logs and causing that move to stop.

The application log from step 3 can be found by either

- Browsing to http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=ptz_control_ws_api_example.
- Browsing to the Apps page and select App log.

It will first log that it successfully retrieved the session token, subscribed to the datahub topics, and started running the main loop:

```none
[ INFO    ] ptz_control_ws_api_example[542446]: Retrieved session token: 5057201671775133927
[ INFO    ] ptz_control_ws_api_example[542446]: Subscribed to topic: com.axis.ptz.pan-tilt-state.v1
[ INFO    ] ptz_control_ws_api_example[542446]: Subscribed to topic: com.axis.ptz.optical-zoom-state.v1
```

It will then log a lot of datahub events, similar to below logs:

```none
[ INFO    ] ptz_control_ws_api_example[542446]: Received com.axis.ptz.pan-tilt-state.v1 topic event on channel 1, pan -169.622650, tilt -78.801727, pan moving: false, tilt moving: false, pan speed: 2.323047, tilt speed: 0.000000
[ INFO    ] ptz_control_ws_api_example[542446]: Received com.axis.ptz.optical-zoom-state.v1 topic event on channel 1, zoom mag: 1.000463, hfov: 58.307686, vfov: 34.821022, zoom moving: false
[ INFO    ] ptz_control_ws_api_example[542446]: Received com.axis.ptz.pan-tilt-state.v1 topic event on channel 1, pan -156.583588, tilt -77.959900, pan moving: false, tilt moving: false, pan speed: 0.000000, tilt speed: 0.000000
[ INFO    ] ptz_control_ws_api_example[542446]: Received com.axis.ptz.optical-zoom-state.v1 topic event on channel 1, zoom mag: 1.000463, hfov: 58.307686, vfov: 34.821022, zoom moving: false
```

## License

**[MIT](../LICENSE)**
