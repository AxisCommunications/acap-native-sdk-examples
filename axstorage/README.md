# How to manage storage disks in an ACAP application

This guide explains how to build an ACAP application that uses the axstorage API. This example illustrates how to handle storage disks. It is possible to list, setup and release storage devices, subscribe to different events and write data. This examples shows how to do all of the above and, if available, writes to two files in the SD card every 10 seconds.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
axstorage
├── app
│   ├── axstorage.c
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── Dockerfile
└── README.md
```

- **app/axstorage.c** - Application to show API in C.
- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json** - Defines the application and its configuration.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.

### How to run the code

Below is the step by step instructions on how to execute the program. So basically starting with the generation of the .eap file to running it on a device.

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

`<APP_IMAGE>` is the name to tag the image with, e.g., `axstorage:1.0`

Default architecture is **armv7hf**. To build for **aarch64** it's possible to
update the *ARCH* variable in the Dockerfile or to set it in the `docker build`
command via build argument:

```sh
docker build --platform=linux/amd64 --build-arg ARCH=aarch64 --tag <APP_IMAGE> .
```

Copy the result from the container image to a local directory called `build`:

```sh
docker cp $(docker create --platform=linux/amd64 <APP_IMAGE>):/opt/app ./build
```

The working directory now contains a build folder with the following files:

```sh
axstorage
build
├── Makefile
├── manifest.json
├── package.conf
├── package.conf.orig
├── param.conf
├── axstorage*
├── axstorage_1_0_0_armv7hf.eap
├── axstorage_1_0_0_LICENSE.txt
├── axstorage.c
└── LICENSE
```

- **manifest.json** - Defines the application and its configuration.
- **package.conf** - Defines the application and its configuration.
- **package.conf.orig** - Defines the application and its configuration, original file.
- **param.conf** - File containing additional application parameters.
- **axstorage*** - Application executable binary file.
- **axstorage_1_0_0_armv7hf.eap** - Application package .eap file.
- **axstorage_1_0_0_LICENSE.txt** - Copy of LICENSE file.

> [!NOTE]
>
> For detailed information on how to build, install, and run ACAP applications, refer to the official ACAP documentation: [Build, install, and run](https://developer.axis.com/acap/develop/build-install-run/).

#### Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

1. Click on the tab **Apps** in the device GUI
2. Enable **Allow unsigned apps** toggle
3. Click **(+ Add app)** button to upload the application file
4. Select the newly built application package, depending on architecture:

   - `axstorage_1_0_0_aarch64.eap`
   - `axstorage_1_0_0_armv7hf.eap`

5. Click **Install**
6. Run the application by enabling the **Start** switch

#### The expected output

Application log can be found directly at `http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=axstorage` or by clicking on the "**App log**" link in the device GUI.

```sh
----- Contents of SYSTEM_LOG for 'axstorage' -----
16:40:53.234 [ INFO ] axstorage[1234]: Start AXStorage application
16:40:53.234 [ INFO ] axstorage[1234]: Subscribe for the events of NetworkShare
16:40:53.234 [ INFO ] axstorage[1234]: Subscribe for the events of SD_DISK
16:40:53.234 [ INFO ] axstorage[1234]: Status of events for NetworkShare: writable NO, available NO, exiting NO, full NO
16:40:53.234 [ INFO ] axstorage[1234]: Status of events for SD_DISK: writable YES, available YES, exiting NO, full NO
16:40:53.234 [ INFO ] axstorage[1234]: Setup SD_DISK
16:40:53.234 [ INFO ] axstorage[1234]: Disk: SD_DISK has been setup in /var/spool/storage/areas/SD_DISK/axstorage
16:40:53.234 [ INFO ] axstorage[1234]: Setup of SD_DISK was successful
16:41:03.342 [ INFO ] axstorage[1234]: Writing to /var/spool/storage/areas/SD_DISK/axstorage/file1.log
16:40:03.342 [ INFO ] axstorage[1234]: Writing to /var/spool/storage/areas/SD_DISK/axstorage/file2.log
16:40:13.536 [ INFO ] axstorage[1234]: Writing to /var/spool/storage/areas/SD_DISK/axstorage/file1.log
16:40:13.536 [ INFO ] axstorage[1234]: Writing to /var/spool/storage/areas/SD_DISK/axstorage/file2.log
...
```

If your camera doesn't have a SD card available, the application won't be able to write any files.

When the application is stopped, you will see how the application unsubscribes from the disks and releases the objects:

```sh
16:47:53.807 [ INFO ] axstorage[1234]: Unsubscribed events of NetworkShare
16:47:53.807 [ INFO ] axstorage[1234]: Unsubscribed events of SD_DISK
16:47:53.807 [ INFO ] axstorage[1234]: Release of SD_DISK was successful
16:47:53.807 [ INFO ] axstorage[1234]: Finish AXStorage application
```

## License

**[Apache License 2.0](../LICENSE)**
