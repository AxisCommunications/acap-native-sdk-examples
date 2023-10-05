# What is Axis Camera Application Platform?

AXIS Camera Application Platform (ACAP) is an open application platform from Axis. It provides a development platform for software-based solutions and systems built around Axis devices. ACAP is available for various types of Axis products such as cameras, speakers and intercoms.

## ACAP Native SDK

The ACAP Native SDK is targeted towards users that want to develop plug-in style, event generating applications that fit well into a VMS centric system. This SDK offers high performance by integrating closely with AXIS OS and hardware. Already existing ACAP users should feel at home using this SDK and migrating from previous version ACAP 3 to this SDK should be straightforward.

Please check the following guidelines for ACAP 4:

- [AXIS ACAP 4 SDK Documentation](https://axiscommunications.github.io/acap-documentation/)
- [Introduction](https://axiscommunications.github.io/acap-documentation/docs/introduction.html)
- [Getting Started](https://axiscommunications.github.io/acap-documentation/docs/get-started.html)

## Getting started with the repo

This repository contains a set of application examples which aims to enrich the developers analytics experience. All examples are using Docker framework and has a README file in its directory which shows overview, example directory structure and step-by-step instructions on how to run applications on the camera.

## Example applications

Below is the list of examples available in the repository.

- [axevent](./axevent/)
  - The example code is written in C which illustrates both how to subscribe to different events and how to send an event.
- [axoverlay](./axoverlay/)
  - The example code is written in C which illustrates how to draw plain boxes and text as overlays in a stream.
- [axstorage](./axstorage/)
  - The example code is written in C which shows how to use available storage devices.
- [container-example](./container-example/)
  - This example demonstrates a way of using containers in a native ACAP application.
- [hello-world](./hello-world/)
  - The example code is written in C and shows how to build a simple hello world application.
- [licensekey](./licensekey/)
  - The example code is written in C which illustrates how to check the licensekey status.
- [object-detection](./object-detection/)
  - The example code focus on object detection, cropping and saving detected objects into jpeg files.
- [object-detection-cv25](./object-detection-cv25/)
  - This example is very similar to object-detection, but is designed for AXIS CV25 devices.
- [reproducible-package](./reproducible-package/)
  - An example of how to create a reproducible application package.
- [shell-script-example](./shell-script-example)
  - A simple hello world application written in shell script.
- [tensorflow-to-larod](./tensorflow-to-larod/)
  - This example covers model conversion, model quantization, image formats and custom models.
- [tensorflow-to-larod-artpec8](./tensorflow-to-larod-artpec8/)
  - This example is very similar to tensorflow-to-larod, but is designed for AXIS ARTPEC-8 devices.
- [tensorflow-to-larod-cv25](./tensorflow-to-larod-cv25/)
  - This example is very similar to tensorflow-to-larod, but is designed for AXIS CV25 devices.
- [using-opencv](./using-opencv/)
  - This example covers how to build, bundle and use OpenCV in an application.
- [utility-libraries](./utility-libraries/)
  - These examples covers how to build, bundle and use external libraries.
- [vdo-larod](./vdo-larod/)
  - The example code is written in C and loads a pretrained person-car classification model to the [Machine learning API (Larod)](https://axiscommunications.github.io/acap-documentation/docs/api/native-sdk-api.html#machine-learning-api-larod) and then uses the [Video capture API (VDO)](https://axiscommunications.github.io/acap-documentation/docs/api/native-sdk-api.html#video-capture-api-vdo) to fetch video frames in YUV format and finally run inference.
- [vdo-opencl-filtering](./vdo-opencl-filtering/)
  - This example illustrates how to capture frames from the vdo service, access the received buffer, and finally perform a GPU accelerated Sobel filtering with OpenCL.
- [vdostream](./vdostream/)
  - The example code is written in C which starts a vdo stream and then illustrates how to continuously capture frames from the vdo service, access the received buffer contents as well as the frame metadata.
- [web-server](./web-server/)
  - The example code is written in C which runs a Monkey web server on the camera and exposes an external API with Reverse Proxy configuration in Apache Server.
- [web-server-using-fastcgi](./web-server-using-fastcgi/)
  - This example code is written in C and explains how to build an ACAP application that can handle HTTP requests sent to the Axis device, using the device's own web server.

### DockerHub Image

The ACAP Native SDK image can be used as a basis for custom built images to run your application or as a developer environment inside the container.

- [ACAP Native SDK](https://hub.docker.com/r/axisecp/acap-native-sdk) This image is based on Ubuntu and contains the environment needed for building an AXIS Camera Application Platform (ACAP) Native application. This includes all tools for building and packaging an ACAP Native application as well as API components (header and library files) needed for accessing different parts of the camera firmware.

# Issues

If you encounter issues with the examples, make sure your product is running the latest firmware version or one that is compatible with the ACAP SDK used.
The examples use the ACAP SDK during the build process, of which each version is compatible with a set of firmware versions.
The specific SDK version that each example is based on is specified in the Dockerfile used to build the application, through the `VERSION` variable.
The full compatibility schema for ACAP SDK version and firmware version is available at [Compatibility for Native SDK](https://axiscommunications.github.io/acap-documentation/docs/api/native-sdk-api.html#compatibility).

If the issue persists with a compatible firmware, please create an issue containing the information specified in the template below.

## Issue template

*Axis product/device (e.g. Q1615 Mk III):*

*Device firmware version:*

*Issue description:*

# License

[Apache 2.0](LICENSE)
