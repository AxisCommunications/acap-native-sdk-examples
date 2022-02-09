 *Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A guide to building and running OpenCV in an ACAP application

This guide explains how to build OpenCV from source and bundle it for use in an ACAP. The example application
runs a background subtraction operation on video from the camera for a very simple motion detection application
 to demonstrate how to integrate OpenCVs broad functionality into the AXIS Camera Application Platform.

The example is written for the ARTPEC-7 platform, which is built on the armhf architecture. However, with
minor changes to the `Dockerfile`, the code should be usable on any AXIS camera architecture.

## File structure

```bash
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

> [!IMPORTANT]
> *Depending on the network you are connected to,
The file that needs those settings is:* ~/.docker/config.json. *For
reference please see: <https://docs.docker.com/network/proxy/> and a
[script for Axis device here](../FAQs.md#HowcanIset-upnetworkproxysettingsontheAxisdevice?).*

1. Standing in your working directory run the following commands:

   On armv7hf architecture

   ```bash
   docker build --tag <APP_IMAGE> .
   ```

   On aarch64 architecture

   ```bash
   docker build --tag <APP_IMAGE> --build-arg ARCH=aarch64 .
   ```

   <APP_IMAGE> is the name to tag the image with, e.g., opencv-app:1.0

   Copy the result from the container image to a local directory build:

   ```bash
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

### Walkthrough

#### Dockerfile

In our [Dockerfile](Dockerfile) are the instructions which builds OpenCV and our application. As the application is not built on the camera platform, but rather on our host machine for the camera platform, a crosscompilation toolchain needs to be installed. This is done with the
installation of the `crossbuild-essential-{armhf or arm64}` package, depending on the camera architecture.

With the toolchain installed, OpenCV can be retrieved and configured. To make OpenCV build for the armhf platform, the crosscompilation toolchain
is specified with the `CMAKE_TOOLCHAIN_FILE` option. Other noteworthy options are the ones related to *NEON* and *VFPV3*, which are optimizations
available for the platform that can greatly speed up CPU operations. Those options are implicitly present in the aarch64 compiler implementation, they are therefore explicitly omitted for the aarch64 build. The other configuration options are there to make the OpenCV installation quite stripped
of functionality not needed for our example application. However, you will likely have to change these options to accommodate your custom
application.

OpenCV is built and the output is put in the `/target-root/` directory, as per the `CMAKE_INSTALL_PREFIX` option. This is done in order
to keep the armhf files separated from the non-armhf files. These files are then copied to
the `/opt/axis/acapsdk/sysroots/cortexa9hf-neon-poky-linux-gnueabi/` folder as this is used as the root for the application's Makefile.

#### Makefile

The [Makefile](app/Makefile) in the `app` directory specifies how the application is compiled and packaged. Notable in the Makefile is that the
OpenCV libraries our application needs are bundled with the application. These are copied to a path, `lib/`, relative to the applications
root through the `libscopy` target and then located using rpath.

#### OpenCV application

The OpenCV application can be seen in [example.cpp](app/example.cpp) in the `app` directory. It is a C++ application
which uses OpenCV's MOG2 background subtraction and noise filtering to detect changes in the image in order to perform motion detection.
The code is documented to give a clear understanding of what steps are needed to grab frames from the camera and perform operations on them.
The output of the application can be seen through the `App log` or by running `journalctl -f` while connected through SSH to the camera.

## Notes

Proxy settings for the Docker build are taken from the environment variables `http_proxy` and `https_proxy`.

## License

**[Apache License 2.0](../LICENSE)**

## References

* <https://docs.opencv.org/4.5.1/>
