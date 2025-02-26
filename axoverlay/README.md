*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# An axoverlay based ACAP application on an edge device

This README file explains how to build an ACAP application that uses the axoverlay API. It is achieved by using the containerized API and toolchain images.

Together with this README file, you should be able to find a directory called app. That directory contains the "axoverlay" application source code which can easily be compiled and run with the help of the tools and step by step below.

This example illustrates how to draw overlays in a video stream and Cairo is used as rendering API, see [documentation](https://www.cairographics.org/). In this example two plain boxes in different colors and one overlay text are drawn.

It is preferable to use Palette color space for large overlays like plain boxes, to lower the memory usage.
More detailed overlays like text overlays, should instead use ARGB32 color space.

Different stream resolutions are logged in the Application log.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
axoverlay
├── app
│   ├── axoverlay.c
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── Dockerfile
└── README.md
```

- **app/axoverlay.c** - Application to draw overlays using axoverlay in C.
- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json** - Defines the application and its configuration.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.

### Limitations

- ARTPEC-8, ARTPEC-7 and ARTPEC-6 based devices.
- It is not possible to combine different color spaces for ARTPEC-6.

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
docker build --tag <APP_IMAGE> --build-arg ARCH=<ARCH> .
```

- <APP_IMAGE> is the name to tag the image with, e.g., axoverlay:1.0
- \<ARCH\> is the architecture of the camera you are using, e.g., armv7hf (default) or aarch64

Copy the result from the container image to a local directory build:

```sh
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

The working dir now contains a build folder with the following files:

```sh
axoverlay
├── app
│   ├── axoverlay.c
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── build
│   ├── axoverlay*
│   ├── axoverlay_1_0_0_<ARCH>.eap
│   ├── axoverlay_1_0_0_LICENSE.txt
│   ├── axoverlay.c
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   └── param.conf
├── Dockerfile
└── README.md
```

- **build/axoverlay*** - Application executable binary file.
- **build/axoverlay_1_0_0_\<ARCH\>.eap** - Application package .eap file.
- **build/axoverlay_1_0_0_LICENSE.txt** - Copy of LICENSE file.
- **build/manifest.json** - Defines the application and its configuration.
- **build/package.conf** - Defines the application and its configuration.
- **build/package.conf.orig** - Defines the application and its configuration, original file.
- **build/param.conf** - File containing application parameters.

#### Install your application

Installing your application on an Axis video device is as simple as:

Browse to the following page (replace <AXIS_DEVICE_IP> with the IP number of your Axis video device)

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

- Click on the tab `Apps` in the device GUI
- Enable `Allow unsigned apps` toggle
- Click `(+ Add app)` button to upload the application file
- Browse to the newly built ACAP application, depending on architecture:
  - `axoverlay_1_0_0_aarch64.eap`
  - `axoverlay_1_0_0_armv7hf.eap`
- Click `Install`
- Run the application by enabling the `Start` switch

#### The expected output

Application log can be found directly at:

```sh
http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=axoverlay
```

```sh
----- Contents of SYSTEM_LOG for 'axoverlay' -----

14:13:07.412 [ INFO ] axoverlay[0]: starting axoverlay
14:13:07.661 [ INFO ] axoverlay[2906]: Max resolution (width x height): 1920 x 1080
```

Overlays are shown when the stream has been started:

*Goto your device web page above > Click on the tab **Stream** in the device GUI >  Press the **Play** icon to see the overlays.*

```sh
----- Contents of SYSTEM_LOG for 'axoverlay' -----
14:13:18.819 [ INFO ] axoverlay[2906]: Adjust callback for overlay: 1920 x 1080
14:13:18.819 [ INFO ] axoverlay[2906]: Adjust callback for stream: 1920 x 1080
14:13:19.039 [ INFO ] axoverlay[2906]: Render callback for camera: 1
14:13:19.039 [ INFO ] axoverlay[2906]: Render callback for overlay: 1920 x 1088
14:13:19.039 [ INFO ] axoverlay[2906]: Render callback for stream: 1920 x 1080
```

> [!NOTE]
> *In this case, overlay height has been adjusted to make the height divisible with 16.
> This makes the overlay height (1088) larger than the stream height (1080).*

In this example, an adjustment callback function is used to adapt the plain boxes to the stream resolution. This makes the rendering of the plain boxes correct.
It is possible to update the resolution by:

*Goto your device web page above > Click on the tab **Stream** in the device GUI > Update **Resolution** dropdown menu to **1280x720 (16:9)***

```sh
----- Contents of SYSTEM_LOG for 'axoverlay' -----
14:28:28.112 [ INFO ] axoverlay[2906]: Adjust callback for overlay: 1920 x 1080
14:28:28.112 [ INFO ] axoverlay[2906]: Adjust callback for stream: 1280 x 720
14:28:28.225 [ INFO ] axoverlay[2906]: Render callback for camera: 1
14:28:28.225 [ INFO ] axoverlay[2906]: Render callback for overlay: 1280 x 720
14:28:28.225 [ INFO ] axoverlay[2906]: Render callback for stream: 1280 x 720
```

## License

**[Apache License 2.0](../LICENSE)**
