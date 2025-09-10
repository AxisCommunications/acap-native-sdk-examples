*Copyright (C) 2025, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# A pipewire stream based ACAP application on an edge device

This README file explains how to build an ACAP application that uses the [pipewire stream API](https://docs.pipewire.org/page_streams.html#ssec_produce). It is achieved by using the containerized API and toolchain images.

Together with this README file, you should be able to find a directory called app. That directory contains the "audioplayback" application source code which can easily be compiled and run with the help of the tools and step by step below.

This example illustrates how to continuously play audio samples to the pipewire service by filling pipewire buffers with samples of a sine wave.

The naming convention of the audio nodes in pipewire is described in the [Native SDK API](https://developer.axis.com/acap/api/native-sdk-api/#pipewire)

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
audio-playback
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── audioplayback.c
├── Dockerfile
└── README.md
```

- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json** - Defines the application and its configuration.
- **app/audioplayback.c** - Application to play audio to the pipewire service in C.
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

<APP_IMAGE> is the name to tag the image with, e.g., audioplayback:1.0

Default architecture is **armv7hf**. To build for **aarch64** it's possible to
update the *ARCH* variable in the Dockerfile or to set it in the `docker build`
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
audio-playback
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── audioplayback.c
├── build
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   ├── param.conf
│   ├── audioplayback*
│   ├── Audio_playback_1_0_0_armv7hf.eap
│   ├── Audio_playback_1_0_0_LICENSE.txt
│   └── audioplayback.c
├── Dockerfile
└── README.md
```

- **build/manifest.json** - Defines the application and its configuration.
- **build/package.conf** - Defines the application and its configuration.
- **build/package.conf.orig** - Defines the application and its configuration, original file.
- **build/param.conf** - File containing application parameters.
- **build/audioplayback*** - Application executable binary file.
- **build/Audio_playback_1_0_0_armv7hf.eap** - Application package .eap file.
- **build/Audio_playback_1_0_0_LICENSE.txt** - Copy of LICENSE file.

#### Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

- Click on the tab `Apps` in the device GUI
- Enable `Allow unsigned apps` toggle
- Click `(+ Add app)` button to upload the application file
- Browse to the newly built ACAP application, depending on architecture:
  - `Audio_playback_1_0_0_aarch64.eap`
  - `Audio_playback_1_0_0_armv7hf.eap`
- Click `Install`
- Run the application by enabling the `Start` switch

#### The expected output

Application log can be found directly at:

```sh
http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=audioplayback
```

#### Output

```sh
----- Contents of SYSTEM_LOG for 'audioplayback' -----

audioplayback[91929]: I audioplayback [audioplayback.c:333:main]: Starting.
audioplayback[91929]: D audioplayback [audioplayback.c:203:registry_event_global]: Ignore node AudioDevice0Input0.Unprocessed with id 70.
audioplayback[91929]: I audioplayback [audioplayback.c:201:registry_event_global]: Found node AudioDevice0Output0 with id 81.
audioplayback[91929]: D audioplayback [audioplayback.c:105:on_state_changed]: State for stream from AudioDevice0Output0 changed unconnected -> connecting
audioplayback[91929]: D audioplayback [audioplayback.c:203:registry_event_global]: Ignore node AudioDevice0Input0 with id 114.
audioplayback[91929]: D audioplayback [audioplayback.c:105:on_state_changed]: State for stream from AudioDevice0Output0 changed connecting -> paused
audioplayback[91929]: I audioplayback [audioplayback.c:90:on_param_changed]: Playing to node AudioDevice0Output0, rate 48000.
audioplayback[91929]: D audioplayback [audioplayback.c:105:on_state_changed]: State for stream from AudioDevice0Output0 changed paused -> streaming
```

## License

**[Apache License 2.0](../LICENSE)**
