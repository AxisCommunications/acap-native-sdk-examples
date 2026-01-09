*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A vdo stream based ACAP application on an edge device

This README file explains how to build an ACAP application that uses the vdostream API. It is achieved by using the containerized API and toolchain images.

Together with this README file, you should be able to find a directory called app. That directory contains the "vdoencodeclient" application source code which can easily
be compiled and run with the help of the tools and step by step below.

This example illustrates how to continuously capture frames from the vdo service, access the received buffer contents as well as the frame metadata. Captured frames are logged in the Application log.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
vdostream
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json.h264
│   ├── manifest.json.h265
│   ├── manifest.json.jpeg
│   ├── manifest.json.nv12
│   ├── manifest.json.y800
│   └── vdoencodeclient.c
├── Dockerfile
└── README.md
```

- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json** - Defines the application and its configuration.
- **app/vdoencodeclient.c** - Application to capture the frames using vdo service in C.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.

### Limitations

- Supported video compression formats for an Axis video device are found in the
  data-sheet of the device.

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
docker build --platform=linux/amd64 --tag <APP_IMAGE> --build-arg VDO_FORMAT=<VDO_FORMAT> .
```

<!-- textlint-disable terminology -->
<VDO_FORMAT> is the video compression format. Supported values are *h264*, *h265*, *jpeg*, *nv12* and *y800*
<!-- textlint-enable -->

<APP_IMAGE> is the name to tag the image with, e.g., vdoencodeclient:1.0

Default architecture is **armv7hf**. To build for **aarch64** it's possible to
update the *ARCH* variable in the Dockerfile or to set it in the `docker build`
command via build argument:

```sh
docker build --platform=linux/amd64 --build-arg ARCH=aarch64 --build-arg VDO_FORMAT=<VDO_FORMAT> --tag <APP_IMAGE> .
```

Copy the result from the container image to a local directory build:

```sh
docker cp $(docker create --platform=linux/amd64 <APP_IMAGE>):/opt/app ./build
```

The working dir now contains a build folder with the following files:

```sh
vdostream
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json.h264
│   ├── manifest.json.h265
│   ├── manifest.json.jpeg
│   ├── manifest.json.nv12
│   ├── manifest.json.y800
│   └── vdoencodeclient.c
├── build
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── manifest.json.h264
│   ├── manifest.json.h265
│   ├── manifest.json.jpeg
│   ├── manifest.json.nv12
│   ├── manifest.json.y800
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

- **build/manifest.json** - Defines the application and its configuration.
- **build/package.conf** - Defines the application and its configuration.
- **build/package.conf.orig** - Defines the application and its configuration, original file.
- **build/param.conf** - File containing application parameters.
- **build/vdoencodeclient*** - Application executable binary file.
- **build/vdoencodeclient_1_0_0_armv7hf.eap** - Application package .eap file.
- **build/vdoencodeclient_1_0_0_LICENSE.txt** - Copy of LICENSE file.

> [!NOTE]
>
> For detailed information on how to build, install, and run ACAP applications, refer to the official ACAP documentation: [Build, install, and run](https://developer.axis.com/acap/develop/build-install-run/).

#### Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

1. Click on the tab **Apps** in the device GUI
2. Enable **Allow unsigned apps** toggle
3. Click **(+ Add app)** button to upload the application file
4. Select the newly built application package, depending on architecture:

    - `vdoencodeclient_1_0_0_aarch64.eap`
    - `vdoencodeclient_1_0_0_armv7hf.eap`

5. Click **Install**
6. Run the application by enabling the **Start** switch

#### The expected output

Application log can be found directly at:

```sh
http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=vdoencodeclient
```

#### Output - format h264

```sh
----- Contents of SYSTEM_LOG for 'vdoencodeclient' -----

vdoencodeclient[49013]: Starting stream: h264, 640x360, 30 fps
vdoencodeclient[49013]: frame =    0, type = I, size = 2700
vdoencodeclient[49013]: frame =    1, type = P, size = 19
vdoencodeclient[49013]: frame =    2, type = P, size = 19
vdoencodeclient[49013]: frame =    3, type = P, size = 19
vdoencodeclient[49013]: frame =    4, type = P, size = 19
vdoencodeclient[49013]: frame =    5, type = P, size = 19
vdoencodeclient[49013]: frame =    6, type = P, size = 19
vdoencodeclient[49013]: frame =    7, type = P, size = 19
vdoencodeclient[49013]: frame =    8, type = P, size = 19
vdoencodeclient[49013]: frame =    9, type = P, size = 19
vdoencodeclient[49013]: frame =   10, type = P, size = 19
vdoencodeclient[49013]: frame =   11, type = P, size = 19
vdoencodeclient[49013]: frame =   12, type = P, size = 19
vdoencodeclient[49013]: frame =   13, type = P, size = 19
vdoencodeclient[49013]: frame =   14, type = P, size = 19
vdoencodeclient[49013]: frame =   15, type = P, size = 19
vdoencodeclient[49013]: frame =   16, type = P, size = 19
vdoencodeclient[49013]: frame =   17, type = P, size = 19
vdoencodeclient[49013]: frame =   18, type = P, size = 19
vdoencodeclient[49013]: frame =   19, type = P, size = 19
vdoencodeclient[49013]: frame =   20, type = P, size = 19
vdoencodeclient[49013]: frame =   21, type = P, size = 19
vdoencodeclient[49013]: frame =   22, type = P, size = 19
vdoencodeclient[49013]: frame =   23, type = P, size = 19
vdoencodeclient[49013]: frame =   24, type = P, size = 19
```

#### Output - format h265

```sh
----- Contents of SYSTEM_LOG for 'vdoencodeclient' -----

vdoencodeclient[29828]: Starting stream: h265, 640x360, 30 fps
vdoencodeclient[29828]: frame =    0, type = I, size = 1404
vdoencodeclient[29828]: frame =    1, type = P, size = 30
vdoencodeclient[29828]: frame =    2, type = P, size = 31
vdoencodeclient[29828]: frame =    3, type = P, size = 30
vdoencodeclient[29828]: frame =    4, type = P, size = 33
vdoencodeclient[29828]: frame =    5, type = P, size = 43
vdoencodeclient[29828]: frame =    6, type = P, size = 42
vdoencodeclient[29828]: frame =    7, type = P, size = 37
vdoencodeclient[29828]: frame =    8, type = P, size = 40
vdoencodeclient[29828]: frame =    9, type = P, size = 33
vdoencodeclient[29828]: frame =   10, type = P, size = 33
vdoencodeclient[29828]: frame =   11, type = P, size = 33
vdoencodeclient[29828]: frame =   12, type = P, size = 39
vdoencodeclient[29828]: frame =   13, type = P, size = 65
vdoencodeclient[29828]: frame =   14, type = P, size = 104
vdoencodeclient[29828]: frame =   15, type = P, size = 36
vdoencodeclient[29828]: frame =   16, type = P, size = 168
vdoencodeclient[29828]: frame =   17, type = P, size = 39
vdoencodeclient[29828]: frame =   18, type = P, size = 30
vdoencodeclient[29828]: frame =   19, type = P, size = 30
vdoencodeclient[29828]: frame =   20, type = P, size = 30
vdoencodeclient[29828]: frame =   21, type = P, size = 30
vdoencodeclient[29828]: frame =   22, type = P, size = 30
vdoencodeclient[29828]: frame =   23, type = P, size = 30
vdoencodeclient[29828]: frame =   24, type = P, size = 30
 ```

<!-- textlint-disable terminology -->
#### Output - format jpeg
<!-- textlint-enable -->

```sh
----- Contents of SYSTEM_LOG for 'vdoencodeclient' -----

vdoencodeclient[33823]: Starting stream: jpeg, 640x360, 30 fps
vdoencodeclient[33823]: frame =    0, type = jpeg, size = 7802
vdoencodeclient[33823]: frame =    1, type = jpeg, size = 7797
vdoencodeclient[33823]: frame =    2, type = jpeg, size = 7807
vdoencodeclient[33823]: frame =    3, type = jpeg, size = 7797
vdoencodeclient[33823]: frame =    4, type = jpeg, size = 7811
vdoencodeclient[33823]: frame =    5, type = jpeg, size = 7811
vdoencodeclient[33823]: frame =    6, type = jpeg, size = 7815
vdoencodeclient[33823]: frame =    7, type = jpeg, size = 7811
vdoencodeclient[33823]: frame =    8, type = jpeg, size = 7809
vdoencodeclient[33823]: frame =    9, type = jpeg, size = 7813
vdoencodeclient[33823]: frame =   10, type = jpeg, size = 7812
vdoencodeclient[33823]: frame =   11, type = jpeg, size = 7807
vdoencodeclient[33823]: frame =   12, type = jpeg, size = 7811
vdoencodeclient[33823]: frame =   13, type = jpeg, size = 7803
vdoencodeclient[33823]: frame =   14, type = jpeg, size = 7807
vdoencodeclient[33823]: frame =   15, type = jpeg, size = 7808
vdoencodeclient[33823]: frame =   16, type = jpeg, size = 7807
vdoencodeclient[33823]: frame =   17, type = jpeg, size = 7809
vdoencodeclient[33823]: frame =   18, type = jpeg, size = 7803
vdoencodeclient[33823]: frame =   19, type = jpeg, size = 7800
vdoencodeclient[33823]: frame =   20, type = jpeg, size = 7805
vdoencodeclient[33823]: frame =   21, type = jpeg, size = 7803
vdoencodeclient[33823]: frame =   22, type = jpeg, size = 7809
vdoencodeclient[33823]: frame =   23, type = jpeg, size = 7815
vdoencodeclient[33823]: frame =   24, type = jpeg, size = 7813
```

#### Output - format nv12

```sh
----- Contents of SYSTEM_LOG for 'vdoencodeclient' -----

vdoencodeclient[31151]: Starting stream: nv12, 640x360, 30 fps
vdoencodeclient[31151]: frame =    0, type = yuv, size = 345600
vdoencodeclient[31151]: frame =    1, type = yuv, size = 345600
vdoencodeclient[31151]: frame =    2, type = yuv, size = 345600
vdoencodeclient[31151]: frame =    3, type = yuv, size = 345600
vdoencodeclient[31151]: frame =    4, type = yuv, size = 345600
vdoencodeclient[31151]: frame =    5, type = yuv, size = 345600
vdoencodeclient[31151]: frame =    6, type = yuv, size = 345600
vdoencodeclient[31151]: frame =    7, type = yuv, size = 345600
vdoencodeclient[31151]: frame =    8, type = yuv, size = 345600
vdoencodeclient[31151]: frame =    9, type = yuv, size = 345600
vdoencodeclient[31151]: frame =   10, type = yuv, size = 345600
vdoencodeclient[31151]: frame =   11, type = yuv, size = 345600
vdoencodeclient[31151]: frame =   12, type = yuv, size = 345600
vdoencodeclient[31151]: frame =   13, type = yuv, size = 345600
vdoencodeclient[31151]: frame =   14, type = yuv, size = 345600
vdoencodeclient[31151]: frame =   15, type = yuv, size = 345600
vdoencodeclient[31151]: frame =   16, type = yuv, size = 345600
vdoencodeclient[31151]: frame =   17, type = yuv, size = 345600
vdoencodeclient[31151]: frame =   18, type = yuv, size = 345600
vdoencodeclient[31151]: frame =   19, type = yuv, size = 345600
vdoencodeclient[31151]: frame =   20, type = yuv, size = 345600
vdoencodeclient[31151]: frame =   21, type = yuv, size = 345600
vdoencodeclient[31151]: frame =   22, type = yuv, size = 345600
vdoencodeclient[31151]: frame =   23, type = yuv, size = 345600
vdoencodeclient[31151]: frame =   24, type = yuv, size = 345600
```

#### Output - format y800

```sh
----- Contents of SYSTEM_LOG for 'vdoencodeclient' -----

vdoencodeclient[32479]: Starting stream: y800, 640x360, 30 fps
vdoencodeclient[32479]: frame =    0, type = yuv, size = 230400
vdoencodeclient[32479]: frame =    1, type = yuv, size = 230400
vdoencodeclient[32479]: frame =    2, type = yuv, size = 230400
vdoencodeclient[32479]: frame =    3, type = yuv, size = 230400
vdoencodeclient[32479]: frame =    4, type = yuv, size = 230400
vdoencodeclient[32479]: frame =    5, type = yuv, size = 230400
vdoencodeclient[32479]: frame =    6, type = yuv, size = 230400
vdoencodeclient[32479]: frame =    7, type = yuv, size = 230400
vdoencodeclient[32479]: frame =    8, type = yuv, size = 230400
vdoencodeclient[32479]: frame =    9, type = yuv, size = 230400
vdoencodeclient[32479]: frame =   10, type = yuv, size = 230400
vdoencodeclient[32479]: frame =   11, type = yuv, size = 230400
vdoencodeclient[32479]: frame =   12, type = yuv, size = 230400
vdoencodeclient[32479]: frame =   13, type = yuv, size = 230400
vdoencodeclient[32479]: frame =   14, type = yuv, size = 230400
vdoencodeclient[32479]: frame =   15, type = yuv, size = 230400
vdoencodeclient[32479]: frame =   16, type = yuv, size = 230400
vdoencodeclient[32479]: frame =   17, type = yuv, size = 230400
vdoencodeclient[32479]: frame =   18, type = yuv, size = 230400
vdoencodeclient[32479]: frame =   19, type = yuv, size = 230400
vdoencodeclient[32479]: frame =   20, type = yuv, size = 230400
vdoencodeclient[32479]: frame =   21, type = yuv, size = 230400
vdoencodeclient[32479]: frame =   22, type = yuv, size = 230400
vdoencodeclient[32479]: frame =   23, type = yuv, size = 230400
vdoencodeclient[32479]: frame =   24, type = yuv, size = 230400
```

## License

**[Apache License 2.0](../LICENSE)**
