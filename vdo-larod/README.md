*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A combined VDO stream and Larod based ACAP application running inference on an edge device

This README file explains how to build an ACAP application that uses:

- the [Video capture API (VDO)](https://axiscommunications.github.io/acap-documentation/docs/api/native-sdk-api.html#video-capture-api-vdo) to fetch frames from e.g. a camera
- the [Machine learning API (Larod)](https://axiscommunications.github.io/acap-documentation/docs/api/native-sdk-api.html#machine-learning-api-larod) to load a graph model and run preprocessing and classification inferences

It is achieved by using the containerized API and toolchain images.

Together with this README file you should be able to find a directory called app. That directory contains the "vdo_larod" application source code, which can easily be compiled and run with the help of the tools and step by step below.

## Detailed outline of example application

This application opens a client to VDO and starts fetching frames (in a new thread) in the YUV format. It tries to find the smallest VDO stream resolution that fits the width and height required by the neural network. The thread fetching frames is written so that it always tries to provide a frame as new as possible even if not all previous frames have been processed by larod.

Steps in application:

1. Fetch image data from VDO.
2. Preprocess the images (crop to 224x224, scale and color convert) using larod with either the libyuv or VProc backend (depending on platform).
3. Run MobileNetV2 inferences on a specific chip with the preprocessing output as input on a larod backend specified by a command line argument.
4. Sort the classification scores output by the MobileNetV2 inferences and print the top scores along with corresponding indices and [ImageNet][1] labels to stdout.
5. Repeat for 5 iterations.

See the manifest.json.* files to change the configuration on chip, image size, number of iterations, model path and label path.

## Which backends and models are supported?

Unless you modify the app to your own needs you should only use a MobileNetV2 model that takes 224x224 RGB images as input, and that outputs an array of 1001 classification scores of type `uint8` or `float32`.

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
│   ├── manifest.json.cpu
│   ├── manifest.json.cv25
│   ├── manifest.json.edgetpu
│   └── vdo_larod.c
├── Dockerfile
└── README.md
```

- **app/imgprovider.c/h** - Implementation of vdo parts, written in C.
- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json.artpec8** - Defines the application and its configuration when building for DLPU with TensorFlow Lite.
- **app/manifest.json.cpu** - Defines the application and its configuration when building for CPU with TensorFlow Lite.
- **app/manifest.json.cv25** - Defines the application and its configuration when building chip and model for cv25 DLPU.
- **app/manifest.json.edgetpu** - Defines the application and its configuration when building chip and model for Google TPU.
- **app/vdo_larod.c** - Application using larod, written in C.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.

## Limitations

- ARTPEC-7 based device with edge TPU
- ARTPEC-8
- CV25
- This application was not written to optimize performance
- MobileNet is good for classification, but it requires that the object you want to classify should cover almost all the frame

## How to run the code

Below is the step by step instructions on how to execute the program. So basically starting with the generation of the .eap file to running it on a device:

### Build the application

> **Note**
>
> Depending on the network you are connected to, you may need to add proxy settings.
> The file that needs these settings is: `~/.docker/config.json`. For reference please see
> https://docs.docker.com/network/proxy and a
> [script for Axis devices](https://axiscommunications.github.io/acap-documentation/docs/develop/build-install-run.html#configure-network-proxy-settings) in the ACAP documentation.

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

```sh
docker build --tag <APP_IMAGE> --build-arg CHIP=<CHIP> .
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

- \<APP_IMAGE\> is the name to tag the image with, e.g., vdo_larod:1.0
- \<CHIP\> is the chip type. Supported values are *artpec8*, *cpu*, *cv25* and *edgetpu*.
- \<ARCH\> is the architecture. Supported values are armv7hf (default) and aarch64

See the following sections for build commands for each chip.

#### Build for ARTPEC-8 with Tensorflow Lite

To build a package for ARTPEC-8 with Tensorflow Lite, run the following commands standing in your working directory:

```sh
docker build --build-arg ARCH=aarch64 --build-arg CHIP=artpec8 --tag <APP_IMAGE> .
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
│   ├── manifest.json.cpu
│   ├── manifest.json.edgetpu
│   ├── manifest.json.cv25
│   ├── model
|   │   ├── imagenet_labels.txt
|   │   └── mobilenet_v2_1.0_224_quant.tflite / mobilenet_v2_1.0_224_quant_edgetpu.tflite / mobilenet_v2_cavalry.bin
│   ├── package.conf
│   ├── package.conf.orig
│   ├── param.conf
│   ├── vdo_larod*
│   ├── vdo_larod_{cpu,edgetpu}_1_0_0_armv7hf.eap / vdo_larod_{cv25,artpec8}_1_0_0_aarch64.eap
│   ├── vdo_larod_{cpu,edgetpu}_1_0_0_LICENSE.txt / vdo_larod_{cv25,artpec8}_1_0_0_LICENSE.txt
│   └── vdo_larod.c
```

- **build/manifest.json** - Defines the application and its configuration.
- **build/model** - Folder containing models used in this application.
- **build/model/imagenet_labels.txt** - Label file for MobileNet V2 (ImageNet).
- **build/model/mobilenet_v2_1.0_224_quant_edgetpu.tflite** - Model file for MobileNet V2 (ImageNet), used for Google TPU.
- **build/model/mobilenet_v2_1.0_224_quant.tflite** - Model file for MobileNet V2 (ImageNet), used for ARTPEC-8 and CPU with TensorFlow Lite.
- **build/model/mobilenet_v2_cavalry.bin** - Model file for MobileNet V2 (ImageNet), used for ARTPEC-8 and CV25.
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
- **build/vdo_larod_cv25_1_0_0_aarch64.eap** - Application package .eap file.
- **build/vdo_larod_cv25_1_0_0_LICENSE.txt** - Copy of LICENSE file.

### Install your application

Installing your application on an Axis video device is as simple as:

Browse to the following page (replace <axis_device_ip> with the IP number of your Axis video device)

```sh
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

Depending on the selected chip, different output is received.

In previous larod versions, the chip was referred to as a number instead of a string. See the table below to understand the mapping:

| Chips | Larod 1/2 (int) | Larod 3 (string) |
|-------|--------------|------------------|
| CPU with TensorFlow Lite | 2 | cpu-tflite |
| Google TPU | 4 | google-edge-tpu-tflite |
| Ambarella CVFlow (NN) | 6 | ambarella-cvflow |
| ARTPEC-8 DLPU | 12 | axis-a8-dlpu-tflite |

#### Output - ARTPEC-8 with TensorFlow Lite

```sh
----- Contents of SYSTEM_LOG for 'vdo_larod' -----


vdo_larod[4165]: Starting /usr/local/packages/vdo_larod/vdo_larod
vdo_larod[4165]: 'buffer.strategy': <uint32 3>
vdo_larod[4165]: 'channel': <uint32 1>
vdo_larod[4165]: 'format': <uint32 3>
vdo_larod[4165]: 'height': <uint32 240>
vdo_larod[4165]: 'width': <uint32 320>
vdo_larod[4165]: Creating VDO image provider and creating stream 320 x 240
vdo_larod[4165]: Dump of vdo stream settings map =====
vdo_larod[4165]: chooseStreamResolution: We select stream w/h=320 x 240 based on VDO channel info.
vdo_larod[4165]: Calculate crop image
vdo_larod[4165]: Create larod models
vdo_larod[4165]: Create preprocessing maps
vdo_larod[4165]: Crop VDO image X=40 Y=0 (240 x 240)
vdo_larod[4165]: Setting up larod connection with chip axis-a8-dlpu-tflite, model /usr/local/packages/vdo_larod/model/mobilenet_v2_1.0_224_quant.tflite and label file /usr/local/packages/vdo_larod/model
vdo_larod[4165]: Available chip ids:
vdo_larod[4165]: Chip: cpu-tflite
vdo_larod[4165]: Chip: axis-ace-proc
vdo_larod[4165]: Chip: cpu-proc
vdo_larod[4165]: Chip: axis-a8-dlpu-tflite
vdo_larod[4165]: Chip: axis-a8-dlpu-native
vdo_larod[4165]: Chip: axis-a8-dlpu-proc
vdo_larod[4165]: Chip: axis-a8-gpu-proc
vdo_larod[4165]: Allocate memory for input/output buffers
vdo_larod[4165]: Connect tensors to file descriptors
vdo_larod[4165]: Create input/output tensors
vdo_larod[4165]: Create job requests
vdo_larod[4165]: Determine tensor buffer sizes
vdo_larod[4165]: Start fetching video frames from VDO
vdo_larod[4165]: Converted image in 6 ms
vdo_larod[4165]: Ran inference for 8 ms
vdo_larod[4165]: Top result:  955  banana with score 96.20%
vdo_larod[4165]: Converted image in 2 ms
vdo_larod[4165]: Ran inference for 7 ms
vdo_larod[4165]: Top result:  955  banana with score 97.40%
vdo_larod[4165]: Converted image in 3 ms
vdo_larod[4165]: Ran inference for 7 ms
vdo_larod[4165]: Top result:  955  banana with score 97.80%
vdo_larod[4165]: Converted image in 2 ms
vdo_larod[4165]: Ran inference for 7 ms
vdo_larod[4165]: Top result:  955  banana with score 98.20%
vdo_larod[4165]: Converted image in 2 ms
vdo_larod[4165]: Ran inference for 7 ms
vdo_larod[4165]: Stop streaming video from VDO
vdo_larod[4165]: Top result:  955  banana with score 98.00%
vdo_larod[4165]: Exit /usr/local/packages/vdo_larod/vdo_larod
```

#### Output - CPU with TensorFlow Lite

```sh
----- Contents of SYSTEM_LOG for 'vdo_larod' -----

Starting /usr/local/packages/vdo_larod/vdo_larod
vdo_larod[1670]: 'buffer.strategy': <uint32 3>
vdo_larod[1670]: 'channel': <uint32 1>
vdo_larod[1670]: 'format': <uint32 3>
vdo_larod[1670]: 'height': <uint32 240>
vdo_larod[1670]: 'width': <uint32 320>
vdo_larod[1670]: Creating VDO image provider and creating stream 320 x 240
vdo_larod[1670]: Dump of vdo stream settings map =====
vdo_larod[1670]: chooseStreamResolution: We select stream w/h=320 x 240 based on VDO channel info.
vdo_larod[1670]: Calculate crop image
vdo_larod[1670]: Create larod models
vdo_larod[1670]: Create preprocessing maps
vdo_larod[1670]: Crop VDO image X=40 Y=0 (240 x 240)
vdo_larod[1670]: Setting up larod connection with chip cpu-tflite, model /usr/local/packages/vdo_larod/model/mobilenet_v2_1.0_224_quant.tflite and label file /usr/local/packages/vdo_larod/model/imagenet_labels.txt
vdo_larod[1670]: Available chip ids:
vdo_larod[1670]: Chip: cpu-tflite
vdo_larod[1670]: Chip: google-edge-tpu-tflite
vdo_larod[1670]: Chip: axis-ace-proc
vdo_larod[1670]: Chip: cpu-proc
vdo_larod[1670]: Chip: axis-a7-gpu-proc
vdo_larod[1670]: Allocate memory for input/output buffers
vdo_larod[1670]: Connect tensors to file descriptors
vdo_larod[1670]: Create input/output tensors
vdo_larod[1670]: Create job requests
vdo_larod[1670]: Determine tensor buffer sizes
vdo_larod[1670]: Start fetching video frames from VDO
vdo_larod[1670]: Converted image in 6 ms
vdo_larod[1670]: Ran inference for 293 ms
vdo_larod[1670]: Top result:  955  banana with score 77.20%
vdo_larod[1670]: Converted image in 5 ms
vdo_larod[1670]: Ran inference for 224 ms
vdo_larod[1670]: Top result:  955  banana with score 76.80%
vdo_larod[1670]: Converted image in 5 ms
vdo_larod[1670]: Ran inference for 217 ms
vdo_larod[1670]: Top result:  955  banana with score 76.00%
vdo_larod[1670]: Converted image in 10 ms
vdo_larod[1670]: Ran inference for 213 ms
vdo_larod[1670]: Top result:  955  banana with score 75.60%
vdo_larod[1670]: Converted image in 5 ms
vdo_larod[1670]: Ran inference for 221 ms
vdo_larod[1670]: Stop streaming video from VDO
vdo_larod[1670]: Top result:  955  banana with score 75.20%
vdo_larod[1670]: Exit /usr/local/packages/vdo_larod/vdo_larod
```

#### Output - Google TPU

```sh
----- Contents of SYSTEM_LOG for 'vdo_larod' -----

 Starting /usr/local/packages/vdo_larod/vdo_larod
vdo_larod[31476]: 'buffer.strategy': <uint32 3>
vdo_larod[31476]: 'channel': <uint32 1>
vdo_larod[31476]: 'format': <uint32 3>
vdo_larod[31476]: 'height': <uint32 240>
vdo_larod[31476]: 'width': <uint32 320>
vdo_larod[31476]: Creating VDO image provider and creating stream 320 x 240
vdo_larod[31476]: Dump of vdo stream settings map =====
vdo_larod[31476]: chooseStreamResolution: We select stream w/h=320 x 240 based on VDO channel info.
vdo_larod[31476]: Calculate crop image
vdo_larod[31476]: Create larod models
vdo_larod[31476]: Create preprocessing maps
vdo_larod[31476]: Crop VDO image X=40 Y=0 (240 x 240)
vdo_larod[31476]: Setting up larod connection with chip google-edge-tpu-tflite, model /usr/local/packages/vdo_larod/model/mobilenet_v2_1.0_224_quant_edgetpu.tflite and label file /usr/local/packages/vdo_larod/model/imagenet_labels.txt
vdo_larod[31476]: Available chip ids:
vdo_larod[31476]: Chip: axis-a7-gpu-tflite
vdo_larod[31476]: Chip: cpu-tflite
vdo_larod[31476]: Chip: google-edge-tpu-tflite
vdo_larod[31476]: Chip: axis-ace-proc
vdo_larod[31476]: Chip: cpu-proc
vdo_larod[31476]: Chip: axis-a7-gpu-proc
vdo_larod[31476]: Allocate memory for input/output buffers
vdo_larod[31476]: Connect tensors to file descriptors
vdo_larod[31476]: Create input/output tensors
vdo_larod[31476]: Create job requests
vdo_larod[31476]: Determine tensor buffer sizes
vdo_larod[31476]: Start fetching video frames from VDO
vdo_larod[31476]: Converted image in 6 ms
vdo_larod[31476]: Ran inference for 22 ms
vdo_larod[31476]: Top result:  955  banana with score 97.20%
vdo_larod[31476]: Converted image in 5 ms
vdo_larod[31476]: Ran inference for 5 ms
vdo_larod[31476]: Top result:  955  banana with score 98.40%
vdo_larod[31476]: Converted image in 6 ms
vdo_larod[31476]: Ran inference for 8 ms
vdo_larod[31476]: Top result:  955  banana with score 99.20%
vdo_larod[31476]: Converted image in 4 ms
vdo_larod[31476]: Ran inference for 5 ms
vdo_larod[31476]: Top result:  955  banana with score 99.20%
vdo_larod[31476]: Converted image in 6 ms
vdo_larod[31476]: Ran inference for 4 ms
vdo_larod[31476]: Stop streaming video from VDO
vdo_larod[31476]: Top result:  955  banana with score 99.20%
vdo_larod[31476]: Exit /usr/local/packages/vdo_larod/vdo_larod```
```

#### Output - CV25

```sh
----- Contents of SYSTEM_LOG for 'vdo_larod' -----


vdo_larod[5100]: setupLarod: Unable to load model: Could not load model: Could not initialize net
vdo_larod[584171]: Starting /usr/local/packages/vdo_larod/vdo_larod
vdo_larod[584171]: 'buffer.strategy': <uint32 3>
vdo_larod[584171]: 'channel': <uint32 1>
vdo_larod[584171]: 'format': <uint32 3>
vdo_larod[584171]: 'height': <uint32 240>
vdo_larod[584171]: 'width': <uint32 320>
vdo_larod[584171]: Creating VDO image provider and creating stream 320 x 240
vdo_larod[584171]: Dump of vdo stream settings map =====
vdo_larod[584171]: chooseStreamResolution: We select stream w/h=320 x 240 based on VDO channel info.
vdo_larod[584171]: Calculate crop image
vdo_larod[584171]: Create larod models
vdo_larod[584171]: Create preprocessing maps
vdo_larod[584171]: Crop VDO image X=40 Y=0 (240 x 240)
vdo_larod[584171]: Setting up larod connection with chip ambarella-cvflow, model /usr/local/packages/vdo_larod/model/mobilenet_v2_cavalry.bin and label file /usr/local/packages/vdo_larod/model/imagenet_labels.txt
vdo_larod[584171]: Available chip ids:
vdo_larod[584171]: Chip: ambarella-cvflow
vdo_larod[584171]: Chip: ambarella-cvflow-proc
vdo_larod[584171]: Chip: cpu-tflite
vdo_larod[584171]: Chip: cpu-proc
vdo_larod[584171]: Allocate memory for input/output buffers
vdo_larod[584171]: Connect tensors to file descriptors
vdo_larod[584171]: Create input/output tensors
vdo_larod[584171]: Determine tensor buffer sizes
vdo_larod[584171]: createAndMapTmpFile: Setting up a temp fd with pattern /tmp/larod.in.test-XXXXXX and size 150528
vdo_larod[584171]: createAndMapTmpFile: Setting up a temp fd with pattern /tmp/larod.out.test-XXXXXX and size 32032
vdo_larod[584171]: createAndMapTmpFile: Setting up a temp fd with pattern /tmp/larod.pp.test-XXXXXX and size 115200
vdo_larod[584171]: Create job requests
vdo_larod[584171]: Start fetching video frames from VDO
vdo_larod[584171]: Converted image in 4 ms
vdo_larod[584171]: Ran inference for 6 ms
vdo_larod[584171]: Top result:  955  banana with score 56.81%
vdo_larod[584171]: Converted image in 4 ms
vdo_larod[584171]: Ran inference for 6 ms
vdo_larod[584171]: Top result:  955  banana with score 69.17%
vdo_larod[584171]: Converted image in 4 ms
vdo_larod[584171]: Ran inference for 6 ms
vdo_larod[584171]: Top result:  955  banana with score 72.00%
vdo_larod[584171]: Converted image in 4 ms
vdo_larod[584171]: Ran inference for 6 ms
vdo_larod[584171]: Top result:  955  banana with score 75.61%
vdo_larod[584171]: Converted image in 4 ms
vdo_larod[584171]: Ran inference for 6 ms
vdo_larod[584171]: Stop streaming video from VDO
vdo_larod[584171]: Top result:  955  banana with score 76.88%
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

[1]: http://www.image-net.org/
