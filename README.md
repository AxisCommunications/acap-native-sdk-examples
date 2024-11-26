# What is Axis Camera Application Platform?

AXIS Camera Application Platform (ACAP) is an open application platform from Axis. It provides a development platform for software-based solutions and systems built around Axis devices. ACAP is available for various types of Axis products such as cameras, speakers and intercoms.

## ACAP Native SDK

The ACAP Native SDK is targeted towards users that want to develop plug-in style, event generating applications that fit well into a VMS centric system. This SDK offers high performance by integrating closely with AXIS OS and hardware. Already existing ACAP users should feel at home using this SDK and migrating from previous version ACAP 3 to this SDK should be straightforward.

Please check the following guidelines for ACAP application development:

- [ACAP documentation](https://axiscommunications.github.io/acap-documentation/)
- [Introduction](https://axiscommunications.github.io/acap-documentation/docs/introduction.html)
- [Getting Started](https://axiscommunications.github.io/acap-documentation/docs/get-started.html)
- [Developer environment requisites](https://axiscommunications.github.io/acap-documentation/docs/get-started/set-up-developer-environment/pre-requisites.html)

## Getting started with the repository

This repository contains a set of application examples which aims to enrich the developers analytics experience. All examples are using Docker framework and has a README file in its directory which shows overview, example directory structure and step-by-step instructions on how to run applications on the camera.

## Example applications

Below is the list of examples available in the repository.

- [axevent](./axevent/)
  - Examples in C that illustrate how to subscribe to and send events.
- [axoverlay](./axoverlay/)
  - An example in C that illustrates how to draw plain boxes and text as overlays in a stream.
- [axparameter](./axparameter/)
  - An example in C that demonstrates how to manage application-defined parameters, allowing you to add, remove, set, get, and register callback functions for parameter value updates.
- [axserialport](./axserialport/)
  - An example in C that shows the use of the serial port API.
- [axstorage](./axstorage/)
  - An example in C that shows how to use available storage devices.
- [bounding-box](./bounding-box/)
  - An example in C that demonstrates how to portably draw burnt-in bounding boxes on selected video sources or channels.
- [container-example](./container-example/)
  - An example that demonstrates the use of containers in a native ACAP application.
- [curl-openssl](./curl-openssl/)
  - An example that use curl and OpenSSL libraries to retrieve a file securely from an external server.
- [hello-world](./hello-world/)
  - A simple hello world C application.
- [licensekey](./licensekey/)
  - An example in C that illustrates how to check the licensekey status.
- [message-broker](./message-broker/)
  - Examples that showcase how to use the Message Broker API.
- [object-detection](./object-detection/)
  - An example of object detection, cropping and saving detected objects into JPEG files.
- [object-detection-cv25](./object-detection-cv25/)
  - An example of object detection, cropping and saving detected objects into JPEG files on AXIS CV25 devices.
- [remote-debug-example](./remote-debug-example/)
  - An example of how to remote debug an ACAP application.
- [reproducible-package](./reproducible-package/)
  - An example of how to create a reproducible application package.
- [shell-script-example](./shell-script-example)
  - A simple hello world shell script application.
- [tensorflow-to-larod](./tensorflow-to-larod/)
  - An example that shows model conversion, model quantization, image formats and custom models.
- [tensorflow-to-larod-artpec8](./tensorflow-to-larod-artpec8/)
  - An example that shows model conversion, model quantization, image formats and custom models on AXIS ARTPEC-8 devices.
- [tensorflow-to-larod-cv25](./tensorflow-to-larod-cv25/)
  - An example that shows model conversion, model quantization, image formats and custom models on AXIS CV25 devices.
- [using-opencv](./using-opencv/)
  - An example that shows how to build, bundle and use OpenCV in an application.
- [utility-libraries](./utility-libraries/)
  - These examples covers how to build, bundle and use external libraries.
  <!-- textlint-disable terminology -->
- [vapix](./vapix/)
  <!-- textlint-enable -->
  - An example in C that retrieves VAPIX credentials over D-Bus and makes VAPIX calls over a loopback interface.
  <!-- textlint-disable terminology -->
- [vapix-sh](./vapix-sh/)
  <!-- textlint-enable -->
  - A shellscript example that retrieves VAPIX credentials over D-Bus and makes VAPIX calls over a loopback interface.
- [vdo-larod](./vdo-larod/)
  - An example in C that loads a pretrained person-car classification model to the [Machine learning API (Larod)](https://axiscommunications.github.io/acap-documentation/docs/api/native-sdk-api.html#machine-learning-api-larod) and then uses the [Video capture API (VDO)](https://axiscommunications.github.io/acap-documentation/docs/api/native-sdk-api.html#video-capture-api-vdo) to fetch video frames in YUV format and finally run inference.
- [vdo-opencl-filtering](./vdo-opencl-filtering/)
  - An example that illustrates how to capture frames from the vdo service, access the received buffer, and finally perform a GPU accelerated Sobel filtering with OpenCL.
- [vdostream](./vdostream/)
  - An example in C that starts a vdo stream and then illustrates how to continuously capture frames from the vdo service, access the received buffer contents as well as the frame metadata.
- [web-server](./web-server/)
  - An example in C that runs a Monkey web server on the camera and exposes an external API with Reverse Proxy configuration in Apache Server.
- [web-server-using-fastcgi](./web-server-using-fastcgi/)
  - An example in C and explains how to build an ACAP application that can handle HTTP requests sent to the Axis device, using the device's own web server.

### Docker Hub image

The ACAP Native SDK image can be used as a basis for custom built images to run your application or as a developer environment inside the container.

- [ACAP Native SDK](https://hub.docker.com/r/axisecp/acap-native-sdk) This image is based on Ubuntu and contains the environment needed for building an AXIS Camera Application Platform (ACAP) Native application. This includes all tools for building and packaging an ACAP Native application as well as API components (header and library files) needed for accessing different parts of the camera firmware.

## ACAP application development

Common topics of interest for developers can be found under [ACAP application
development](./DEV.md), e.g. when testing out the examples in this repository
or migrating from an old version of ACAP SDK.

# Issues

If you encounter issues with the examples, make sure your product is running the latest firmware version or one that is compatible with the ACAP SDK used.
The examples use the ACAP SDK during the build process, of which each version is compatible with a set of firmware versions.
The specific SDK version that each example is based on is specified in the Dockerfile used to build the application, through the `VERSION` variable.
The full compatibility schema for ACAP SDK version and firmware version is available at [Compatibility for Native SDK](https://axiscommunications.github.io/acap-documentation/docs/api/native-sdk-api.html#compatibility).

If the issue persists with a compatible firmware, please create an issue containing the information specified in the template below.

# License

[Apache 2.0](LICENSE)
