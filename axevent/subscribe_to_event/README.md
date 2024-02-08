*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP application subscribing to an ONVIF event on an edge device

This README file explains how to build an ACAP application that uses axevent library for subscribing to an ONVIF event.

It is achieved by using the containerized API and toolchain images.

Together with this README file you should be able to find a directory called app.
That directory contains the "subscribe_to_event" application source code, which can easily
be compiled and run with the help of the tools and step by step below.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
subscribe_to_event
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── subscribe_to_event.c
├── Dockerfile
└── README.md
```

- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application "subscribe_to_event".
- **app/manifest.json** - Defines the application and its configuration.
- **app/subscribe_to_event.c** - Application which subscribes for event, written in C.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example "subscribe_to_event".
- **README.md** - Step by step instructions on how to run the example.

### How to run the code

Below is the step by step instructions on how to execute the program. So basically starting with the generation of the .eap files to running it on a device:

#### Build the application

Standing in your working directory run the following commands:

> **Note**
>
> Depending on the network you are connected to, you may need to add proxy settings.
> The file that needs these settings is: `~/.docker/config.json`. For reference please see
> https://docs.docker.com/network/proxy and a
> [script for Axis devices](https://axiscommunications.github.io/acap-documentation/docs/develop/build-install-run.html#configure-network-proxy-settings) in the ACAP documentation.

```sh
docker build --tag <APP_IMAGE> .
```

<APP_IMAGE> is the name to tag the image with, e.g., subscribe-to-event:1.0

Default architecture is **armv7hf**. To build for **aarch64** it's possible to
update the *ARCH* variable in the Dockerfile or to set it in the docker build
command via build argument:

```sh
docker build --build-arg ARCH=aarch64 --tag <APP_IMAGE> .
```

Copy the result from the container image to a local directory build:

```sh
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

The working dir now contains a build folder with the following files:

```sh
subscribe_to_event
├── app
│   ├── LICENSE
│   ├── Makefile
│   └── subscribe_to_event.c
├── build
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   ├── param.conf
│   ├── subscribe_to_event*
│   ├── subscribe_to_event_1_0_0_armv7hf.eap
│   ├── subscribe_to_event_1_0_0_LICENSE.txt
│   └── subscribe_to_event.c
├── Dockerfile
└── README.md
```

- **build/manifest.json** - Defines the application and its configuration.
- **build/package.conf** - Defines the application and its configuration.
- **build/package.conf.orig** - Defines the application and its configuration, original file.
- **build/param.conf** - File containing application parameters.
- **build/subscribe_to_event*** - Application executable binary file.
- **build/subscribe_to_event_1_0_0_armv7hf.eap** - Application package .eap file for "subscribe_to_event".
- **build/subscribe_to_event_1_0_0_LICENSE.txt** - Copy of LICENSE file.

#### Install your application

Installing your application on an Axis video device is as simple as:

Browse to the following page (replace <axis_device_ip> with the IP number of your Axis video device)

```sh
http://<axis_device_ip>/#settings/apps
```

*Goto your device web page above > Click on the tab **App** in the device GUI > Click Add **(+)** sign and browse to
the newly built **subscribe_to_event_1_0_0_armv7hf.eap** > Click **Install** > Run the application by enabling the **Start** switch*

Application is now available as an application on the device and has been started to subscribe for events.

> [!IMPORTANT]
> See instructions in [README](../send_event/README.md) for Send Events application to get an **send_event_1_0_0_armv7hf.eap** file.

*Goto your device web page above > Click on the tab **App** in the device GUI > Click Add **(+)** sign and browse to
the newly built **send_event_1_0_0_armv7hf.eap** > Click **Install** > Run the application by enabling the **Start** switch*

Application is now available as an application on the device and has been started to send events.

#### The expected output

Application logs can be found directly at:

```sh
http://<axis_device_ip>/axis-cgi/admin/systemlog.cgi?appname=send_event
```

or

```sh
http://<axis_device_ip>/axis-cgi/admin/systemlog.cgi?appname=subscribe_to_event
```

or by clicking on the "**App log**" link in the device GUI or by extracting the logs using following commands
in the terminal.
> [!IMPORTANT]
*> Please make sure SSH is enabled on the device to run the
following commands.*

```sh
ssh root@<axis_device_ip>
cd /var/log/
head -200 info.log
```

##### Output

```sh
16:23:51.242 [ INFO ] subscribe_to_event[0]: starting subscribe_to_event
16:23:51.280 [ INFO ] subscribe_to_event[20408]: Started logging from subscribe event application
16:23:51.281 [ INFO ] subscribe_to_event[20408]: And here's the token: 1234
16:23:56.628 [ INFO ] send_event[0]: starting send_event
16:23:56.670 [ INFO ] send_event[20562]: Started logging from send event application
16:23:56.783 [ INFO ] send_event[20562]: Declaration complete for : 1
16:23:56.792 [ INFO ] subscribe_to_event[20408]: Received event with value: 0.000000
16:24:06.956 [ INFO ] send_event[20562]: Send stateful event with value: 0.000000
16:24:06.964 [ INFO ] subscribe_to_event[20408]: Received event with value: 0.000000
16:24:16.956 [ INFO ] send_event[20562]: Send stateful event with value: 10.000000
16:24:16.963 [ INFO ] subscribe_to_event[20408]: Received event with value: 10.000000
```

A stateful event will be sent every 10th second, changing its value. Stateful events are sent when a property's state changes.
They are also sent immediately when a consumer registers a new subscription. This is done to notify the consumer of
the initial state of the property. This is the reason why there is one log stating that "Received event with value: 0.000000" before the first
log stating "Send stateful event with value: 0.000000".

## License

**[Apache License 2.0](../../LICENSE)**
