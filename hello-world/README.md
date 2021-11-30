*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A hello-world ACAP application using manifest

This README file explains how to build a simple Hello World manifest ACAP application. It is achieved by using the containerized API and toolchain images.

Together with this README file, you should be able to find a directory called app. That directory contains the "hello-world" application source code which can easily be compiled and run with the help of the tools and step by step below.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```bash
hello-world
├── app
│   ├── hello_world.c
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── Dockerfile
└── README.md
```

* **app/hello_world.c** - Hello World application which writes to system-log.
* **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
* **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
* **app/manifest.json** - Defines the application and its configuration.
* **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
* **README.md** - Step by step instructions on how to run the example.

### How to run the code

Below is the step by step instructions on how to execute the program. So basically starting with the generation of the .eap file to running it on a device:

#### Build the application

Standing in your working directory run the following commands:

> [!IMPORTANT]
> *Depending on the network you are connected to,
The file that needs those settings is:* ~/.docker/config.json. *For
reference please see: <https://docs.docker.com/network/proxy/> and a
[script for Axis device here](../FAQs.md#HowcanIset-upnetworkproxysettingsontheAxisdevice?).*

```bash
docker build --tag <APP_IMAGE> .
```

<APP_IMAGE> is the name to tag the image with, e.g., hello_world:1.0

Default architecture is **armv7hf**. To build for **aarch64** it's possible to
update the *ARCH* variable in the Dockerfile or to set it in the docker build
command via build argument:

```bash
docker build --build-arg ARCH=aarch64 --tag <APP_IMAGE> .
```

Copy the result from the container image to a local directory build:

```bash
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

The working dir now contains a build folder with the following files:

```bash
hello-world
├── app
│   ├── hello_world.c
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── build
│   ├── hello_world*
│   ├── hello_world_1_0_0_armv7hf.eap
│   ├── hello_world_1_0_0_LICENSE.txt
│   ├── hello_world.c
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   └── param.conf
├── Dockerfile
└── README.md
```

* **build/hello_world*** - Application executable binary file.
* **build/hello_world_1_0_0_armv7hf.eap** - Application package .eap file.
* **build/hello_world_1_0_0_LICENSE.txt** - Copy of LICENSE file.
* **build/manifest.json** - Defines the application and its configuration.
* **build/package.conf** - Defines the application and its configuration.
* **build/package.conf.orig** - Defines the application and its configuration, original file.
* **build/param.conf** - File containing application parameters.

#### Install your application

Installing your application on an Axis video device is as simple as:

Browse to the following page (replace <axis_device_ip> with the IP number of your Axis video device)

```bash
http://<axis_device_ip>/#settings/apps
```

*Goto your device web page above > Click on the tab **App** in the device GUI > Add **(+)** sign and browse to
the newly built **hello_world_1_0_0_armv7hf.eap** > Click **Install** > Run the application by enabling the **Start** switch*

#### The expected output

Application log can be found directly at:

```sh
http://<axis_device_ip>/axis-cgi/admin/systemlog.cgi?appname=hello_world
```

or by clicking on the "**App log**" link in the device GUI or by extracting the logs using following commands in the terminal.

>[!IMPORTANT]
*> Please make sure SSH is enabled on the device to run the following commands.*

```bash
tail -f /var/log/info.log | grep hello_world
```

```sh
----- Contents of SYSTEM_LOG for 'hello_world' -----

14:13:07.412 [ INFO ] hello_world[6425]: Hello World!

```

## License

**[Apache License 2.0](../LICENSE)**
