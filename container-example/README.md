 *Copyright (C) 2020, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# An example of how to use containers in a native ACAP4 application

## Overview

### Purpose

The purpose of this example is to demonstrate a way of using containers in a native ACAP application.

The advantage of using a native ACAP application for this, as opposed to a Computer Vision ACAP application, is that the application can be administered via the VAPIX API and the web GUI in AxOS. It also has the advantage that the container images can be included in the .eap application file so that the entire application is contained in this single, installable file, without any dependency on external image repositories.

However, the actual container images that are used in this type of application may well have been built using the Computer Vision SDK, but this is not a requirement.

### How it works

This example works by taking advantage of the option to include post-install and pre-uninstall scripts in a native ACAP. In the post-install script we load the container image(s) to the local image store on the device, and in the pre-uninstall script we remove them from the store. The actual application executable is a shell script that runs "docker compose up" when starting and "docker compose down" before exiting. This requires that a Docker compose file has been written, describing how to run the container(s). 

In order for this to work, we need to have docker compose functionality included in the device, which means another ACAP must first be installed: the [Docker Compose ACAP](https://github.com/AxisCommunications/docker-compose-acap)

In this way we are able to construct a native ACAP that consists of one or several containers running on an Axis device.

### The application

This minimal example consist of an Alpine Linux container that executes a script where the nc (netcat) program displays a text on a simple web page on port 80. Port 80 in the container is then mapped to port 8080 on the device.  

## Prerequisites
* An Axis camera with edge container functionality
* AxOS 10.7 or later
* The [Docker Compose ACAP](https://github.com/AxisCommunications/docker-compose-acap) must be installed on the device

## Build the application

### Pull and save the Alpine linux container image

Standing in your working directory run the following commands:

```bash
docker pull arm32v7/alpine:3.14.0
docker save -o alpine.tar arm32v7/alpine:3.14.0
```
If you have a device with an aarch64 architecture you need to change arm32v7 to arm64v8 above.

### Build the ACAP

Standing in your working directory run the following command:

```bash
docker build --tag eap-container-example .
```

### Extract the .eap file from the container
```bash
docker cp $(docker create eap-container-example:latest):/opt/app/Container_Example_1_0_0_armv7hf.eap .
```

## Install the application

Browse to the following page (replace <axis_device_ip> with the IP number of your Axis video device)

```bash
http://<axis_device_ip>/#settings/apps
```

*Goto your device web page above > Click on the tab **App** in the device GUI > Add **(+)** sign and browse to
the newly built **Container_Example_1_0_0_armv7hf.eap** > Click **Install** > Run the application by enabling the **Start** switch*

## Test the application

Use a web browser to browse to
```
http://<axis_device_ip>:8080
```
The page should display the text "Hello from an ACAP!"

## License
**[Apache License 2.0](../LICENSE)**
