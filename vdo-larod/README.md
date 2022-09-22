 *Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A combined vdo stream and larod based ACAP application running inference on an edge device

This README file explains how to build an ACAP application that uses:

- vdo to fetch frames from e.g. a camera
- a library called libyuv to do image preprocessing
- [larod API](../FAQs.md#WhatisLarod?) to load a graph model and run classification inferences on it

It is achieved by using the containerized API and toolchain images.

Together with this README file you should be able to find a directory called app. That directory contains the "vdo_larod" application source code, which can easily
be compiled and run with the help of the tools and step by step below.

## Detailed outline of example application

This application opens a client to vdo and starts fetching frames (in a new thread) in the yuv format. It tries to match twice the WIDTH and HEIGHT that is required by the neural network. The thread fetching frames is written so that it always tries to provide a frame as new as possible even if not all previous frames have been processed by libyuv and larod. The implementation of the vdo specific parts of the app can be found in file "imgprovider.c".

The image preprocessing is then started by libyuv in "imgconverter.c". The code will perform the following three steps:

1. Convert the full size NV12 image delivered by vdo to interleaved BGRA color format since the bulk of libyuv functions works on that format.
2. Crop out a part of the full size image. The crop will be taken from the center of the vdo image and be as big as possible while still maintaining the WIDTH x HEIGHT aspect ratio. This crop will then be scaled to be of the same size as what the neural network requires (WIDTH x HEIGHT).
3. Lastly the scaled image will be converted from BGRA to interleaved RGB which is a common format for e.g. Mobilenet CNNs.

Finally larod will load a neural network model and start processing. It simply takes the images produced by vdo and libyuv and makes synchronous inferences calls to the neural network that was loaded. These function calls return when inferences are finished upon which the application parses the output tensor provided to print the top result to syslog/application log. The larod related code is found in "vdo_larod.c".

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```bash
vdo-larod
├── app
│   ├── argparse.c
│   ├── argparse.h
│   ├── imgconverter.c
│   ├── imgconverter.h
│   ├── imgprovider.c
│   ├── imgprovider.h
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json.artpec8
│   ├── manifest.json.cpu
│   ├── manifest.json.cv25
│   ├── manifest.json.edgetpu
│   └── vdo_larod.c
├── Dockerfile
├── README.md
└── yuv
```

- **app/argparse.c/h** - Implementation of argument parser, written in C.
- **app/imgconverter.c/h** - Implementation of libyuv parts, written in C.
- **app/imgprovider.c/h** - Implementation of vdo parts, written in C.
- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json.artpec8** - Defines the application and its configuration when building for DLPU with TensorFlow Lite.
- **app/manifest.json.cpu** - Defines the application and its configuration when building for CPU with TensorFlow Lite.
- **app/manifest.json.cv25** - Defines the application and its configuration when building chip and model for cv25 DLPU.
- **app/manifest.json.edgetpu** - Defines the application and its configuration when building chip and model for Google TPU.
- **app/vdo-larod.c** - Application using larod, written in C.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.
- **yuv** - Folder containing files for building libyuv.

Below is the patch to build libyuv, remaining details are found in Dockerfile:

```bash
vdo-larod
├── yuv
│   └── 0001-Create-a-shared-library.patch
```

- **yuv/0001-Create-a-shared-library.patch** - Patch, which is needed to support building an .so file, for the shared library libyuv.

## Limitations

- ARTPEC-7 based device with edge TPU.
- ARTPEC-8
- CV25
- This application was not written to optimize performance.
- MobileNet is good for classification, but it requires that the object you want to classify should cover almost all the frame.

## How to run the code

Below is the step by step instructions on how to execute the program. So basically starting with the generation of the .eap file to running it on a device:

### Build the application

> [!IMPORTANT]
> *Depending on the network you are connected to,
The file that needs those settings is:* ~/.docker/config.json. *For
reference please see: <https://docs.docker.com/network/proxy/> and a
[script for Axis device here](../FAQs.md#HowcanIset-upnetworkproxysettingsontheAxisdevice?).*

Depending on selected chip, different models can be used for running larod.
Label file is used for identifying objects in the video stream. In this
example, model and label files are downloaded from <https://coral.ai/models/>,
when building the application. Which model that is used is configured through
attributes in manifest.json and the *CHIP* parameter in the Dockerfile.

The attributes in manifest.json that configures model are:

- runOptions, which contains the application command line options.
- friendlyName, a user friendly package name which is also part of the .eap file name.

The **CHIP** argument in the Dockerfile also needs to be changed depending on
model. This argument controls which files are to be included in the package,
e.g. model files. These files are copied to the application directory during
installation.

> Different devices support different chips and models.

Building is done using the following commands:

```bash
docker build --tag <APP_IMAGE> --build-arg CHIP=<CHIP> .
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

- \<APP_IMAGE\> is the name to tag the image with, e.g., vdo_larod:1.0
- \<CHIP\> is the chip type. Supported values are *artpec8*, *cpu*, *cv25* and *edgetpu*.
- \<ARCH\> is the architecture. Supported values are armv7hf (default) and aarch64

See the following sections for build commands for each chip.

#### Build for ARTPEC-8 with Tensorflow Lite

To build a package for ARTPEC-8 with Tensorflow Lite, run the following commands standing in your working directory:

```bash
docker build --build-arg ARCH=aarch64 --build-arg CHIP=artpec8 --tag <APP_IMAGE> .
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

#### Build for CPU with Tensorflow Lite

To build a package for CPU with Tensorflow Lite, run the following commands standing in your working directory:

```bash
docker build --build-arg CHIP=cpu --tag <APP_IMAGE> .
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

#### Build for Google TPU

To build a package for Google TPU instead, run the following commands standing in your working directory:

```bash
docker build --build-arg CHIP=edgetpu --tag <APP_IMAGE> .
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

#### Build for CV25 using DLPU

To build a package for CV25 run the following commands standing in your working directory:

```bash
docker build --build-arg ARCH=aarch64 --build-arg CHIP=cv25 --tag <APP_IMAGE> .
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

#### Build output

The working directory now contains a build folder with the following files of importance:

```bash
vdo-larod
├── build
│   ├── argparse.c
│   ├── argparse.h
│   ├── imgconverter.c
│   ├── imgconverter.h
│   ├── imgprovider.c
│   ├── imgprovider.h
│   ├── label
|   │   └── imagenet_labels.txt
│   ├── lib
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── manifest.json.artpec8
│   ├── manifest.json.cpu
│   ├── manifest.json.edgetpu
│   ├── manifest.json.cv25
│   ├── model
|   │   ├── mobilenet_v2_cavalry.bin
|   │   ├── mobilenet_v2_1.0_224_quant_edgetpu.tflite
|   │   └── mobilenet_v2_1.0_224_quant.tflite
│   ├── package.conf
│   ├── package.conf.orig
│   ├── param.conf
│   ├── vdo_larod*
│   ├── vdo_larod_{cpu,edgetpu}_1_0_0_armv7hf.eap / vdo_larod_{cv25,artpec8}_1_0_0_aarch64.eap
│   ├── vdo_larod_{cpu,edgetpu}_1_0_0_LICENSE.txt / vdo_larod_{cv25,artpec8}_1_0_0_LICENSE.txt
│   └── vdo_larod.c
```

- **build/label** - Folder containing label files used in this application.
- **build/label/imagenet_labels.txt** - Label file for MobileNet V2 (ImageNet).
- **build/lib** - Folder containing compiled library files for libyuv.
- **build/manifest.json** - Defines the application and its configuration.
- **build/model** - Folder containing models used in this application.
- **build/model/mobilenet_v2_1.0_224_quant_edgetpu.tflite** - Model file for MobileNet V2 (ImageNet), used for Google TPU.
- **build/model/mobilenet_v2_1.0_224_quant.tflite** - Model file for MobileNet V2 (ImageNet), used for ARTPEC-8 and CPU with TensorFlow Lite.
- **build/package.conf** - Defines the application and its configuration.
- **build/package.conf.orig** - Defines the application and its configuration, original file.
- **build/param.conf** - File containing application parameters.
- **build/vdo_larod** - Application executable binary file.

  If chip `artpec8` has been built.
- **build/vdo_larod_artpec8_1_0_0_aarch64.eap** - Application package .eap file.
- **build/vdo_larod_artpec8_1_0_0_LICENSE.txt** - Copy of LICENSE file.

  If chip `cpu` has been built.
- **build/vdo_larod_cpu_1_0_0_armv7hf.eap** - Application package .eap file.
- **build/vdo_larod_cpu_1_0_0_LICENSE.txt** - Copy of LICENSE file.

  If chip `edgetpu` has been built.
- **build/vdo_larod_edgetpu_1_0_0_armv7hf.eap** - Application package .eap file.
- **build/vdo_larod_edgetpu_1_0_0_LICENSE.txt** - Copy of LICENSE file.

  If chip `cv25` has been built.
- **build/vdo_larod_preprocessing_cv25_1_0_0_aarch64.eap** - Application package .eap file.
- **build/vdo_larod_preprocessing_cv25_1_0_0_LICENSE.txt** - Copy of LICENSE file.

### Install your application

Installing your application on an Axis video device is as simple as:

Browse to the following page (replace <axis_device_ip> with the IP number of your Axis video device)

```bash
http://<axis_device_ip>/#settings/apps
```

*Go to your device web page above >
 Click on the tab **App** in the device GUI >
 Add **(+)** sign and browse to the newly built
**vdo_larod_cv25_1_0_0_aarch64.eap** or
 **vdo_larod_artpec8_1_0_0_aarch64.eap** or
 **vdo_larod_cpu_1_0_0_armv7hf.eap** or
 **vdo_larod_edgetpu_1_0_0_armv7hf.eap** >
 Click **Install** >
 Run the application by enabling the **Start** switch*

The application is now installed on the device and named "vdo_larod_<CHIP>".

### The expected output

Application log can be found directly at:

```sh
http://<axis_device_ip>/axis-cgi/admin/systemlog.cgi?appname=vdo_larod
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

Depending on selected chip, different output is received. The label file is used for identifying objects.

In the system log the chip is sometimes only mentioned as a number, they are mapped as follows:

| Number | Chip |
| --- | --- |
| 2 | CPU with TensorFlow Lite |
| 4 | Google TPU |
| 6 | Ambarella CVFlow (NN) |
| 12 | ARTPEC-8 DLPU |

#### Output - ARTPEC-8 with TensorFlow Lite

```sh
----- Contents of SYSTEM_LOG for 'vdo_larod' -----

vdo_larod[423215]: Creating VDO image provider and creating stream 320 x 240
vdo_larod[423215]: Dump of vdo stream settings map =====
vdo_larod[423215]: chooseStreamResolution: We select stream w/h=320 x 240 based on VDO channel info.
vdo_larod[423215]: Setting up larod connection with chip 12 and model /usr/local/packages/vdo_larod/model/mobilenet_v2_1.0_224_quant.tflite
vdo_larod[423215]: Converted image in 2 ms
vdo_larod[423215]: Creating temporary files and memmaps for inference input and output tensors
vdo_larod[423215]: Start fetching video frames from VDO
vdo_larod[423215]: createAndMapTmpFile: Setting up a temp fd with pattern /tmp/larod.in.test-XXXXXX and size 150528
vdo_larod[423215]: createAndMapTmpFile: Setting up a temp fd with pattern /tmp/larod.out.test-XXXXXX and size 1001
vdo_larod[423215]: Converted image in 2 ms
vdo_larod[423215]: Ran inference for 7 ms
vdo_larod[27814]: Top result:  955  banana with score 94.20%
vdo_larod[423215]: Converted image in 2 ms
vdo_larod[423215]: Ran inference for 8 ms
vdo_larod[27814]: Top result:  955  banana with score 94.60%
```

#### Output - CPU with TensorFlow Lite

```sh
----- Contents of SYSTEM_LOG for 'vdo_larod' -----

vdo_larod[13021]: Creating VDO image provider and creating stream 320 x 240
vdo_larod[13021]: Dump of vdo stream settings map =====
vdo_larod[13021]: chooseStreamResolution: We select stream w/h=320 x 240 based on VDO channel info.
vdo_larod[13021]: Setting up larod connection with chip 2 and model /usr/local/packages/vdo_larod/model/mobilenet_v2_1.0_224_quant.tflite
vdo_larod[13021]: Creating temporary files and memmaps for inference input and output tensors
vdo_larod[13021]: Start fetching video frames from VDO
vdo_larod[13021]: createAndMapTmpFile: Setting up a temp fd with pattern /tmp/larod.in.test-XXXXXX and size 150528
vdo_larod[13021]: createAndMapTmpFile: Setting up a temp fd with pattern /tmp/larod.out.test-XXXXXX and size 1001
vdo_larod[13021]: Converted image in 3 ms
vdo_larod[13021]: Ran inference for 417 ms
vdo_larod[13021]: Top result:  955  banana with score 84.00%
vdo_larod[13021]: Converted image in 3 ms
vdo_larod[13021]: Ran inference for 356 ms
vdo_larod[13021]: Top result:  955  banana with score 85.60%
```

#### Output - Google TPU

```sh
----- Contents of SYSTEM_LOG for 'vdo_larod' -----

vdo_larod[27814]: Creating VDO image provider and creating stream 320 x 240
vdo_larod[27814]: Dump of vdo stream settings map =====
vdo_larod[27814]: chooseStreamResolution: We select stream w/h=320 x 240 based on VDO channel info.
vdo_larod[27814]: Setting up larod connection with chip 4 and model /usr/local/packages/vdo_larod/model/mobilenet_v2_1.0_224_quant_edgetpu.tflite
vdo_larod[27814]: Creating temporary files and memmaps for inference input and output tensors
vdo_larod[27814]: Start fetching video frames from VDO
vdo_larod[27814]: createAndMapTmpFile: Setting up a temp fd with pattern /tmp/larod.in.test-XXXXXX and size 150528
vdo_larod[27814]: createAndMapTmpFile: Setting up a temp fd with pattern /tmp/larod.out.test-XXXXXX and size 1001
vdo_larod[27814]: Converted image in 3 ms
vdo_larod[27814]: Ran inference for 27 ms
vdo_larod[27814]: Top result:  955  banana with score 93.60%
vdo_larod[27814]: Converted image in 3 ms
vdo_larod[27814]: Ran inference for 17 ms
vdo_larod[27814]: Top result:  955  banana with score 93.60%
```

#### Output - CV25

```sh

----- Contents of SYSTEM_LOG for 'vdo_larod' -----


2022-08-17T09:36:17.450+02:00 axis-b8a44f31b72f [ INFO    ] vdo_larod[584067]: Starting ...

vdo_larod[584067]: Creating VDO image provider and creating stream 320 x 240
vdo_larod[584067]: Dump of vdo stream settings map =====
vdo_larod[584067]: chooseStreamResolution: We select stream w/h=320 x 240 based on VDO channel info.
vdo_larod[584067]: Setting up larod connection with chip 6 and model /usr/local/packages/vdo_larod/model/mobilenet_v2_cavalry.bin
vdo_larod[584067]: Creating temporary files and memmaps for inference input and output tensors
vdo_larod[584067]: Start fetching video frames from VDO
vdo_larod[584067]: createAndMapTmpFile: Setting up a temp fd with pattern /tmp/larod.in.test-XXXXXX and size 150528
vdo_larod[584067]: createAndMapTmpFile: Setting up a temp fd with pattern /tmp/larod.out.test-XXXXXX and size 32032
vdo_larod[584067]: Converted image in 3 ms
vdo_larod[584067]: Ran inference for 6 ms
vdo_larod[584067]: Top result:  955  banana with score 73.90%
vdo_larod[584067]: Converted image in 2 ms
vdo_larod[584067]: Ran inference for 6 ms
vdo_larod[584067]: Top result:  955  banana with score 78.71%
```

#### Conclusion

- This is an example of test data, which is dependent on selected device and chip.
- One full-screen banana has been used for testing.
- Running inference is much faster on ARTPEC-8 and Google TPU in comparison to CPU.
- Converting images takes almost the same time on all chips.
- Objects with score less than 60% are generally not good enough to be used as classification results.

## License

**[Apache License 2.0](../LICENSE)**
