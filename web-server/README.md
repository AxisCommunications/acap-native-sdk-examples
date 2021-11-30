# How to use Web Server in ACAP4 Native Application

This example explains how to build and use [Monkey Web Server](https://github.com/monkey/monkey) in ACAP4 Native SDK with Reverse Proxy configuration in Apache Server.

## Reverse Proxy configuration in Apache Server

[Reverse Proxy configuration](https://httpd.apache.org/docs/2.4/howto/reverse_proxy.html) provides a flexible way for an ACAP application to expose an external API through the Apache Server in AxOs system and internally route the requests to a small Web Server running in the ACAP application.

Reverse Proxy is a technique that can be used for exposing many types of network APIs and can e.g. cover same cgi usecases as axHttp API from ACAP4 Native SDK.

The Apache server is configured using post-install and pre-uninstall scripts features in a native ACAP. The post-install script adds a configuration file to apache configuration with reverse configuration for monkey server and applies it to Apache Server, and in the pre-uninstall the configuration is removed.

The Web Server running in the ACAP application can also be exposed directly to the network by allowing external access to the port in the network configuration for the device. There are some disavantages with exposing Web Server directly to the network such as non standard ports and no reuse of authentication, TLS and other features that comes with Apache Server.

## Monkey Web Server

Monkey is a fast and lightweight Web Server for Linux. It has been designed to be very scalable with low memory and CPU consumption, the perfect solution for Embedded Linux and high end production environments. Besides the common features as HTTP server, it expose a flexible C API which aims to behave as a fully HTTP development framework, so it can be extended as desired through the plugins interface. The Monkey Web Server [documentation](http://monkey-project.com/documentation/1.5) describes the configuration in detail.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```bash
web-server
├── app
│   ├── LICENSE - Text file which lists all open source licensed source code distributed with the application
│   ├── manifest.json - Defines the application and its configuration
│   ├── postinstall.sh - Shell script for adding configration to apache
│   ├── preuninstall.sh - Shell script for removing  configration to apache
│   └── reverseproxy.conf - Configuration for reverse proxy
├── Dockerfile - Docker file with the specified Axis toolchain and API container to build the example specified
├── monkey.patch - Patch for using monkey examples in a native ACAP
└── README.md - Step by step instructions on how to run the example
```

## Limitations

* Apache Reverse Proxy can not translate content with absolute addresses (i.e. /image.png) in the HTML page. Use only relative content (i.e. image.png or ../image.png)). More information how to handle relative urls correctly with a reverse proxy [here](https://serverfault.com/questions/561892/how-to-handle-relative-urls-correctly-with-a-reverse-proxy).

## How to build the code

> [!IMPORTANT]
> *Depending on the network you are connected to,
The file that needs those settings is:*~/.docker/config.json.*
For reference please see: https://docs.docker.com/network/proxy/ and a
[script for Axis device here](../FAQs.md#HowcanIset-upnetworkproxysettingsontheAxisdevice?).*

Standing in your working directory run the following commands:

```bash
docker build --tag <APP_IMAGE> .
```

<APP_IMAGE> is the name to tag the image with, e.g., vdoencodeclient:1.0

Default architecture is **armv7hf**. To build for **aarch64** it's possible to
update the *ARCH* variable in the Dockerfile or to set it in the docker build
command via build argument:

```bash
docker build --build-arg ARCH=aarch64 --tag <APP_IMAGE> .
```

Copy the result from the container image to a local directory build:

```bash
docker cp $(docker create <APP_IMAGE>):/opt/monkey/examples ./build
```

A build subfolder now contains the following files:

```bash
web-server
├── build/
│   ├── monkey* - Application executable binary file.
│   ├── monkey_1_0_0_armv7hf.eap - Application package .eap file.
│   ├── monkey_1_0_0_LICENSE.txt - Copy of LICENSE file.
│   ├── package.conf - Defines the application and its configuration.
│   ├── package.conf.orig - Defines the application and its configuration, original file.
│   ├── param.conf - File containing application parameters.
│   └── ...
└── ...
```

## Install your application

Installing your application on an Axis video device is as simple as:

Browse to the following page (replace <axis_device_ip> with the IP number of your Axis video device)

```bash
http://<axis_device_ip>/#settings/apps
```

Goto your device web page above > Click on the tab **App** in the device GUI > Add **(+)** sign and browse to
the newly built **monkey_1_0_0_armv7hf.eap** > Click **Install** > Run the application by enabling the **Start** switch

## Browse to the web server

The Web Server can be accessed from a Web Browser eighter directly using a port number (i.e. http://mycamera:2001) or through the Apache Server in the camera using an extension to the camera web URL (i.e http://mycamera/monkey/).

## C API Examples

Some C API examples are included in the app folder. To build any of the examples, use the build and install procedure as described above after making following changes to the build files:

1. app/manifest.json: Replace AppName "monkey" with the name of the example: hello, list or quiz
2. Dockerfile: Replace monkey in /usr/local/packages/monkey with the name of the example: hello, list or quiz

## License

**[Apache License 2.0](../LICENSE)**
