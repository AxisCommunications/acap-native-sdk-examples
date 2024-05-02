*Copyright (C) 2020, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# An example of how to use containers in a native ACAP application

## Overview

### Purpose

The purpose of this example is to demonstrate a way of using containers in a native ACAP application.

The advantage of using a native ACAP application for this, as opposed to a Computer Vision ACAP application,
is that the application can be administered via the VAPIX API and the web GUI in Axis OS. It also has
the advantage that the container images can be included in the .eap application file so that the entire
application is contained in this single, installable file, without any dependency on external image repositories.

However, the actual container images that are used in this type of application may well have been built
using the ACAP Computer Vision SDK, but this is not a requirement.

### How it works

This example works by taking advantage of the option to include post-install and pre-uninstall scripts
in a native ACAP application. In the post-install script we load the container image(s) to the local
image store on the device, and in the pre-uninstall script we remove them from the store. The actual
application executable is a shell script that runs `docker compose up` when starting and
`docker compose down` before exiting. This requires that a Docker compose file has been written, describing
how to run the container(s).

In order for this to work, we need to have docker compose functionality included in the device, which
means another ACAP application must first be installed: the [Docker Compose ACAP][docker-compose-acap].

In this way we are able to construct a native ACAP application that consists of one or several containers
running on an Axis device.

### The application

This minimal example consist of an Alpine Linux container that executes a script where the [nc][nc-man]
(netcat) program displays a text on a simple web page on port 80. Port 80 in the container is then mapped
to port 8080 on the device.

## Prerequisites

- An Axis device with edge container functionality.
- Axis OS 11.10 or later.
- The [Docker Compose ACAP][docker-compose-acap] version 3.0 or later, installed and running on the device.

## Build and extract the application for armv7hf

Navigate to the root directory of this repository and then start with setting environment variables:

```sh
export ARCH="armv7hf"
export PLATFORM="linux/arm/v7"
```

Next, pull the [Alpine linux container image][alpine] for armv7hf and save it to a .tar file:

```sh
docker pull --platform=$PLATFORM alpine:3.19.1
docker save -o alpine.tar alpine:3.19.1
```

Finally build the application and extract the .eap file:

```sh
docker buildx build --build-arg ARCH=$ARCH --output build-$ARCH .
```

The .eap file can now be found in the `build-armv7hf` folder.

## Build and extract the application for aarch64

Navigate to the root directory of this repository and then start with setting environment variables:

```sh
export ARCH="aarch64"
export PLATFORM="linux/arm64/v8"
```

Next, pull the [Alpine linux container image][alpine] for aarch64 and save it to a .tar file:

```sh
docker pull --platform=$PLATFORM alpine:3.19.1
docker save -o alpine.tar alpine:3.19.1
```

Finally build the application and extract the .eap file:

```sh
docker buildx build --build-arg ARCH=$ARCH --output build-$ARCH .
```

The .eap file can now be found in the `build-aarch64` folder.

## Install the application

On your Axis device, navigate to `http://<axis_device_ip>/camera/index.html#/apps`, where <axis-device-ip>
is the IP of your device. Make sure that the [Docker Compose ACAP][docker-compose-acap] application
is installed and running. In the settings of that application `IPCSocket` must be set to `yes`.
`TCPSocket` can be set to `no` since this example does not connect to the Docker Daemon from outside
of the device. However, if you do want to run with TCP Socket, TLS should be use (`UseTLS` set to `yes`).
In that case, follow the instructions in the [Docker Compose ACAP][docker-compose-acap] for how to
generate and upload TLS certificates to the device. For the other settings refer to the documentation
in that repo.

Click on the `+Add app` button on the page and in the popup window that appears, select the .eap-file
that was built earlier.

Alternatively, you can use `upload.cgi` in the [VAPIX Application API][VAPIX-application] to directly
upload the .eap-file from command line.

## Run the application

Start the application and then use a web browser to browse to

```html
http://<axis_device_ip>:8080
```

The page should display the text "Hello from an ACAP!"

## License

**[Apache License 2.0](../LICENSE)**

<!-- Links to external references -->
<!-- markdownlint-disable MD034 -->
[alpine]: https://hub.docker.com/_/alpine
[docker-compose-acap]: https://github.com/AxisCommunications/docker-compose-acap
[nc-man]: https://www.commandlinux.com/man-page/man1/nc.1.html
[VAPIX-application]: https://www.axis.com/vapix-library/subjects/T10102231/section/t10062344/display
<!-- markdownlint-enable MD034 -->