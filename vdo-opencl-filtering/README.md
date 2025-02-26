*Copyright (C) 2022, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# VDO stream combined with OpenCL filtering based ACAP application on an edge device

This README file explains how to build an ACAP application that uses the vdostream API. It is achieved by using the containerized Axis API and toolchain images.

Together with this README file, you should be able to find a directory called app. That directory contains the "vdo_cl_filter_demo" application source code which can easily be compiled and run with the help of the tools and step by step below.

This example illustrates how to capture frames from the vdo service, access the received buffer, and finally perform a GPU accelerated Sobel filtering with OpenCL.
Here, the GPU access the image buffer in a zero-copy fashion, which otherwise may be a bottleneck.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure used in the example:

```sh
vdo-opencl-filtering
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── sobel_nv12.cl
│   └── vdo_cl_filter_demo.c
├── Dockerfile
└── README.md
```

- **app/LICENSE** - License for source code
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json** - Defines the application and its configuration.
- **app/sobel_nv12.cl** - OpenCL program containing definitions and operations for Sobel filtering kernels.
- **app/vdo_cl_filter_demo.c** - Application to capture the frames using vdo service, setting up OpenCL, and processing the image, in C.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.

### Limitations

The example is done for a captured video stream in YUV NV12 format. For different stream formats the OpenCL program must be altered.

This example requires OpenCL 1.2 with GPU acceleration, see the [ACAP API page](https://developer.axis.com/acap/api/native-sdk-api/#opencl) for more information.

### How to run the code

Below is the step by step instructions on how to execute the program. So basically starting with the generation of the .eap file to running it on a device:

#### Build the application

Standing in your working directory run the following commands:

> [!NOTE]
>
> Depending on the network your local build machine is connected to, you may need to add proxy
> settings for Docker. See
> [Proxy in build time](https://developer.axis.com/acap/develop/proxy/#proxy-in-build-time).

```sh
docker build --tag <APP_IMAGE> --build-arg ARCH=<ARCH> .
```

- <APP_IMAGE> is the name to tag the image with, e.g., vdo_cl_filter_demo:1.0
- \<ARCH\> is the architecture of the camera you are using, e.g., armv7hf (default) or aarch64

Copy the result from the container image to a local directory build:

```sh
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

The working directory now contains a build folder with the following files:

```sh
vdo-opencl-filtering
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── sobel_nv12.cl
│   └── vdo_cl_filter_demo.c
├── build
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   ├── param.conf
│   ├── sobel_nv12.cl
│   ├── vdo_cl_filter_demo*
│   ├── vdo_cl_filter_demo_1_0_0_<ARCH>.eap
│   ├── vdo_cl_filter_demo_1_0_0_LICENSE.txt
│   └── vdo_cl_filter_demo.c
├── Dockerfile
└── README.md
```

- **build/manifest.json** - Defines the application and its configuration.
- **build/package.conf** - Defines the application and its configuration.
- **build/package.conf.orig** - Defines the application and its configuration, original file.
- **build/param.conf** - File containing application parameters.
- **build/vdo_cl_filter_demo*** - Application executable binary file.
- **build/vdo_cl_filter_demo_1_0_0_\<ARCH\>.eap** - Application package .eap file.
- **build/vdo_cl_filter_demo_1_0_0_LICENSE.txt** - Copy of LICENSE file.

#### Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

- Click on the tab `Apps` in the device GUI
- Enable `Allow unsigned apps` toggle
- Click `(+ Add app)` button to upload the application file
- Browse to the newly built ACAP application, depending on architecture:
  - `vdo_cl_filter_demo_1_0_0_aarch64.eap`
  - `vdo_cl_filter_demo_1_0_0_armv7hf.eap`
- Click `Install`
- Run the application by enabling the `Start` switch

#### Program output

> [!IMPORTANT]
> To run the commands below, see section [Access the
> device](../DEV.md#access-the-device) for setup.

The application will create an output file on the device under
`/usr/local/packages/vdo_cl_filter_demo/localdata/cl_vdo_demo.yuv` by default.
To see the content of the file

```sh
ssh my-ssh-user@<AXIS_DEVICE_IP>
cat /usr/local/packages/vdo_cl_filter_demo/localdata/cl_vdo_demo.yuv

(Content of file)
```

## License

**[Apache License 2.0](../LICENSE)**
