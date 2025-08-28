*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A combined VDO stream and Larod based ACAP application running inference on an edge device

This README file explains how to build an ACAP application that uses:

- the [Video capture API (VDO)](https://developer.axis.com/acap/api/native-sdk-api/#video-capture-api-vdo) to fetch frames from e.g. a camera
- the [Machine learning API (Larod)](https://developer.axis.com/acap/api/native-sdk-api/#machine-learning-api-larod) to load a graph model and run preprocessing and classification inferences

It is achieved by using the containerized API and toolchain images.

Together with this README file you should be able to find a directory called app. That directory contains the "vdo_larod" application source code, which can easily be compiled and run with the help of the tools and step by step below.

## Prerequisites

- Axis camera equipped with CPU or DLPU
- [Docker](https://docs.docker.com/get-docker/)

## Detailed outline of example application

This application opens a client to VDO and starts fetching frames (in a new thread) in the YUV format. It tries to find the smallest VDO stream resolution that fits the width and height required by the neural network.

Steps in application:

1. Fetch image data from VDO.
2. Preprocess the images (crop to 480x270 (if needed), scale and color convert) using larod with libyuv backend (depending on platform).
3. Run inferences using the trained model on a specific chip with the preprocessing output as input on a larod backend specified by a command-line argument.
4. The model's confidence scores for the presence of person and car in the image are printed as the output.
5. Repeat for 5 iterations.

See the manifest.json.* files to change the configuration on chip, image size, number of iterations and model path.

## Which backends and models are supported?

Unless you modify the app to your own needs you should only use our pretrained model that takes 480x270 (256x256 for Ambarella CV25 and Google TPU) RGB images as input,
and that outputs an array of 2 confidence scores of person and car in the format of `float32`.

You can run the example with any inference backend as long as you can provide it with a model as described above.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
vdo-larod
├── app
│   ├── imgprovider.c
│   ├── imgprovider.h
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json.artpec8
│   ├── manifest.json.artpec9
│   ├── manifest.json.cpu
│   ├── manifest.json.cv25
│   ├── manifest.json.edgetpu
│   ├── model.c
│   ├── model.h
│   ├── panic.c
│   ├── panic.h
│   └── vdo_larod.c
├── Dockerfile
└── README.md
```

- **app/imgprovider.c/h** - Implementation of vdo parts, written in C.
- **app/utility-functions.c/h** - Contains all the necessary helper functions written in C that are used while building the ACAP application.
- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
  <!-- textlint-disable -->
- **app/manifest.json.artpec8** - Defines the application and its configuration when building for artpec8 DLPU with TensorFlow Lite.
- **app/manifest.json.artpec9** - Defines the application and its configuration when building for artpec9 DLPU with TensorFlow Lite.
  <!-- textlint-enable -->
- **app/manifest.json.cpu** - Defines the application and its configuration when building for CPU with TensorFlow Lite.
- **app/manifest.json.cv25** - Defines the application and its configuration when building chip and model for cv25 DLPU.
- **app/manifest.json.edgetpu** - Defines the application and its configuration when building chip and model for Google TPU.
- **app/panic.c/h** - Utility for exiting the program on error
- **app/vdo_larod.c** - Application using larod, written in C.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.

## Limitations

- The example shows how to run on a device's DLPU or CPU, but for good
  performance it's recommended to use products with DLPU. See links to search
  for products with DLPU support in [Axis device compatibility](https://www.axis.com/support/tools/product-selector/shared/%5B%7B%22index%22%3A%5B10%2C1%5D%2C%22value%22%3A%22DLPU%22%7D%5D).
- This application was not written to optimize performance
- The pretrained models only outputs the confidence of two classes i.e., person and car. For options on pretrained models that classify a higher number of classes, visit the  [Axis Model Zoo](https://github.com/AxisCommunications/axis-model-zoo).

## How to run the code

Below is the step by step instructions on how to execute the program. So basically starting with the generation of the .eap file to running it on a device:

### Build the application

> [!NOTE]
>
> Depending on the network your local build machine is connected to, you may need to add proxy
> settings for Docker. See
> [Proxy in build time](https://developer.axis.com/acap/develop/proxy/#proxy-in-build-time).

Depending on selected chip, different models are trained and are used for running laord.
In this example, model files are downloaded from an AWS S3 bucket,
when building the application. Which model that is used is configured through
attributes in manifest.json and the *CHIP* parameter in the Dockerfile.

The attributes in manifest.json that configures model are:

- runOptions, which contains the application command-line options.
- friendlyName, a user friendly package name which is also part of the .eap filename.

The **CHIP** argument in the Dockerfile also needs to be changed depending on
model. This argument controls which files are to be included in the package,
e.g. model files. These files are copied to the application directory during
installation.

> Different devices support different chips and models.

Building is done using the following commands:

```sh
docker build --tag <APP_IMAGE> --build-arg CHIP=<CHIP> .
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

- \<APP_IMAGE\> is the name to tag the image with, e.g., `vdo_larod:1.0`.
- \<CHIP\> is the chip type. Supported values are `artpec9`, `artpec8`, `cpu`, `cv25` and `edgetpu`.
- \<ARCH\> is the architecture. Supported values are `armv7hf` (default) and `aarch64`.

See the following sections for build commands for each chip.

#### Build for ARTPEC-8 with Tensorflow Lite

To build a package for ARTPEC-8 with Tensorflow Lite, run the following commands standing in your working directory:

```sh
docker build --build-arg ARCH=aarch64 --build-arg CHIP=artpec8 --tag <APP_IMAGE> .
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

#### Build for ARTPEC-9 with Tensorflow Lite

To build a package for ARTPEC-9 with Tensorflow Lite, run the following commands standing in your working directory:

```sh
docker build --build-arg ARCH=aarch64 --build-arg CHIP=artpec9 --tag <APP_IMAGE> .
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

#### Build for CPU with Tensorflow Lite

To build a package for CPU with Tensorflow Lite, run the following commands standing in your working directory:

```sh
docker build --build-arg CHIP=cpu --tag <APP_IMAGE> .
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

#### Build for Google TPU

To build a package for Google TPU instead, run the following commands standing in your working directory:

```sh
docker build --build-arg CHIP=edgetpu --tag <APP_IMAGE> .
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

#### Build for CV25 using DLPU

To build a package for CV25 run the following commands standing in your working directory:

```sh
docker build --build-arg ARCH=aarch64 --build-arg CHIP=cv25 --tag <APP_IMAGE> .
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

#### Build output

The working directory now contains a build folder with the following files of importance:

```sh
vdo-larod
├── build
│   ├── imgprovider.c
│   ├── imgprovider.h
│   ├── lib
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── manifest.json.artpec8
│   ├── manifest.json.artpec9
│   ├── manifest.json.cpu
│   ├── manifest.json.edgetpu
│   ├── manifest.json.cv25
│   ├── model
|   │   └── model.tflite / model.bin
│   ├── package.conf
│   ├── package.conf.orig
│   └── panic.c
│   └── panic.h
│   ├── param.conf
│   ├── vdo_larod*
│   ├── vdo_larod_{cpu,edgetpu}_1_0_0_armv7hf.eap / vdo_larod_{cv25,artpec8,artpec9}_1_0_0_aarch64.eap
│   ├── vdo_larod_{cpu,edgetpu}_1_0_0_LICENSE.txt / vdo_larod_{cv25,artpec8,artpec9}_1_0_0_LICENSE.txt
│   └── vdo_larod.c
```

- **build/manifest.json** - Defines the application and its configuration.
- **build/model** - Folder containing models used in this application.
- **build/model/model.tflite** - Trained model file used for ARTPEC-8, ARTPEC-9, and CPU, or trained model file used for Google TPU, depending on `<CHIP>`.
- **build/model/model.bin** - Trained model file used for CV25.
- **build/package.conf** - Defines the application and its configuration.
- **build/package.conf.orig** - Defines the application and its configuration, original file.
- **build/param.conf** - File containing application parameters.
- **build/vdo_larod** - Application executable binary file.

  If chip `artpec8` has been built.
- **build/vdo_larod_artpec8_1_0_0_aarch64.eap** - Application package .eap file.
- **build/vdo_larod_artpec8_1_0_0_LICENSE.txt** - Copy of LICENSE file.

  If chip `artpec9` has been built.
- **build/vdo_larod_artpec9_1_0_0_aarch64.eap** - Application package .eap file.
- **build/vdo_larod_artpec9_1_0_0_LICENSE.txt** - Copy of LICENSE file.

  If chip `cpu` has been built.
- **build/vdo_larod_cpu_1_0_0_armv7hf.eap** - Application package .eap file.
- **build/vdo_larod_cpu_1_0_0_LICENSE.txt** - Copy of LICENSE file.

  If chip `edgetpu` has been built.
- **build/vdo_larod_edgetpu_1_0_0_armv7hf.eap** - Application package .eap file.
- **build/vdo_larod_edgetpu_1_0_0_LICENSE.txt** - Copy of LICENSE file.

  If chip `cv25` has been built.
- **build/vdo_larod_cv25_1_0_0_aarch64.eap** - Application package .eap file.
- **build/vdo_larod_cv25_1_0_0_LICENSE.txt** - Copy of LICENSE file.

#### Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

- Click on the tab `Apps` in the device GUI
- Enable `Allow unsigned apps` toggle
- Click `(+ Add app)` button to upload the application file
- Browse to the newly built ACAP application, depending on architecture:
  - `vdo_larod_cv25_1_0_0_aarch64.eap`
  - `vdo_larod_artpec8_1_0_0_aarch64.eap`
  - `vdo_larod_artpec9_1_0_0_aarch64.eap`
  - `vdo_larod_cpu_1_0_0_armv7hf.eap`
  - `vdo_larod_edgetpu_1_0_0_armv7hf.eap`
- Click `Install`
- Run the application by enabling the `Start` switch

The application is now installed on the device and named "vdo_larod_<CHIP>".

### The expected output

Application log can be found directly at:

```sh
http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=vdo_larod
```

Depending on the selected chip, different output is received.

In previous larod versions, the chip was referred to as a number instead of a string. See the table below to understand the mapping:

| Chips | Larod 1 (int) | Larod 3 |
|-------|--------------|------------------|
| CPU with TensorFlow Lite | 2 | cpu-tflite |
| Google TPU | 4 | google-edge-tpu-tflite |
| Ambarella CVFlow (NN) | 6 | ambarella-cvflow |
| ARTPEC-8 DLPU | 12 | axis-a8-dlpu-tflite |
| ARTPEC-9 DLPU | - | a9-dlpu-tflite |

#### Output - ARTPEC-8 with TensorFlow Lite

```sh
----- Contents of SYSTEM_LOG for 'vdo_larod' -----


vdo_larod[584171]: Starting /usr/local/packages/vdo_larod/vdo_larod
vdo_larod[584171]: chooseStreamResolution: We select stream w/h=480 x 270 based on VDO channel info.
vdo_larod[584171]: Creating VDO image provider and creating stream 480 x 270
vdo_larod[584171]: Setting up larod connection with chip axis-a8-dlpu-tflite and model file /usr/local/packages/vdo_larod/model/model.tflite
vdo_larod[584171]: Loading the model... This might take up to 5 minutes depending on your device model.
vdo_larod[584171]: Model loaded successfully
vdo_larod[584171]: Created mmaped model output 0 with size 1
vdo_larod[584171]: Created mmaped model output 1 with size 1
vdo_larod[584171]: Start fetching video frames from VDO

vdo_larod[584171]: Ran pre-processing for 2 ms
vdo_larod[584171]: Ran inference for 16 ms
vdo_larod[584171]: Person detected: 65.14% - Car detected: 11.92%

vdo_larod[4165]: Exit /usr/local/packages/vdo_larod/vdo_larod
```

#### Output - ARTPEC-9 with TensorFlow Lite

```sh
----- Contents of SYSTEM_LOG for 'vdo_larod' -----

vdo_larod[584171]: Starting /usr/local/packages/vdo_larod/vdo_larod
vdo_larod[584171]: chooseStreamResolution: We select stream w/h=480 x 270 based on VDO channel info.
vdo_larod[584171]: Creating VDO image provider and creating stream 480 x 270
vdo_larod[584171]: Setting up larod connection with chip a9-dlpu-tflite and model file /usr/local/packages/vdo_larod/model/model.tflite
vdo_larod[584171]: Loading the model... This might take up to 5 minutes depending on your device model.
vdo_larod[584171]: Model loaded successfully
vdo_larod[584171]: Created mmaped model output 0 with size 1
vdo_larod[584171]: Created mmaped model output 1 with size 1
vdo_larod[584171]: Start fetching video frames from VDO

vdo_larod[584171]: Ran pre-processing for 2 ms
vdo_larod[584171]: Ran inference for 7 ms
vdo_larod[584171]: Person detected: 65.14% - Car detected: 11.92%

vdo_larod[4165]: Exit /usr/local/packages/vdo_larod/vdo_larod
```

#### Output - CPU with TensorFlow Lite

```sh
----- Contents of SYSTEM_LOG for 'vdo_larod' -----

vdo_larod[584171]: Starting /usr/local/packages/vdo_larod/vdo_larod
vdo_larod[584171]: chooseStreamResolution: We select stream w/h=480 x 270 based on VDO channel info.
vdo_larod[584171]: Creating VDO image provider and creating stream 480 x 270
vdo_larod[584171]: Setting up larod connection with chip cpu-tflite and model file /usr/local/packages/vdo_larod/model/model.tflite
vdo_larod[584171]: Loading the model... This might take up to 5 minutes depending on your device model.
vdo_larod[584171]: Model loaded successfully
vdo_larod[584171]: Created mmaped model output 0 with size 1
vdo_larod[584171]: Created mmaped model output 1 with size 1
vdo_larod[584171]: Start fetching video frames from VDO

vdo_larod[584171]: Ran pre-processing for 3 ms
vdo_larod[584171]: Ran inference for 2594 ms
vdo_larod[584171]: Change VDO stream framerate to 1.000000 because of too long inference time
vdo_larod[584171]: Person detected: 65.14% - Car detected: 11.92%

vdo_larod[4165]: Exit /usr/local/packages/vdo_larod/vdo_larod
```

#### Output - Google TPU

```sh
----- Contents of SYSTEM_LOG for 'vdo_larod' -----

vdo_larod[584171]: Starting /usr/local/packages/vdo_larod/vdo_larod
vdo_larod[584171]: chooseStreamResolution: We select stream w/h=256 x 256 based on VDO channel info.
vdo_larod[584171]: Creating VDO image provider and creating stream 256 x 256
vdo_larod[584171]: Setting up larod connection with chip google-edge-tpu-tflite and model file /usr/local/packages/vdo_larod/model/model.tflite
vdo_larod[584171]: Loading the model... This might take up to 5 minutes depending on your device model.
vdo_larod[584171]: Model loaded successfully
vdo_larod[584171]: Created mmaped model output 0 with size 1
vdo_larod[584171]: Created mmaped model output 1 with size 1
vdo_larod[584171]: Start fetching video frames from VDO

vdo_larod[584171]: Ran pre-processing for 2 ms
vdo_larod[584171]: Ran inference for 16 ms
vdo_larod[584171]: Person detected: 65.14% - Car detected: 11.92%

vdo_larod[4165]: Exit /usr/local/packages/vdo_larod/vdo_larod
```

#### Output - CV25

```sh
----- Contents of SYSTEM_LOG for 'vdo_larod' -----


vdo_larod[584171]: Starting /usr/local/packages/vdo_larod/vdo_larod
vdo_larod[584171]: chooseStreamResolution: We select stream w/h=256 x 256 based on VDO channel info.
vdo_larod[584171]: Creating VDO image provider and creating stream 256 x 256
vdo_larod[584171]: Setting up larod connection with chip ambarella-cvflow and model file /usr/local/packages/vdo_larod/model/model.bin
vdo_larod[584171]: Loading the model... This might take up to 5 minutes depending on your device model.
vdo_larod[584171]: Model loaded successfully
vdo_larod[584171]: Created mmaped model output 0 with size 32
vdo_larod[584171]: Created mmaped model output 1 with size 32
vdo_larod[584171]: Start fetching video frames from VDO
vdo_larod[584171]: Ran pre-processing for 1 ms
vdo_larod[584171]: Ran inference for 50 ms
vdo_larod[584171]: Person detected: 65.14% - Car detected: 11.92%

vdo_larod[584171]: Exit /usr/local/packages/vdo_larod/vdo_larod
```

#### A note on performance

Buffers are allocated and tracked by VDO and larod. As such they will
automatically be handled as efficiently as possible. The libyuv backend will
map each buffer once and never copy. The VProc backend and any inference
backend that supports dma-bufs will use that to achieve both zero copy and zero
mapping. Inference backends not supporting dma-bufs will map each buffer once
and never copy just like libyuv. It should also be mentioned that the input
tensors of the inference model will be used as output tensors for the
preprocessing model to avoid copying data.

The application however does no pipelining of preprocessing and inferences, but
uses the synchronous liblarod API call `larodRunJob()` in the interest of
simplicity. One could implement pipelining using `larodRunJobAsync()` and thus
improve performance, but with some added complexity to the program.

#### Conclusion

- This is an example of test data, which is dependent on selected device and chip.
- One full-screen banana has been used for testing.
- Running inference is much faster on ARTPEC-8 and Google TPU in comparison to CPU.
- Converting images takes almost the same time on all chips.
- Objects with score less than 60% are generally not good enough to be used as classification results.

## License

**[Apache License 2.0](../LICENSE)**
