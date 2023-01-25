 *Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A larod based ACAP application running inference on an edge device

This README file explains how to build an ACAP application that uses the [larod API](../FAQs.md#WhatisLarod?). It is achieved by using the containerized API and toolchain images.

Together with this README file, you should be able to find a directory called app. That directory contains the "larod_simple_app" application source code which can easily
be compiled and run with the help of the tools and step by step below.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
larod
├── app
│   ├── input
│   │   └── veiltail-11457_640_RGB_224x224.bin
│   ├── larod_simple_app.c
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── Dockerfile
├── extract_analyze_output.sh
└── README.md
```

* **app/input/veiltail-11457_640_RGB_224x224.bin** - 224x224 raw bitmap image of a goldfish to run inference on.
* **app/larod_simple_app.c** - Example application to load a model and run inference on it.
* **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
* **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
* **app/manifest.json** - Defines the application and its configuration.
* **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
* **extract_analyze_output.sh** - The script shows the matched class in the output.
* **README.md** - Step by step instructions on how to run the example.

### Limitations

* ARTPEC-7 based device with video support is required
* In order to change the binary name, it has to be done in the Makefile.

### How to run the code

Below is the step by step instructions on how to execute the program. So basically starting with the generation of the .eap file to running it on a device:

#### Build the application

Standing in your working directory run the following commands:

> **Note**
>
> Depending on the network you are connected to, you may need to add proxy settings.
The file that needs those settings is:* ~/.docker/config.json. *For
reference please see: <https://docs.docker.com/network/proxy/> and a
[script for Axis device here](../FAQs.md#HowcanIset-upnetworkproxysettingsontheAxisdevice?).*

```sh
docker build --tag <APP_IMAGE> .
```

<APP_IMAGE> is the name to tag the image with, e.g., larod-simple-app:1.0

Copy the result from the container image to a local directory build:

```sh
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

Model and label files are downloaded from <https://coral.ai/models/>, when building the application.

The working dir now contains a build folder with the following files:

```sh
larod
│── app
│   ├── input
│   │   └── veiltail-11457_640_RGB_224x224.bin
│   ├── larod_simple_app.c
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── build
│   ├── input
│   │   └── veiltail-11457_640_RGB_224x224.bin
│   ├── larod_simple_app*
│   ├── larod-simple-app_1_0_0_armv7hf.eap
│   ├── larod-simple-app_1_0_0_LICENSE.txt
│   ├── larod_simple_app.c
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── model
│   │   ├── labels_mobilenet_quant_v1_224.txt
│   │   └── mobilenet_v1_1.0_224_quant.tflite
│   │── package.conf
│   ├── package.conf.orig
│   └── param.conf
├── Dockerfile
├── extract_analyze_output.sh
└── README.md
```

* **build/larod_simple_app** - Application executable binary file.
* **build/larod_simple_app_1_0_0_armv7hf.eap** - Application package .eap file.
* **build/larod_simple_app_1_0_0_LICENSE.txt** - Copy of LICENSE file.
* **build/manifest.json** - Defines the application and its configuration.
* **build/model** - Folder containing label and models files used in this application.
* **build/model/labels_mobilenet_quant_v1_224.txt** - Label file for MobileNet V1.
* **build/model/mobilenet_v1_1.0_224_quant.tflite** - Model file for MobileNet V1.
* **build/package.conf.orig** - Defines the application and its configuration, original file.
* **build/param.conf** - File containing application parameters.

#### Install your application

Installing your application on an Axis video device is as simple as:

Browse to the following page (replace <axis_device_ip> with the IP number of your Axis video device)

```sh
http://<axis_device_ip>/#settings/apps
```

*Goto your device web page above > Click on the tab **App** in the device GUI > Add **(+)** sign and browse to
the newly built **larod_simple_app_1_0_0_armv7hf.eap** > Click **Install** > Run the application by enabling the **Start** switch*

#### The expected output

In order to see the output, please copy the file with the output (veiltail-11457_640_RGB_224x224.bin.out) from the device into your host
application directory.
> [!IMPORTANT]
*> Please make sure SSH is enabled on the device to run the
following commands.*

```sh
scp root@<axis_device_ip>:/usr/local/packages/larod_simple_app/input/veiltail-11457_640_RGB_224x224.bin.out .
```

Run the script **extract_analyze_output.sh** in the same folder where you copied the output file (veiltail-11457_640_RGB_224x224.bin.out) to see the output.
The script will prompt the password required to access the device.

```sh
./extract_analyze_output.sh <axis_device_ip> <Path_to_labels_mobilenet_quant_v1_224.txt>
```

The model class PATH for objects matching is located in the
*larod/build/model/labels_mobilenet_quant_v1_224.txt*

The matched class will be printed below:

**Output:** The model has found that with a probability of 100 % the picture represents a goldfish.

> [!NOTE]
> *This app only supports models with one input and one output
tensor, whereas larod itself supports any number of either.*

## License

**[Apache License 2.0](../LICENSE)**
