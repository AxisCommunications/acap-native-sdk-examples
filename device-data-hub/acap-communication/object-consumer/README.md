*Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP application that subscribes to Device Data Hub

This application subscribes to a topic using the [Device Data Hub API](https://developer.axis.com/acap/api/#device-data-hub-api) created by another application and logs the received data.
It is part of the acap-communication example. For context, start by reading the README for the object_detector application.

## Project structure

The files for building the application are organized in the following structure.

```sh
object-consumer
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── object_consumer.c
├── Dockerfile
└── README.md
```

- **app/LICENSE** - Open source licensed source code distributed with the specified application.
- **app/Makefile** - Build and link instructions for the specified application.
- **app/manifest.json** - Definition of the *object_consumer* application and its configuration.
- **app/object_consumer.c** - Source code for the *object_consumer* application.
- **Dockerfile** - Dockerfile with the specified Axis toolchain and API container to build the application specified.
- **README.md** - Step by step instructions on how to run the *object_consumer* application.

## Program structure and behavior

### Connecting to Device Data Hub

The application creates a Device Data Hub client and connects to the service. The `dh_client_connect` function uses the current Linux user as the username, which in this case is *acap-object_consumer*.

### Subscribing to a topic

The application creates a `DHSubscriber` to subscribe to *acap.object_detector*.
The functions of `DHSubscriberListener` listener struct handles incoming data by logging it.

> [!NOTE]
> You can subscribe to topics that don't exist yet. Data will arrive once the topic is created and
> published to.

## Access rights

By default, an ACAP application can only access its own topics (prefixed with `acap.<appname>`) and
public topics. Since *acap.object_detector* belongs to another application, this application needs
permission to read from it.

The *object_detector* application grants this permission in its manifest,
giving *acap-object_consumer* read access to *acap.object_detector* and all sub-topics.

## The manifest

To be able to connect to Device Data Hub, enable Device Data Hub in the manifest:

```json
"resources": {
    "deviceDataHub_beta2": {
        "enabled": true
    }
}
```

The application will get access rights to create, delete, write to and subscribe to the topic
*acap.object_consumer* and all topics that have this topic as a prefix. However, this
application never creates any such topic. Instead, the application subscribes to the topic
*acap.object_detector*. The access rights to read data from that topic is given to this
application by the *object_detector* application.

## Build the application

> [!NOTE]
>
> For detailed information on how to build, install, and run ACAP applications, refer to the official ACAP documentation: [Build, install, and run](https://developer.axis.com/acap/develop/build-install-run/).
>
> Depending on the network your local build machine is connected to, you may need to add proxy
> settings for Docker. See
> [Proxy in build time](https://developer.axis.com/acap/develop/proxy/#proxy-in-build-time).

Standing in your working directory, run the following commands:

```sh
docker build --platform=linux/amd64 --build-arg ARCH=<ARCH> --tag <APP_IMAGE> .
```

- `<ARCH>` is the SDK architecture, `armv7hf` or `aarch64`.
- `<APP_IMAGE>` is the name to tag the image with, e.g., `consumer:1.0`

Copy the result from the container image to a local directory `build`:

```sh
docker cp $(docker create --platform=linux/amd64 <APP_IMAGE>):/opt/app ./build
```

The `build` directory contains the build artifacts, where the ACAP application
is found with the suffix `.eap`. Depending on which SDK architecture that was
chosen, one of the following files should be found:

- `object_consumer_1_0_0_aarch64.eap`
- `object_consumer_1_0_0_armv7hf.eap`

## Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

- Click on the tab `Apps` in the device GUI
- Enable `Allow unsigned apps` toggle
- Click `(+ Add app)` button to upload the application file
- Browse to the newly built ACAP application, and select it
- Click `Install`
- Run the application by enabling the `Start` switch

## Expected output

To find the application log, browse to the *Apps* page, click on the
three dots to the right of the application and select `App log`.

If the *object_detector* application has also started, you will see
messages about the received data in the log. Each message contains an object
(human, bird or dog) and a distance.

The log output may look like this:

```text
[log prefix] object_consumer[429721]: Application started
[log prefix] Received Object Detection data: {"distance":99,"object":"human"}
[log prefix] Received Object Detection data: {"distance":105,"object":"bird"}
[log prefix] Received Object Detection data: {"distance":102,"object":"dog"}
[log prefix] Received Object Detection data: {"distance":98,"object":"human"}
[log prefix] Received Object Detection data: {"distance":110,"object":"bird"}
[log prefix] Received Object Detection data: {"distance":104,"object":"dog"}
[log prefix] object_consumer[429721]: Application terminated
```

## License

**[MIT License](./app/LICENSE)**
