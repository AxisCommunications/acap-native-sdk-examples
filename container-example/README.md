*Copyright (C) 2020, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# An example of how to use containers in a native ACAP application

## Overview

### Purpose

The purpose of this example is to demonstrate a way of using containers in a native ACAP application.

The advantage of using a native ACAP application for this, as opposed to a Computer Vision ACAP application,
is that the application can be administered via the VAPIX API and the web GUI in AXIS OS. It also has
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

- An Axis device with [container support](https://www.axis.com/support/tools/product-selector/shared/%5B%7B%22index%22%3A%5B4%2C2%5D%2C%22value%22%3A%22Yes%22%7D%5D), see more info in [Axis devices and compatibility](https://axiscommunications.github.io/acap-documentation/docs/axis-devices-and-compatibility/#acap-computer-vision-sdk-hardware-compatibility).
- AXIS OS 11.10 or later.
- The [Docker Compose ACAP][docker-compose-acap] version 3.0 or later,
  installed and running on the device.
  - In the settings of the application:
    - `IPCSocket` must be set to `yes`.
    - `TCPSocket` can be set to `no` since this example does not connect to the
      Docker Daemon from outside of the device.
      - However, if you do want to run with TCP Socket, TLS should be used
        (`UseTLS` set to `yes`).  In that case, follow the instructions in the
        [Docker Compose ACAP][docker-compose-acap] for how to generate and
        upload TLS certificates to the device.
    - For the other settings refer to the documentation in that repository.

## Getting started

These instructions will guide you on how to execute the code. Below is the
structure and scripts used in the example:

```sh
container-example
├── containerExample
├── docker-compose.yml
├── Dockerfile
├── LICENSE
├── Makefile
├── manifest.json
├── postinstall.sh
├── preuninstall.sh
└── README.md
```

- **containerExample** - Application source code in shell script.
- **docker-compose.yml** - Docker compose file to start a container on the device.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **manifest.json** - Defines the application and its configuration. This includes additional parameters.
- **postinstall.sh** - Post-install script, running at end of an application installation.
- **preuninstall.sh** - Pre-uninstall script, running before an application uninstallion.
- **README.md** - Step by step instructions on how to run the example.

### How to run the code

Below is the step by step instructions on how to execute the program. So
basically starting with the generation of the .eap file to running it on a
device.

#### Build the application

Standing in your working directory run the following commands:

> [!NOTE]
>
> Depending on the network you are connected to, you may need to add proxy settings.
> The file that needs these settings is: `~/.docker/config.json`. For reference please see
> https://docs.docker.com/network/proxy and a
> [script for Axis devices](https://axiscommunications.github.io/acap-documentation/docs/develop/build-install-run.html#configure-network-proxy-settings) in the ACAP documentation.

##### Build the application for armv7hf

Pull the [Alpine linux container image][alpine] for armv7hf and save it to a
.tar file, then build the application:

```sh
docker pull --platform="linux/arm/v7" alpine:3.19.1
docker save -o alpine.tar alpine:3.19.1
docker build --build-arg ARCH=armv7hf --tag container-example:armv7hf .
docker cp $(docker create container-example:armv7hf):/opt/app ./build-armv7hf
```

##### Build the application for aarch64

Pull the [Alpine linux container image][alpine] for aarch64 and save it to a
.tar file, then build the application:

```sh
docker pull --platform="linux/arm64/v8" alpine:3.19.1
docker save -o alpine.tar alpine:3.19.1
docker build --build-arg ARCH=aarch64 --tag container-example:aarch64 .
docker cp $(docker create container-example:aarch64):/opt/app ./build-aarch64
```

##### Extract the application

The `build-armv7hf` and `build-aarch64` directories contain the build
artifacts, where the ACAP application is found with suffix `.eap`, depending on
which SDK architecture that was chosen, one of these files should be found:

- `Container_Example_1_0_0_aarch64.eap`
- `Container_Example_1_0_0_armv7hf.eap`

#### Install your application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

- Click on the tab `App` in the device GUI
- Click `(+)` sign to upload the application file
- Browse to the newly built ACAP application, depending on architecture:
  - `Container_Example_1_0_0_aarch64.eap`
  - `Container_Example_1_0_0_armv7hf.eap`
- Click `Install`
- Run the application by enabling the `Start` switch

#### The expected output

Browse to `http://<AXIS_DEVICE_IP>:8080`, the page should display the text
**Hello from an ACAP!**.

## License

**[Apache License 2.0](../LICENSE)**

<!-- Links to external references -->
<!-- markdownlint-disable MD034 -->
[alpine]: https://hub.docker.com/_/alpine
[docker-compose-acap]: https://github.com/AxisCommunications/docker-compose-acap
[nc-man]: https://www.commandlinux.com/man-page/man1/nc.1.html
<!-- markdownlint-enable MD034 -->