*Copyright (C) 2025, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A pipewire stream based ACAP application on an edge device

This README file explains how to build an ACAP application that uses the [pipewire stream API](https://docs.pipewire.org/page_streams.html#ssec_consume). It is achieved by using the containerized API and toolchain images.

Together with this README file, you should be able to find a directory called app. That directory contains the "audiocapture" application source code which can easily be compiled and run with the help of the tools and step by step below.

This example illustrates how to continuously capture audio samples from the pipewire service, access the received buffer contents as well as the audio metadata. Peak level is calculated from the captured samples and logged in the Application log.

The naming convention of the audio nodes in pipewire is described in the [Native SDK API](https://developer.axis.com/acap/api/native-sdk-api/#pipewire)

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
audio-capture
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── audiocapture.c
├── Dockerfile
└── README.md
```

- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json** - Defines the application and its configuration.
- **app/audiocapture.c** - Application to capture audio from the pipewire service in C.
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
docker build --platform=linux/amd64 --tag <APP_IMAGE> .
```

<APP_IMAGE> is the name to tag the image with, e.g., audiocapture:1.0

Default architecture is **armv7hf**. To build for **aarch64** it's possible to
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
audio-capture
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── audiocapture.c
├── build
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   ├── param.conf
│   ├── audiocapture*
│   ├── Audio_capture_1_0_0_armv7hf.eap
│   ├── Audio_capture_1_0_0_LICENSE.txt
│   └── audiocapture.c
├── Dockerfile
└── README.md
```

- **build/manifest.json** - Defines the application and its configuration.
- **build/package.conf** - Defines the application and its configuration.
- **build/package.conf.orig** - Defines the application and its configuration, original file.
- **build/param.conf** - File containing application parameters.
- **build/audiocapture*** - Application executable binary file.
- **build/Audio_capture_1_0_0_armv7hf.eap** - Application package .eap file.
- **build/Audio_capture_1_0_0_LICENSE.txt** - Copy of LICENSE file.

> [!NOTE]
>
> For detailed information on how to build, install, and run ACAP applications, refer to the official ACAP documentation: [Build, install, and run](https://developer.axis.com/acap/develop/build-install-run/).

#### Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

- Click on the tab `Apps` in the device GUI
- Enable `Allow unsigned apps` toggle
- Click `(+ Add app)` button to upload the application file
- Browse to the newly built ACAP application, depending on architecture:
  - `Audio_capture_1_0_0_aarch64.eap`
  - `Audio_capture_1_0_0_armv7hf.eap`
- Click `Install`
- Run the application by enabling the `Start` switch

#### The expected output

Application log can be found directly at:

```sh
http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=audiocapture
```

#### Output

```sh
----- Contents of SYSTEM_LOG for 'audiocapture' -----

audiocapture[1346447]: I audiocapture [audiocapture.c:368:main]: Starting.
audiocapture[1346447]: I audiocapture [audiocapture.c:235:registry_event_global]: Found Audio/Source node AudioDevice0Input0.Unprocessed with id 90.
audiocapture[1346447]: D audiocapture [audiocapture.c:114:on_state_changed]: State for stream from AudioDevice0Input0.Unprocessed changed unconnected -> connecting
audiocapture[1346447]: I audiocapture [audiocapture.c:235:registry_event_global]: Found Audio/Source node AudioDevice0Input0 with id 134.
audiocapture[1346447]: D audiocapture [audiocapture.c:114:on_state_changed]: State for stream from AudioDevice0Input0 changed unconnected -> connecting
audiocapture[1346447]: I audiocapture [audiocapture.c:235:registry_event_global]: Found Audio/Sink node AudioDevice0Output0 with id 146.
audiocapture[1346447]: D audiocapture [audiocapture.c:114:on_state_changed]: State for stream from AudioDevice0Output0 changed unconnected -> connecting
audiocapture[1346447]: D audiocapture [audiocapture.c:114:on_state_changed]: State for stream from AudioDevice0Input0.Unprocessed changed connecting -> paused
audiocapture[1346447]: D audiocapture [audiocapture.c:114:on_state_changed]: State for stream from AudioDevice0Input0 changed connecting -> paused
audiocapture[1346447]: D audiocapture [audiocapture.c:114:on_state_changed]: State for stream from AudioDevice0Output0 changed connecting -> paused
audiocapture[1346447]: I audiocapture [audiocapture.c:98:on_param_changed]: Capturing from node AudioDevice0Input0.Unprocessed, 2 channel(s), rate 48000.
audiocapture[1346447]: D audiocapture [audiocapture.c:114:on_state_changed]: State for stream from AudioDevice0Input0.Unprocessed changed paused -> streaming
audiocapture[1346447]: I audiocapture [audiocapture.c:98:on_param_changed]: Capturing from node AudioDevice0Input0, 1 channel(s), rate 48000.
audiocapture[1346447]: D audiocapture [audiocapture.c:114:on_state_changed]: State for stream from AudioDevice0Input0 changed paused -> streaming
audiocapture[1346447]: I audiocapture [audiocapture.c:98:on_param_changed]: Capturing from node AudioDevice0Output0, 1 channel(s), rate 48000.
audiocapture[1346447]: D audiocapture [audiocapture.c:114:on_state_changed]: State for stream from AudioDevice0Output0 changed paused -> streaming
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0.Unprocessed, channel 0, peak -42.1 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0, channel 0, peak -19.8 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Output0, channel 0, peak -inf dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0.Unprocessed, channel 0, peak -56.8 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0, channel 0, peak -6.8 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Output0, channel 0, peak -inf dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0.Unprocessed, channel 0, peak -68.4 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0, channel 0, peak -18.4 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Output0, channel 0, peak -inf dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0.Unprocessed, channel 0, peak -66.2 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0, channel 0, peak -16.2 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Output0, channel 0, peak -inf dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0.Unprocessed, channel 0, peak -58.9 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0, channel 0, peak -8.9 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Output0, channel 0, peak -inf dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0.Unprocessed, channel 0, peak -69.3 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0, channel 0, peak -19.3 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Output0, channel 0, peak -inf dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0.Unprocessed, channel 0, peak -68.8 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0, channel 0, peak -18.8 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Output0, channel 0, peak -inf dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0.Unprocessed, channel 0, peak -69.6 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0, channel 0, peak -19.6 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Output0, channel 0, peak -inf dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0.Unprocessed, channel 0, peak -67.0 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0, channel 0, peak -17.0 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Output0, channel 0, peak -inf dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0.Unprocessed, channel 0, peak -39.5 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0, channel 0, peak 10.5 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Output0, channel 0, peak -inf dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0.Unprocessed, channel 0, peak -62.7 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Input0, channel 0, peak -12.7 dBFS.
audiocapture[1346447]: I audiocapture [audiocapture.c:184:on_timeout]: Node AudioDevice0Output0, channel 0, peak -inf dBFS.
```

## License

**[Apache License 2.0](../LICENSE)**
