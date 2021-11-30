*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A licensekey handler based ACAP application on an edge device

This README file explains how to build an ACAP application that uses the licensekey API. It is achieved by using the containerized API and toolchain images.

Together with this README file, you should be able to find a directory called app. That directory contains the application source code which can easily be compiled and run with the help of the tools and step by step below.

This example illustrates how to check the license key status. A license key is a signed file which has been generated for a specific device ID and application ID. The ACAP Service Portal is maintaining both license keys and application IDs, see [Online manual](https://help.axis.com/acap-3-developer-guide#acap-service-portal-for-administrators)

License key status i.e. valid or invalid is logged in the Application log.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```bash
licensekey
├── app
│   ├── LICENSE
│   ├── licensekey_handler.c
│   ├── Makefile
│   └── manifest.json
├── Dockerfile
└── README.md
```

* **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
* **app/licensekey_handler.c** - Application to check licensekey status in C.
* **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
* **app/manifest.json** - Defines the application and its configuration.
* **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
* **README.md** - Step by step instructions on how to run the example.

### Limitations

* The example is done for the armv7hf architecture, but it is possible to update to aarch64 architecture.

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

<APP_IMAGE> is the name to tag the image with, e.g., licensekey-handler:1.0

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
licensekey
├── app
│   ├── LICENSE
│   ├── licensekey_handler.c
│   ├── Makefile
│   └── manifest.json
├── build
│   ├── LICENSE
│   ├── licensekey_handler*
│   ├── licensekey_handler_1_0_0_armv7hf.eap
│   ├── licensekey_handler_1_0_0_LICENSE.txt
│   ├── licensekey_handler.c
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   └── param.conf
├── Dockerfile
└── README.md
```

* **build/licensekey_handler*** - Application executable binary file.
* **build/licensekey_handler_1_0_0_armv7hf.eap** - Application package .eap file.
* **build/licensekey_handler_1_0_0_LICENSE.txt** - Copy of LICENSE file.
* **build/manifest.json** - Defines the application and its configuration.
* **build/package.conf.orig** - Defines the application and its configuration, original file.
* **build/param.conf** - File containing application parameters.

#### Install your application

Installing your application on an Axis video device is as simple as:

Browse to the following page (replace <axis_device_ip> with the IP number of your Axis video device)

```bash
http://<axis_device_ip>/#settings/apps
```

*Goto your device web page above > Click on the tab **Apps** in the device GUI > Add **(+)** sign and browse to
the newly built **licensekey_handler_1_0_0_armv7hf.eap** > Click **Install** > Run the application by enabling the **Start** switch*

#### The expected output

Application log can be found directly at:

```sh
http://<axis_device_ip>/axis-cgi/admin/systemlog.cgi?appname=licensekey_handler
```

or by clicking on the "**App log**" link in the device GUI.

```sh
----- Contents of SYSTEM_LOG for 'licensekey_handler' -----


10:26:42.499 [ INFO ] licensekey_handler[0]: starting licensekey_handler
10:26:42.539 [ INFO ] licensekey_handler[14660]: Licensekey is invalid
10:31:43.058 [ INFO ] licensekey_handler[14660]: Licensekey is invalid
```

A valid license key for a registered application ID is only accessible through ACAP Service Portal, see [Online manual](https://help.axis.com/acap-3-developer-guide#acap-service-portal-for-administrators).

Support for installing license key though device web page is available, if acapPackageConf.copyProtection.method is set to "axis" in the **manifest.json** file, by the following steps:

*Goto your device web page above > Click on the tab **Apps** in the device GUI > Click on the installed **licensekey_handler** application > Install the license with the **Install** button in the **Activate the license** part*

More instructions how to install a valid license key is found on [Axis Developer Community](https://help.axis.com/acap-3-developer-guide#developer-community).

## License

**[Apache License 2.0](../LICENSE)**
