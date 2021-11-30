 *Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A vdo stream based ACAP application on an edge device

This README file explains how to build an ACAP application that uses the vdostream API. It is achieved by using the containerized API and toolchain images.

Together with this README file, you should be able to find a directory called app. That directory contains the "vdoencodeclient" application source code which can easily
be compiled and run with the help of the tools and step by step below.

This example illustrates how to continuously capture frames from the vdo service, access the received buffer contents as well as the frame metadata. Captured frames are logged in the Application log.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```bash
vdostream
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── vdoencodeclient.c
├── Dockerfile
└── README.md
```

* **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
* **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
* **app/manifest.json** - Defines the application and its configuration.
* **app/vdoencodeclient.c** - Application to capture the frames using vdo service in C.
* **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
* **README.md** - Step by step instructions on how to run the example.

### Limitations

* The example is done for the armv7hf architecture, but it is possible to update to aarch64 architecture.
* Supported video compression formats for an Axis video device are found in the data-sheet of the device.

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

<APP_IMAGE> is the name to tag the image with, e.g., vdoencodeclient:1.0

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
vdostream
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── vdoencodeclient.c
├── build
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   ├── param.conf
│   ├── vdoencodeclient*
│   ├── vdoencodeclient_1_0_0_armv7hf.eap
│   ├── vdoencodeclient_1_0_0_LICENSE.txt
│   └── vdoencodeclient.c
├── Dockerfile
└── README.md
```

* **build/manifest.json** - Defines the application and its configuration.
* **build/package.conf** - Defines the application and its configuration.
* **build/package.conf.orig** - Defines the application and its configuration, original file.
* **build/param.conf** - File containing application parameters.
* **build/vdoencodeclient*** - Application executable binary file.
* **build/vdoencodeclient_1_0_0_armv7hf.eap** - Application package .eap file.
* **build/vdoencodeclient_1_0_0_LICENSE.txt** - Copy of LICENSE file.

#### Install your application

Installing your application on an Axis video device is as simple as:

Browse to the following page (replace <axis_device_ip> with the IP number of your Axis video device)

```bash
http://<axis_device_ip>/#settings/apps
```

*Goto your device web page above > Click on the tab **App** in the device GUI > Add **(+)** sign and browse to
the newly built **vdoencodeclient_1_0_0_armv7hf.eap** > Click **Install** > Run the application by enabling the **Start** switch*

Application will run with default video compression format h264.

#### The expected output

Application log can be found directly at:

```sh
http://<axis_device_ip>/axis-cgi/admin/systemlog.cgi?appname=vdoencodeclient
```

or by clicking on the "**App log**" link in the device GUI or by extracting the logs using following commands
in the terminal.
> [!IMPORTANT]
*> Please make sure SSH is enabled on the device to run the
following commands.*

```sh
ssh root@<axis_device_ip>
cd /var/log/
head -50 info.log
```

```sh
----- Contents of SYSTEM_LOG for 'vdoencodeclient' -----

11:00:03.509 [ INFO ] vdoencodeclient[17690]: frame = 35729, type = P, size = 81
11:00:03.532 [ INFO ] vdoencodeclient[17690]: frame = 35730, type = I, size = 78433
11:00:03.572 [ INFO ] vdoencodeclient[17690]: frame = 35731, type = P, size = 567
11:00:03.605 [ INFO ] vdoencodeclient[17690]: frame = 35732, type = P, size = 82
11:00:03.638 [ INFO ] vdoencodeclient[17690]: frame = 35733, type = P, size = 74
11:00:03.672 [ INFO ] vdoencodeclient[17690]: frame = 35734, type = P, size = 450
11:00:03.706 [ INFO ] vdoencodeclient[17690]: frame = 35735, type = P, size = 111
11:00:03.738 [ INFO ] vdoencodeclient[17690]: frame = 35736, type = P, size = 76
11:00:03.772 [ INFO ] vdoencodeclient[17690]: frame = 35737, type = P, size = 74
11:00:03.805 [ INFO ] vdoencodeclient[17690]: frame = 35738, type = P, size = 78
11:00:03.838 [ INFO ] vdoencodeclient[17690]: frame = 35739, type = P, size = 78
11:00:03.872 [ INFO ] vdoencodeclient[17690]: frame = 35740, type = P, size = 86
11:00:03.905 [ INFO ] vdoencodeclient[17690]: frame = 35741, type = P, size = 79
11:00:03.938 [ INFO ] vdoencodeclient[17690]: frame = 35742, type = P, size = 78
11:00:03.972 [ INFO ] vdoencodeclient[17690]: frame = 35743, type = P, size = 77
11:00:04.005 [ INFO ] vdoencodeclient[17690]: frame = 35744, type = P, size = 71
11:00:04.038 [ INFO ] vdoencodeclient[17690]: frame = 35745, type = P, size = 82
11:00:04.072 [ INFO ] vdoencodeclient[17690]: frame = 35746, type = P, size = 75
11:00:04.105 [ INFO ] vdoencodeclient[17690]: frame = 35747, type = P, size = 77
11:00:04.138 [ INFO ] vdoencodeclient[17690]: frame = 35748, type = P, size = 80
```

## License

**[Apache License 2.0](../LICENSE)**
