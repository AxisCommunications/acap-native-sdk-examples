*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A hello-world ACAP application written in shell script using manifest

This README file explains how to build a simple shell script manifest ACAP application. It is achieved by using the containerized API and toolchain images.

Together with this README file, you should be able to find a directory called app. That directory contains the "shell-script-example" shell script file which can easily be run with the help of the tools and step by step below.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
shell-script-example
├── app
│   ├── shell_script_example
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── Dockerfile
└── README.md
```

- **app/shell_script_example** - Shell script application which writes "Hello World!" to system-log.
- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Empty Makefile. Necessary for the build process.
- **app/manifest.json** - Defines the application and its configuration.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.

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
docker build --tag <APP_IMAGE> .
```

<APP_IMAGE> is the name to tag the image with, e.g., shell_script_example:1.0

Copy the result from the container image to a local directory build:

```sh
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

The working dir now contains a build folder with the following files:

```sh
shell-script-example
├── app
│   ├── shell_script_example
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── build
│   ├── shell_script_example*
│   ├── shell_script_example_1_0_0_armv7hf.eap
│   ├── shell_script_example_1_0_0_LICENSE.txt
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   └── param.conf
├── Dockerfile
└── README.md
```

- **build/shell_script_example*** - Application shell script file.
- **build/shell_script_example_1_0_0_armv7hf.eap** - Application package .eap file.
- **build/shell_script_example_1_0_0_LICENSE.txt** - Copy of LICENSE file.
- **build/manifest.json** - Defines the application and its configuration.
- **build/package.conf** - Defines the application and its configuration.
- **build/package.conf.orig** - Defines the application and its configuration, original file.
- **build/param.conf** - File containing application parameters.

#### Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

- Click on the tab `Apps` in the device GUI
- Enable `Allow unsigned apps` toggle
- Click `(+ Add app)` button to upload the application file
- Browse to the newly built ACAP application, depending on architecture:
  - `shell_script_example_1_0_0_aarch64.eap`
  - `shell_script_example_1_0_0_armv7hf.eap`
- Click `Install`
- Run the application by enabling the `Start` switch

#### The expected output

Application log can be found directly at:

```sh
http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=shell_script_example
```

```sh
----- Contents of SYSTEM_LOG for 'shell_script_example' -----

14:13:07.412 [ INFO ] shell_script_example[6425]: Hello World!

```

## License

**[Apache License 2.0](../LICENSE)**
