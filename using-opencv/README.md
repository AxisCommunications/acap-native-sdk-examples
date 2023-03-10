*Copyright (C) 2022, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A guide to building and running OpenCV in an ACAP application

This guide explains how to build OpenCV from source and bundle it for use in an
ACAP application. The example application runs a background subtraction
operation on video from the camera for a very simple motion detection
application to demonstrate how to integrate the broad functionality of OpenCV.

## File structure

```sh
building-opencv
├── app
│   ├── example.cpp - The application running OpenCV code
│   ├── imgprovider.cpp - Convenience functions for VDO
│   ├── imgprovider.h - imgprovider headers
│   ├── LICENSE
│   ├── Makefile - The Makefile specifying how the ACAP should be built
│   └── manifest.json - A file specifying execution-related options for the ACAP
├── Dockerfile - Specification of the container used to build the ACAP
├── README.md
└── sources.list - Text file specifying repositories for armhf packages
```

## Instructions

### Quick start

> **Note**
>
> Depending on the network you are connected to, you may need to add proxy settings.
> The file that needs these settings is: `~/.docker/config.json`. For reference please see
> https://docs.docker.com/network/proxy and a
> [script for Axis devices](https://axiscommunications.github.io/acap-documentation/docs/develop/build-install-run.html#configure-network-proxy-settings) in the ACAP documentation.

1. Standing in your working directory run the following commands:

   On armv7hf architecture

   ```sh
   docker build --tag <APP_IMAGE> .
   ```

   On aarch64 architecture

   ```sh
   docker build --tag <APP_IMAGE> --build-arg ARCH=aarch64 .
   ```

   <APP_IMAGE> is the name to tag the image with, e.g., opencv-app:1.0

   Copy the result from the container image to a local directory build:

   ```sh
   docker cp $(docker create <APP_IMAGE>):/opt/app ./build
   ```

2. You should now have a `build` directory. In it is the `.eap` file that is
   your application.  Upload the application to your camera.
3. Start the application. In the `App log`, a printout from the application
   should be seen. The same log with continuous scroll can be seen by SSHing to
   the camera and running `journalctl -f`. The printout shows whether the
   application has detected movement in the image or not:

   ```sh
   opencv_app[0]: starting opencv_app
   opencv_app[2211]: Running OpenCV example with VDO as video source
   opencv_app[2211]: Creating VDO image provider and creating stream 1024 x 576
   opencv_app[2211]: Dump of vdo stream settings map =====
   opencv_app[2211]: chooseStreamResolution: We select stream w/h=1024 x 576 based on VDO channel info.
   opencv_app[2211]: Start fetching video frames from VDO
   opencv_app[2211]: Motion detected: YES
   opencv_app[2211]: Motion detected: YES
   ```

### Walk-through of application

The [Dockerfile](Dockerfile) suits as a good overview of the build process;

1. First the OpenCV libraries are built with the help of CMake and the ACAP SDK
   libraries, especially `libc` and `libstdc++`.
2. The OpenCV libraries are then copied to the application directory under
   `lib`.
3. Finally the ACAP application is built with the build instructions in the
   [Makefile](app/Makefile) where it links to and bundles the OpenCV libraries
to the application.

#### Building OpenCV libraries

OpenCV libraries are built with CMake and some special steps are made to get
a correct build:

- The ACAP SDK is sourced to get cross compilation variables like `CC` and
  `CXX` which contains the path to the SDK libraries.
- To get correct cross compilation settings, `CMAKE_TOOLCHAIN_FILE` is set to the
  architecture specific file provided by OpenCV.
- CMake picks up environment variables like `CC` and `CXX` but seems to not
  work in combination with `CMAKE_TOOLCHAIN_FILE`. To get CMake to pick up the
cross compiler and SDK library path, these variables are split and set
explicitly in `CMAKE_{C,CXX}_COMPILER` and `CMAKE_{C,CXX}_FLAGS` respectively.

Other noteworthy options are the ones related to `NEON` and `VFPV3`, which are
optimizations available for the platform that can greatly speed up CPU
operations. This is only necessary for armv7hf, in aarch64 these options are
implicitly present.

The other configuration options are there to make the OpenCV installation quite
stripped of functionality not needed for the example application. However, you
will likely have to change these options to accommodate your custom
application.

#### Building ACAP application

The build instructions of the ACAP application are found in the
[Makefile](app/Makefile).  Note the use of the option
`-Wl,--no-as-needed,-rpath,'$$ORIGIN/lib'` which will set the *runtime share
library search path* and is where the bundled libraries will be found when the
application is installed on a device.

#### ACAP application using OpenCV

The example application source code using OpenCV can be seen in
[example.cpp](app/example.cpp). It is a C++ application which uses the OpenCV
MOG2 background subtraction and noise filtering to detect changes in the image
in order to perform motion detection.

The code is documented to give a clear understanding of what steps are needed
to grab frames from the camera and perform operations on them.

The output of the application can be seen through the `App log` or by running
`journalctl -f` while connected through SSH to the device.

## License

**[Apache License 2.0](../LICENSE)**

## References

- <https://docs.opencv.org>
