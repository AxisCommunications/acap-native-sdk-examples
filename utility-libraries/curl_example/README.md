 *Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A guide to building and running libcurl on ACAP3
This README file explains how to build libcurl from source and bundle it for the use in an ACAP. The example application uses the libcurl library to fetch data from
URL and store the data in the application directory on the Camera.

Together with this README file, you should be able to find a directory called app. That directory contains the "curl_example" application source code which can easily
be compiled and run with the help of the tools and step by step below.

APIs specification is available on https://curl.se/libcurl/c

## Getting started
These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```bash
curl_example
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── curl_example.c   
├── Dockerfile        
└── README.md
```

* **app/LICENSE**        - File containing the license conditions.
* **app/Makefile**       - Makefile containing the build and link instructions for building the ACAP3 application.
* **app/manifest.json    - Defines the application and its configuration.
* **app/curl_example.c** - Example application.
* **Dockerfile**         - Docker file with the specified Axis toolchain and API container to build the example specified.
* **README.md**          - Step by step instructions on how to run the example.

### How to run the code
Below is the step by step instructions on how to execute the program. So basically starting with the generation of the .eap file to running it on a device:

#### Build the application
Standing in your working directory run the following commands:

> [!IMPORTANT]
> *Depending on the network you are connected to.
The file that needs those settings is: *~/.docker/config.json.*
For reference please see: https://docs.docker.com/network/proxy/ and a
[script for Axis device here](../FAQs.md#HowcanIset-upnetworkproxysettingsontheAxisdevice?).*

```bash
docker build --tag <APP_IMAGE> .
```

<APP_IMAGE> is the name to tag the image with, e.g., curl_example:1.0

Default architecture is **armv7hf**. To build for **aarch64** it's possible to
update the *ARCH* variable in the Dockerfile or to set it in the docker build
command via build argument:
```bash
docker build --build-arg ARCH=aarch64 --tag <APP_IMAGE> .
```

If the device is inside a network with a proxy, then it can be passed on as a build argument:
```bash
docker build --build-arg CURL_PROXY=<my_proxy> --tag <APP_IMAGE> .
```

Copy the result from the container image to a local directory build:

```bash
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

The working dir now contains a build folder with the following files:

```bash
├── build
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   ├── param.conf
│   ├── curl_example*
│   ├── lib
│   │   ├── libcurl.so@
│   │   ├── libcurl.so.4@
│   │   └── libcurl.so.4.7.0*
│   ├── curl_example_1_0_0_armv7hf.eap
│   ├── curl_example_1_0_0_LICENSE.txt
│   └── curl_example.c
 
```

* **build/package.conf** - Defines the application and its configuration.
* **build/package.conf.orig** - Defines the application and its configuration, original file.
* **build/param.conf** - File containing application parameters.
* **build/curl_example*** - Application executable binary file.
* **build/lib** - Folder containing compiled library files for libcurl
* **build/curl_example_1_0_0_armv7hf.eap** - Application package .eap file.
* **build/curl_example_1_0_0_LICENSE.txt** - Copy of LICENSE file.

#### Install your application
Installing your application on an Axis device is as simple as:

Browse to the following page (replace <axis_device_ip> with the IP number of your Axis device)

```bash
http://<axis_device_ip>/#settings/apps
```

*Goto your device web page above > Click on the tab **App** in the device GUI > Click Add **(+)** sign and browse to
the newly built **curl_example_1_0_0_armv7hf.eap** > Click **Install** > Run the application by enabling the **Start** switch*

Application curl_example is now available as an application on the device.

### libcurl application
The application will fetch the content from "https://ajax.googleapis.com/ajax/libs/jquery/3.6.0/jquery.min.js" and store the content at /usr/local/packages/curl_example/localdata/jquery.min.js on the camera.

#### The expected output
In this example, when start is enabled specific URL file/content will be copied.

>[!IMPORTANT]
*> Please make sure SSH is enabled on the device to run the following commands.*

Fetched URL content can be verified using below command to check that the file contains data.

```bash
ssh root@<axis_device_ip>
ls -la /usr/local/packages/curl_example/localdata/
cat /usr/local/packages/curl_example/localdata/jquery.min.js 
```

## License
**[Apache License 2.0](../../LICENSE)**
