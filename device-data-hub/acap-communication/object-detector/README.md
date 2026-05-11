*Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP application that publishes on Device Data Hub

This application publishes data to a Device Data Hub topic using the [Device Data Hub API](https://developer.axis.com/acap/api/#device-data-hub-api).
The object_detector application demonstrates how to create a topic, detect consumer matches, and publish data when consumers are connected.

## Project structure

The files for building the application are organized in the following structure.

```sh
object-detector
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── object_detector.c
├── Dockerfile
└── README.md
```

- **app/LICENSE** - Open source licensed source code distributed with the specified application.
- **app/Makefile** - Build and link instructions for the specified application.
- **app/manifest.json** - Definition of the *object_detector* application and its configuration.
- **app/object_detector.c** - Source code for the *object_detector* application.
- **Dockerfile** - Dockerfile with the specified Axis toolchain and API container to build the application specified.
- **README.md** - Step by step instructions on how to run the *object_detector* application.

## Program structure and behavior

### Connecting to Device Data Hub

The application first creates a Device Data Hub client and connects to the service. The `dh_client_connect` function uses the current Linux user
as the username, which in this case is *acap-object_detector*  (the part after "acap-" is the
application name).

### Creating a topic

The application creates the topic *acap.object_detector*. Before doing that, it first tries
to delete the topic in case it already exists.

The argument to `dh_client_create_topic` is a JSON string containing the topic definition: name, version,
description, and data schema. The schema specifies that data is a JSON object with "object" and
"distance" properties.

### Publishing data

The application creates a topic data writer with a `on_consumer_match_update` to receive notifications
when subscribers connect or disconnect. When `DH_CONSUMER_MATCH` is received, consumers
exist; when `DH_CONSUMER_NO_MATCH` is received, there are no subscribers.
Data is only written when matching consumers exist, and must conform to the topic's data schema.

## Access rights

This application has default access rights. This means that the application can create, delete,
write to and read from its own topics. These topics are the topic *acap.object_detector*
(that is, <!-- textlint-disable terminology -->"acap."<!-- textlint-enable terminology -->
followed by the application name) and all topics that have this topic as a
prefix. All applications can also read from public topics. If you do not have access rights to
do something with a topic, you will receive an `ApiError`, for example `DH_ERR_AUTHORIZATION_FAILED`.

The manifest grants read access for the application's topics to the user *acap-object_consumer*,
the user of the *object_consumer* application.

## The manifest

The manifest has code that enables Device Data Hub for this application. By just enabling Device Data Hub, the
application *object_detector* will get default access rights, that is access rights to create,
delete, write to and subscribe to the topic *acap.object_detector* and all topics that have
this topic as a prefix.

If an application wants to give other applications access to its topics, this has to be specified
in the manifest. It is done in the "accessControlList" array. Below you can see the JSON code in
the manifest that handles Device Data Hub:

```json
"resources": {
    "deviceDataHub_beta2": {
        "enabled": true,
        "accessControlList": [
            {
                "topics": [".#"],
                "usernames": ["acap-object_consumer"],
                "operations": ["read"]
            }
        ]
    }
}
```

All topics that are specified in the "topics" array will get the prefix "acap.object_detector".
The '#' character is a multi-level wildcard that matches zero or more topic segments starting at
its position. This means that the manifest above will give the user *acap-object_consumer*
read access to the topic *acap.object_detector* and all topics that have this topic as a
prefix, including *acap.object_detector.object_detection_data*. In this way, the application
*object_consumer* will be able to read data that the *object_detector* application writes to
the topic *acap.object_detector*.

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
- `<APP_IMAGE>` is the name to tag the image with, e.g., `detector:1.0`

Copy the result from the container image to a local directory `build`:

```sh
docker cp $(docker create --platform=linux/amd64 <APP_IMAGE>):/opt/app ./build
```

The `build` directory contains the build artifacts, where the ACAP application
is found with the suffix `.eap`. Depending on which SDK architecture that was
chosen, one of the following files should be found:

- `object_detector_1_0_0_aarch64.eap`
- `object_detector_1_0_0_armv7hf.eap`

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

In the log, you should see a message that the application has started. When the
*object_consumer* application is also started, there will be a message that
consumers exist. If there are no more consumers, there will be a message about that.

This application produces data, but the data is not printed to this log. Instead, the
*object_consumer* application prints the data that it receives.

The log output may look like this:

```text
[log prefix] Application started
[log prefix] Consumer match update - Production ID: 14, Status: DH_CONSUMER_MATCH
[log prefix] Consumer match update - Production ID: 14, Status: DH_CONSUMER_NO_MATCH
[log prefix] Application terminated
```

## License

**[MIT License](./app/LICENSE)**
