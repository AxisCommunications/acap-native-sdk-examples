 *Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A guide to building and running a custom library in an ACAP application

This README file explains how to build a user defined custom library from source files and bundle it for use in an ACAP. The example application uses the custom library and prints "Hello World!" to the syslog of the device.

Together with this README file, you should be able to find a directory called app. That directory contains the custom_lib_example application source code which can easily
be compiled and run with the help of the tools.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
custom_lib_example
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── customlib_example.c
│   └── custom_build
├── Dockerfile
└── README.md
```

- **app/LICENSE**             - File containing the license conditions.
- **app/Makefile**            - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json**       - Defines the application and its configuration.
- **app/customlib_example.c** - Example application.
- **build/custom_build**      - Folder containing custom library source files
- **Dockerfile**              - Docker file with the specified Axis toolchain and API container to build the example specified.
- **README.md**               - Step by step instructions on how to run the example.

### How to run the code

Below is the step by step instructions on how to execute the program. So basically starting with the generation of the .eap file to running it on a device:

#### Build the application

Standing in your working directory run the following commands:

> **Note**
>
> Depending on the network you are connected to, you may need to add proxy settings. you may need to add proxy settings.
The file that needs those settings is:* ~/.docker/config.json. *For
reference please see: https://docs.docker.com/network/proxy/ and a
[script for Axis device here](../../FAQs.md#HowcanIset-upnetworkproxysettingsontheAxisdevice?).*

```sh
docker build --tag <APP_IMAGE> .
```

<APP_IMAGE> is the name to tag the image with, e.g., mainfunc:1.0

The default architecture is **armv7hf**. To build for **aarch64** it's possible to
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
├── build
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   ├── param.conf
│   ├── customlib_example*
│   ├── lib
│   │   ├── libcustom.so@
│   │   ├── libcustom.so.1@
│   │   └── libcustom.so.1.0.o*
│   ├── custom_build
│   ├── customlib_example_1_0_0_armv7hf.eap
│   ├── customlib_example_1_0_0_LICENSE.txt
│   └── customlib_example.c

```

- **build/package.conf** - Defines the application and its configuration.
- **build/package.conf.orig** - Defines the application and its configuration, original file.
- **build/param.conf** - File containing application parameters.
- **build/customlib_example*** - Application executable binary file.
- **build/lib** - Folder containing compiled library files for custom library
- **build/customlib_example_1_0_0_armv7hf.eap** - Application package .eap file.
- **build/customlib_example_1_0_0_LICENSE.txt** - Copy of LICENSE file.

#### Install your application

Installing your application on an Axis device is as simple as:

Browse to the following page (replace <axis_device_ip> with the IP number of your Axis device)

```sh
http://<axis_device_ip>/#settings/apps
```

*Go to your device web page above > Click on the tab **App** in the device GUI > Click Add **(+)** sign and browse to
the newly built **customlib_example_1_0_0_armv7hf.eap** > Click **Install** > Run the application by enabling the **Start** switch*

Application custom_lib_example is now available as an application on the device.

### Custom library application

The application will use a user-defined custom library and print "Hello World!" to the syslog of the device.

#### The expected output

The application log can be found at:

```sh
http://<axis_device_ip>/axis-cgi/admin/systemlog.cgi?appname=mainfunc
```

or by clicking on the "**App log**" link in the device GUI or by extracting the logs using following commands in the terminal.

>[!IMPORTANT]
*> Please make sure SSH is enabled on the device to run the following commands.*

```sh
tail -f /var/log/info.log | grep mainfunc
```

```sh
----- Contents of SYSTEM_LOG for 'mainfunc' -----

2021-08-19T14:22:28.068+02:00 axis-accc8ef26bcf [ INFO    ] customlib_example[32561]: Hello World!

```

## License

**[Apache License 2.0](../../LICENSE)**
