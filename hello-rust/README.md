*Copyright (C) 2025, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

> [!Important]
>
> Rust is not among the [supported languages](https://developer.axis.com/acap/acap-sdk-version-3/develop-applications/supported-languages/) for ACAP development.
> However, the SDK can package any executable, and Rust’s [platform support](https://doc.rust-lang.org/beta/rustc/platform-support.html) includes both `aarch64` and `armv7hf`.
> Additionally, the community-driven project [acap-rs](https://github.com/AxisCommunications/acap-rs) offers tools to simplify ACAP app development in Rust.

# A hello-world ACAP application using manifest and Rust

This README file explains how to build a simple Hello World manifest ACAP application using Rust. It is achieved by using the containerized API and toolchain images.

Together with this README file, you should be able to find a directory called app. That directory contains the "hello-rust" application source code which can easily be compiled and run with the help of the tools and step by step below.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
hello-rust
├── Dockerfile
├── README.md
└── app
    ├── Cargo.lock
    ├── Cargo.toml
    ├── LICENSE
    ├── Makefile
    ├── hello_rust
    ├── manifest.json
    └── src
        └── main.rs
```

- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.
- **app/Cargo.lock** - Cargo lockfile specifying the exact version of any dependencies to allow reproducible builds.
- **app/Cargo.toml** - Cargo manifest specifying dependencies, etc.
- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json** - Defines the application and its configuration.
- **app/src/main.rs** - Hello World application which writes to system-log.

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

<APP_IMAGE> is the name to tag the image with, e.g., hello_rust:1.0

Default architecture is **armv7hf**. To build for **aarch64** it's possible to
update the *ARCH* variable in the Dockerfile or to set it in the docker build
command via build argument:

```sh
docker build --build-arg ARCH=aarch64 --tag <APP_IMAGE> .
```

Copy the result from the container image to a local directory build:

```sh
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

The working dir now contains a build folder with the following files:

```sh
hello-rust
.
├── Dockerfile
├── README.md
├── app
│   ├── Cargo.lock
│   ├── Cargo.toml
│   ├── LICENSE
│   ├── Makefile
│   ├── hello_rust
│   ├── manifest.json
│   └── src
│       └── main.rs
└── build
    ├── Cargo.lock
    ├── Cargo.toml
    ├── LICENSE
    ├── Makefile
    ├── hello_rust
    ├── hello_rust_1_0_0_LICENSE.txt
    ├── hello_rust_1_0_0_aarch64.eap
    ├── manifest.json
    ├── package.conf
    ├── package.conf.orig
    ├── param.conf
    └── src
        └── main.rs
```

- **build/hello_rust*** - Application executable binary file.
- **build/hello_rust_1_0_0_aarch64.eap** - Application package .eap file.
- **build/hello_rust_1_0_0_LICENSE.txt** - Copy of LICENSE file.
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
  - `hello_rust_1_0_0_aarch64.eap`
  - `hello_rust_1_0_0_armv7hf.eap`
- Click `Install`
- Run the application by enabling the `Start` switch

#### The expected output

Application log can be found directly at:

```sh
http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=hello_rust
```

```sh
----- Contents of SYSTEM_LOG for 'hello_rust' -----

2025-04-17T20:24:33.559+00:00 axis-b8a44fc09016 [ INFO    ] hello_rust[140843]: Hello stderr!
2025-04-17T20:24:33.559+00:00 axis-b8a44fc09016 [ INFO    ] hello_rust[140843]: Hello stdout!
```

## License

**[Apache License 2.0](../LICENSE)**
