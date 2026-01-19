# What is Axis Camera Application Platform?

AXIS Camera Application Platform (ACAP) is an open application platform from Axis. It provides a development platform for software-based solutions and systems built around Axis devices. ACAP is available for various types of Axis products such as cameras, speakers and intercoms.

## ACAP Native SDK

The ACAP Native SDK is targeted towards users that want to develop plug-in style, event generating applications that fit well into a VMS centric system. This SDK offers high performance by integrating closely with AXIS OS and hardware. Already existing ACAP users should feel at home using this SDK and migrating from previous version ACAP 3 to this SDK should be straightforward.

Please check the following guidelines for ACAP application development:

- [ACAP documentation](https://developer.axis.com/acap/)
- [Introduction](https://developer.axis.com/acap/introduction/what-is-acap)
- [Getting Started](https://developer.axis.com/acap/get-started/set-up-developer-environment/pre-requisites)

## Getting started with the repository

This repository contains a set of application examples which aims to enrich the developers analytics experience. All examples are using Docker framework and has a README file in its directory which shows overview, example directory structure and step-by-step instructions on how to run applications on the camera.

## Example applications

Below is the list of examples available in the repository.

The examples are organized into logical groups to help you find the most relevant starting point for your application development needs. Each group contains examples that build upon similar concepts or complement each other in functionality.

### Introduction

- [hello-world](./hello-world/)
  - A simple hello world C application.

### Streaming video

- [vdostream](./vdostream/)
  - An example in C that starts a vdo stream and then illustrates how to continuously capture frames from the vdo service, access the received buffer contents as well as the frame metadata.

### Audio

- [audio-capture](./audio-capture/)
  - Example in C that illustrate how to capture audio.
- [audio-playback](./audio-playback/)
  - Example in C that illustrate how to play audio.

### Machine learning

#### Object Detection

- [object-detection](./object-detection/)
  - An example of object detection, drawing bounding boxes around detected objects on the video stream.
- [object-detection-cv25](./object-detection-cv25/)
  - An example of object detection, cropping and saving detected objects into JPEG files on AXIS CV25 devices.
- [object-detection-yolov5](./object-detection-yolov5/)
  - An example of object detection using YOLOv5, drawing bounding boxes around detected objects on the video stream.

#### Train and convert models for Axis devices

- [tensorflow-to-larod](./tensorflow-to-larod/)
  - A guide of how to train and export machine learning models to make them compatible with ARTPEC-7 devices.
- [tensorflow-to-larod-artpec8](./tensorflow-to-larod-artpec8/)
  - A guide of how to train and export machine learning models to make them compatible with ARTPEC-8 devices.
- [tensorflow-to-larod-artpec9](./tensorflow-to-larod-artpec9/)
  - A guide of how to train and export machine learning models to make them compatible with ARTPEC-9 devices. Note that this example is pointing to [tensorflow-to-larod-artpec8](./tensorflow-to-larod-artpec8).
- [tensorflow-to-larod-cv25](./tensorflow-to-larod-cv25/)
  - A guide of how to train and export machine learning models to make them compatible with CV25 devices.
- [vdo-larod](./vdo-larod/)
  - An example in C that runs one of the  machine learning models trained with the tensorflow-to-larod* guides on the video stream of the device.

### Build custom libraries for an application

- [using-opencv](./using-opencv/)
  - An example that shows how to build, bundle and use OpenCV in an application.
- [utility-libraries](./utility-libraries/)
  - These examples covers how to build, bundle and use external libraries.

### Graphical components

- [axoverlay](./axoverlay/)
  - An example in C that illustrates how to draw plain boxes and text as overlays in a stream.
- [bounding-box](./bounding-box/)
  - An example in C that demonstrates how to portably draw burnt-in bounding boxes on selected video sources or channels.
- [vdo-opencl-filtering](./vdo-opencl-filtering/)
  - An example that illustrates how to capture frames from the vdo service, access the received buffer, and finally perform a GPU accelerated Sobel filtering with OpenCL.

### Networking and web interface

- [curl-openssl](./curl-openssl/)
  - An example that use cURL and OpenSSL libraries to retrieve a file securely from an external server.
- [web-server](./web-server/)
  - An example in C that serves HTTP requests by setting up the Axis device web server in a reverse proxy configuration and route to a custom web server running in the ACAP application.
- [web-server-using-fastcgi](./web-server-using-fastcgi/)
  - An example in C and explains how to build an ACAP application that can handle HTTP requests sent to the Axis device, using the device's own web server.

### Event handling

- [axevent](./axevent/)
  - Examples in C that illustrate how to subscribe to and send events.

### Parameter handling

- [axparameter](./axparameter/)
  - An example in C that demonstrates how to manage application-defined parameters, allowing you to add, remove, set, get, and register callback functions for parameter value updates.

### Debugging and testing

- [remote-debug-example](./remote-debug-example/)
  - An example of how to remote debug an ACAP application.

### Container applications

- [container-example](./container-example/)
  - An example that demonstrates the use of containers in a native ACAP application.

### Miscellaneous

- [axserialport](./axserialport/)
  - An example in C that shows the use of the serial port API.
- [axstorage](./axstorage/)
  - An example in C that shows how to use available storage devices.
- [licensekey](./licensekey/)
  - An example in C that illustrates how to check the licensekey status.
- [message-broker](./message-broker/)
  - Examples that showcase how to use the Message Broker API.
- [nexus](./nexus/)
  - Examples that showcase how to use the Nexus API.
- [reproducible-package](./reproducible-package/)
  - An example of how to create a reproducible application package.
- [shell-script-example](./shell-script-example)
  - A simple hello world shell script application.
  <!-- textlint-disable terminology -->
- [vapix](./vapix/)
  <!-- textlint-enable -->
  - An example in C that retrieves VAPIX credentials over D-Bus and makes VAPIX calls over a loopback interface.

## Docker Hub image

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
The full compatibility schema for ACAP SDK version and firmware version is available at [Compatibility for Native SDK](https://developer.axis.com/acap/api/native-sdk-api/#compatibility).

If the issue persists with a compatible firmware, please create an issue containing the information specified in the template below.

# License

[Apache 2.0](LICENSE)
