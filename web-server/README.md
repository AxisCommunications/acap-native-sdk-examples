*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# How to run a web server in an ACAP application

This example explains how to build and use [Monkey Web Server](https://github.com/monkey/monkey) in ACAP Native SDK with Reverse Proxy configuration in Apache Server.
This example addresses a similar problem as the [web-server-using-fastcgi](../web-server-using-fastcgi) example but in a more modular way as the application can have its own web server without need of being strongly integrated with the device.

## Reverse Proxy configuration in Apache Server

[Reverse Proxy configuration](https://httpd.apache.org/docs/2.4/howto/reverse_proxy.html) provides a flexible way for an ACAP application to expose an external API through the Apache Server in AXIS OS and internally route the requests to a small Web Server running in the ACAP application.

**Reverse proxy** is a technique that can be used for exposing many types of network APIs and can e.g. cover the same CGI use cases as the old [AxHTTP](https://axiscommunications.github.io/acap-documentation/docs/acap-sdk-version-3/api/#http-api) API and is a different implementation to the [FastCGI](https://axiscommunications.github.io/acap-documentation/docs/api/native-sdk-api.html#fastcgi) API.

The Apache server is configured using the manifest.json file in an ACAP application. In manifest.json under `configuration`, it is possible to specify a `settingPage` and a `reverseProxy` where the latter will connect the Monkey server to the Apache server.

The Web Server running in the ACAP application can also be exposed directly to the network by allowing external access to the port in the network configuration for the device. There are some disadvantages with exposing Web Server directly to the network such as non standard ports and no reuse of authentication, TLS and other features that comes with Apache Server.

## Monkey Web Server

Monkey is a fast and lightweight Web Server for Linux. It has been designed to be very scalable with low memory and CPU consumption, the perfect solution for Embedded Linux and high end production environments. Besides the common features as HTTP server, it expose a flexible C API which aims to behave as a fully HTTP development framework, so it can be extended as desired through the plugins interface. The Monkey Web Server [documentation](https://github.com/monkey/monkey-docs/) describes the configuration in detail.

> [!NOTE]
> Currently, there's an issue with the Monkey web server when using the reverse proxy, impacting asset discovery. As a workaround, we're showcasing the `list` example.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure used in the example:

```sh
web-server
├── app
│   ├── LICENSE - Text file which lists all open source licensed source code distributed with the application
│   └── manifest.json - Defines the application and its configuration
├── Dockerfile - Docker file with the specified Axis container image to build the example specified
├── monkey.patch - Patch for using monkey examples in a native ACAP
└── README.md - Step by step instructions on how to run the example
```

## Limitations

- Apache Reverse Proxy can not translate content with absolute addresses (i.e. /image.png) in the HTML page. Use only relative content (i.e. image.png or ../image.png)). More information how to handle relative URLs correctly with a reverse proxy [here](https://serverfault.com/questions/561892/how-to-handle-relative-urls-correctly-with-a-reverse-proxy).

## Proxy settings

Depending on the network, you might need proxy settings in the following file: *~/.docker/config.json

For reference please see: https://docs.docker.com/network/proxy/.

## How to build the code

Standing in your working directory run the following commands:

```sh
# Set your device IP address and architecture
export ARCH=<armv7hf or aarch64>
export AXIS_DEVICE_IP=<Axis device IP address>
export PASS=<device password>

docker build . --build-arg ARCH --tag web-server:$ARCH
```

## Install your application

Installing your application on an Axis video device is as simple as:

```sh
docker run --rm web-server:$ARCH eap-install.sh $AXIS_DEVICE_IP $PASS install
```

# Start your application using a web browser

Goto your device web page > Click on the tab **Apps** in the device GUI and locate the application. Run the application by enabling the **Start** switch.

The Web Server can be accessed from a Web Browser through the Apache Server in the device using an extension to the device web URL (i.e http://<AXIS_DEVICE_IP>/local/list/my_web_server)

# Start your application from command-line

As an alternative the application can be started, stopped and removed from command-line using following commands:

```sh
docker run --rm web-server:$ARCH eap-install.sh $AXIS_DEVICE_IP $PASS start
docker run --rm web-server:$ARCH eap-install.sh $AXIS_DEVICE_IP $PASS stop
docker run --rm web-server:$ARCH eap-install.sh $AXIS_DEVICE_IP $PASS remove
```

## License

**[Apache License 2.0](../LICENSE)**
