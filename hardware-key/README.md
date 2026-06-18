*Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# An ACAP that makes use of the new Certificate1 API to use a hardware backed private key

This README file explains how to build a ACAP that generates a signed CMS file that contains the contents of /proc/cpuinfo. It is achieved by using the containerized API and toolchain images.

Together with this README file, you should be able to find a directory called app. That directory contains the "trustreport" application source code which can easily be compiled and run with the help of the tools and step by step below.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
hello-world
├── app
│   ├── trustreport.c
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── Dockerfile
└── README.md
```

- **app/trustreport.c** - Hello World application which writes to system-log.
- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json** - Defines the application and its configuration.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.

### Hardware Requirements

This ACAP can only be used on Axis devices running 11.11 firmware or later.

### How to run the code

Below is the step by step instructions on how to execute the program. So basically starting with the generation of the .eap file to running it on a device:

#### Build the application

Standing in your working directory run the following commands:

> [!NOTE]
>
> Depending on the network your local build machine is connected to, you may need to add proxy
> settings for Docker. See
> [Proxy in build time](https://developer.axis.com/acap/develop/proxy/#proxy-in-build-time).

```sh
docker build --tag <APP_IMAGE> .
```

<APP_IMAGE> is the name to tag the image with, e.g., trustreport:1.0

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
hello-world
├── app
│   ├── trustreport.c
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── build
│   ├── trustreport*
│   ├── trustreport_1_0_0_armv7hf.eap
│   ├── trustreport_1_0_0_LICENSE.txt
│   ├── trustreport.c
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   └── param.conf
├── Dockerfile
└── README.md
```

- **build/trustreport*** - Application executable binary file.
- **build/trustreport_1_0_0_armv7hf.eap** - Application package .eap file.
- **build/trustreport_1_0_0_LICENSE.txt** - Copy of LICENSE file.
- **build/manifest.json** - Defines the application and its configuration.
- **build/package.conf** - Defines the application and its configuration.
- **build/package.conf.orig** - Defines the application and its configuration, original file.
- **build/param.conf** - File containing application parameters.

#### Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

- Click on the tab `Apps` in the device GUI
- Enable `Allow unsigned apps` toggle
- Click `(+ Add app)` button to upload the application file
- Browse to the newly built ACAP application, depending on architecture:
  - `trustreport_1_0_0_aarch64.eap`
  - `trustreport_1_0_0_armv7hf.eap`
- Click `Install`
- Run the application by enabling the `Start` switch

#### How to verify the application

Open the application cgi by either clicing on the Open button in the application list or by browsing to:

```sh
http://<AXIS_DEVICE_IP>/local/trustreport/trustreport.cgi
```

Write down the chosen certificate alias that you wish to use for signing. By
default newer devices contains IDEVID certificates like: "Axis device ID ECC-P256 (802.1AR)"

Download the signed cms package, verify and extract the content:

```sh
curl -o cpuinfo.pem https://172.29.162.153/local/trustreport/trustreport.cgi?action=sign_syslog

openssl cms -in cpuinfo.pem -verify  -CAfile chain.pem -out cpuinfo.txt
cat cpuinfo.txt
.....
```

## License

**[Apache License 2.0](../LICENSE)**
