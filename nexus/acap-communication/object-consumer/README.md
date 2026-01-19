*Copyright (C) 2025, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP application that subscribes on Nexus

This application is part of the *acap-communication* example. It is recommended to first
read the README file of the *object_detector* application.

## Introduction

This application subscribes to a topic created by another application and logs the received data.

### Connecting to Nexus

The application creates a client and connects. The `Connect` function uses the current Linux user as
the username, which in this case is *acap-object_consumer*.

### Subscribing to a topic

The application creates a `TopicDataSubscriber` to subscribe to *acap.object_detector*.
The `ObjectLogger` listener class handles incoming data by logging it.

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

To be able to connect to Nexus, enable Nexus in the manifest:

```json
"resources": {
    "nexus_beta1": {
        "enabled": true
    }
}
```

The application will get access rights to create, delete, write to and subscribe to the topic
*acap.object_consumer* and all topics that have this topic as a prefix. However, this
application never creates any such topic. Instead, the application subscribes to the topic
*acap.object_detector*. The access rights to read data from that topic is given to this
application by the *object_detector* application.

## Nexus API documentation

[Link to documentation](https://developer.axis.com/acap/api/src/api/nexus-client-cpp/html/index.html)

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
chosen, one of following files should be found:

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
[log prefix] object_consumer[429721]: Received data: {"distance":90,"object":"human"}
[log prefix] object_consumer[429721]: Received data: {"distance":150,"object":"bird"}
[log prefix] object_consumer[429721]: Received data: {"distance":120,"object":"dog"}
[log prefix] object_consumer[429721]: Received data: {"distance":89,"object":"human"}
[log prefix] object_consumer[429721]: Received data: {"distance":155,"object":"bird"}
[log prefix] object_consumer[429721]: Received data: {"distance":122,"object":"dog"}
[log prefix] object_consumer[429721]: Application terminated
```

## License

**[MIT License](./app/LICENSE)**
