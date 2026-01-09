*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A guide to building and running a custom library in an ACAP application

This README file explains how to build a user defined custom library from source files and bundle it for use in an ACAP. The example application uses the custom library and prints "Hello World!" to the syslog of the device.

Together with this README file, you should be able to find a directory called app. That directory contains the custom_lib_example application source code which can easily
be compiled and run with the help of the tools.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
custom_lib_example
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── customlib_example.c
│   └── custom_build
├── Dockerfile
└── README.md
```

- **app/LICENSE**             - File containing the license conditions.
- **app/Makefile**            - Build and link instructions for the application.
- **app/manifest.json**       - Defines the application and its configuration.
- **app/customlib_example.c** - Example application.
- **build/custom_build**      - Folder containing custom library source files
- **Dockerfile**              - Assembles an image containing the ACAP Native SDK and builds the application using it.
- **README.md**               - Step by step instructions on how to run the example.

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
docker build --platform=linux/amd64 --tag <APP_IMAGE> .
```

<APP_IMAGE> is the name to tag the image with, e.g., mainfunc:1.0

The default architecture is **armv7hf**. To build for **aarch64** it's possible to
update the *ARCH* variable in the Dockerfile or to set it in the `docker build`
command via build argument:

```sh
docker build --platform=linux/amd64 --build-arg ARCH=aarch64 --tag <APP_IMAGE> .
```

Copy the result from the container image to a local directory build:

```sh
docker cp $(docker create --platform=linux/amd64 <APP_IMAGE>):/opt/app ./build
```

The working dir now contains a build folder with the following files:

```sh
├── build
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   ├── param.conf
│   ├── customlib_example*
│   ├── lib
│   │   ├── libcustom.so@
│   │   ├── libcustom.so.1@
│   │   └── libcustom.so.1.0.o*
│   ├── custom_build
│   ├── customlib_example_1_0_0_armv7hf.eap
│   ├── customlib_example_1_0_0_LICENSE.txt
│   └── customlib_example.c

```

- **build/package.conf** - Defines the application and its configuration.
- **build/package.conf.orig** - Defines the application and its configuration, original file.
- **build/param.conf** - File containing application parameters.
- **build/customlib_example*** - Application executable binary file.
- **build/lib** - Folder containing compiled library files for custom library
- **build/customlib_example_1_0_0_armv7hf.eap** - Application package .eap file.
- **build/customlib_example_1_0_0_LICENSE.txt** - Copy of LICENSE file.

#### Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

1. Click on the tab **Apps** in the device GUI
2. Enable **Allow unsigned apps** toggle
3. Click **(+ Add app)** button to upload the application file
4. Select the newly built application package, depending on architecture:

   - `customlib_example_1_0_0_aarch64.eap`
   - `customlib_example_1_0_0_armv7hf.eap`

5. Click **Install**
6. Run the application by enabling the **Start** switch

Application custom_lib_example is now available as an application on the device.

### Custom library application

The application will use a user-defined custom library and print "Hello World!" to the syslog of the device.

#### The expected output

The application log can be found at:

```sh
http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=mainfunc
```

```sh
----- Contents of SYSTEM_LOG for 'mainfunc' -----

2021-08-19T14:22:28.068+02:00 axis-accc8ef26bcf [ INFO    ] customlib_example[32561]: Hello World!

```

## License

**[Apache License 2.0](../../LICENSE)**
