*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# Serve HTTP requests through reverse proxy

This example demonstrates how to setup the Axis device web server (Apache) in a
[Reverse Proxy](https://httpd.apache.org/docs/2.4/howto/reverse_proxy.html)
configuration, where HTTP requests to the application are routed to a web server
[Monkey](https://github.com/monkey/monkey) running inside the ACAP application
and acting as a CGI.

The advantage of a webserver proxy is that when porting existing code to your
ACAP application, its request handling can remain largely unmodified. This eases
the task of sharing code between platforms. The webserver proxy method enforces
a URL routing scheme as follows:

`http://<AXIS_DEVICE_IP>/local/<appName>/<apiPath>`

With `<appName>` and `<apiPath>` as defined in the manifest.

Note that this example shows the reverse proxy concept using Monkey, but you are
free to use any webserver of your choice.

## Alternative approach

Another example that serves HTTP requests is
[web-server-using-fastcgi](../web-server-using-fastcgi), where the Axis device
web server and the supported ACAP API
[FastCGI](https://developer.axis.com/acap/api/native-sdk-api/#fastcgi)
are used.

## Reverse proxy configuration in Apache server

A reverse proxy configuration provides a flexible way for an ACAP application
to expose an external API through the Apache Server in AXIS OS and internally
route the requests to a web server running in the ACAP application.

The Apache server is configured using the `manifest.json` file in an ACAP
application. In `manifest.json` under `configuration`, it is possible to specify
a `settingPage` and a `reverseProxy` where the latter will connect the Monkey
server to the Apache server.

Prior to manifest 1.5.0, reverse proxy was only supported through the
postinstall script. The manifest based method is more strict on URLs in order to
avoid name clashes that could occur in the old mechanism. When upgrading, your
URLs will change to the format shown in
[Serve HTTP requests through reverse proxy](#serve-http-requests-through-reverse-proxy).

The web server running in the ACAP application can also be exposed directly to
the network by allowing external access to the port in the network
configuration for the device. There are disadvantages with exposing Web
Server directly to the network such as non standard ports and no reuse of
authentication, TLS and other features that comes with Apache Server.

## Monkey web server

Monkey is a fast and lightweight web server for Linux. It has been designed to
be very scalable with low memory and CPU consumption, the perfect solution for
Embedded Linux and high end production environments. Besides the common
features as HTTP server, it expose a flexible C API which aims to behave as a
fully HTTP development framework, so it can be extended as desired through the
plugins interface. The Monkey Web Server
[documentation](https://github.com/monkey/monkey-docs/) describes the
configuration in detail.

> [!NOTE]
> The main license of the Monkey repository is Apache, but some files
> are licensed under other types such as GPL. Make sure to control how it
> affects your application in order to be compliant.

## Getting started

These instructions will guide you on how to execute the code. Below is the
structure used in the example:

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

- Apache Reverse Proxy can not translate content with absolute addresses (i.e.
  /image.png) in the HTML page. Use only relative content (i.e. image.png or
../image.png). More information how to handle relative URLs correctly with a
reverse proxy
[here](https://serverfault.com/questions/561892/how-to-handle-relative-urls-correctly-with-a-reverse-proxy).

## Proxy settings

Depending on the network, you might need proxy settings in the following file: *~/.docker/config.json

For reference please see: https://docs.docker.com/network/proxy/.

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

- `<APP_IMAGE>` is the name to tag the image with, e.g., `web-server:1.0`
- `<ARCH>` is the SDK architecture, `armv7hf` or `aarch64`.

Copy the result from the container image to a local directory `build`:

```sh
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

The `build` directory contains the build artifacts, where the ACAP application
is found with suffix `.eap`, depending on which SDK architecture that was
chosen, one of these files should be found:

- `web_server_rev_proxy_1_0_0_aarch64.eap`
- `web_server_rev_proxy_1_0_0_armv7hf.eap`

#### Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

- Click on the tab `Apps` in the device GUI
- Enable `Allow unsigned apps` toggle
- Click `(+ Add app)` button to upload the application file
- Browse to the newly built ACAP application, depending on architecture:
  - `web_server_rev_proxy_1_0_0_aarch64.eap`
  - `web_server_rev_proxy_1_0_0_armv7hf.eap`
- Click `Install`
- Run the application by enabling the `Start` switch

#### The expected output

A user can make a HTTP request to the application API using e.g. `curl`

```sh
curl -u<USER>:<PASSWORD> --anyauth http://<AXIS_DEVICE_IP>/local/web_server_rev_proxy/my_web_server
<html><body><h1>ACAP application with reverse proxy web server</h1><pre><br>Application name:      web_server_rev_proxy<br>Reverse proxy path:    /local/web_server_rev_proxy/my_web_server<br>Request timestamp:     Fri Jan 24 14:54:47 2025
</pre></body></html>
```

As can be seen it's HTML code, browse to web page
`http://<AXIS_DEVICE_IP>/local/web_server_rev_proxy/my_web_server`
for seeing it rendered.

The application log can be found by either

- Browse to `http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=web_server_rev_proxy`.
- Browse to the application page and click the `App log`.

## License

**[Apache License 2.0](../LICENSE)**
