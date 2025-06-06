*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# An ACAP application subscribing to multiple events

This README file explains how to build an ACAP application that uses the axevent API. It is achieved by using the containerized API and toolchain images.

Together with this README file, you should be able to find a directory called app. That directory contains the "subscribe_to_events" application source code which can easily
be compiled and run with the help of the tools and step by step below.

This example illustrates how to subscribe to a number of events in an Axis device and how you may easily trigger them.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
subscribe_to_events
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── subscribe_to_events.c
├── Dockerfile
└── README.md
```

- **app/LICENSE** - File containing the license conditions.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json** - Defines the application and its configuration.
- **app/subscribe_to_events.c** - Example application.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.

### Limitations

- Which events that are available varies between Axis products, e.g.
  video-related events will not be available on a loudspeaker. It's however
  possible to subscribe to such events that will never be sent, this doesn't
  give any error.

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

<APP_IMAGE> is the name to tag the image with, e.g., subscribe-to-events:1.0

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
subscribe_to_events
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── subscribe_to_events.c
├── build
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   ├── param.conf
│   ├── README.md
│   ├── subscribe_to_events*
│   ├── subscribe_to_events_1_0_0_armv7hf.eap
│   ├── subscribe_to_events_1_0_0_LICENSE.txt
│   └── subscribe_to_events.c
├── Dockerfile
└── README.md
```

- **build/manifest.json** - Defines the application and its configuration.
- **build/package.conf** - Defines the application and its configuration.
- **build/package.conf.orig** - Defines the application and its configuration, original file.
- **build/param.conf** - File containing application parameters.
- **build/subscribe_to_events*** - Application executable binary file.
- **build/subscribe_to_events_1_0_0_armv7hf.eap** - Application package .eap file.
- **build/subscribe_to_events_1_0_0_LICENSE.txt** - Copy of LICENSE file.

#### Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

- Click on the tab `Apps` in the device GUI
- Enable `Allow unsigned apps` toggle
- Click `(+ Add app)` button to upload the application file
- Browse to the newly built ACAP application, depending on architecture:
  - `subscribe_to_events_1_0_0_aarch64.eap`
  - `subscribe_to_events_1_0_0_armv7hf.eap`
- Click `Install`
- Run the application by enabling the `Start` switch

### Test application - trigger events

The application has subscribed to the following events:

- *Manual Trigger* - Sends an event when manually clicking a switch button in device GUI.
- *Tampering* - Sends a stateless event when image is tampered with, like putting a cloth over camera. (Requires video)
- *Day/Night* - Sends an event when light conditions change and switching to day/night mode. (Requires video)
- *PTZ* - Sends an event when any of PTZ (Pan/Tilt/Zoom) channels 1-8 are moving (Requires video and PTZ)
- *Audio* - Sends an event when audio level pass below or above trigger level (Requires audio)

Start off by opening the device web page, follow the instruction for each event
on how to trigger it.

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

#### The expected output

In this example, when an event take place an INFO status message is printed to
the syslog.

Open a second web browser window and open the application log, update the web
page every time you expect a new event to have occurred.

```sh
http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=subscribe_to_events
```

#### Manual trigger event

In device web page, click on the tab **System** in the device GUI > Click **Events** > Click **Manual triggers** > Click the switch to create an event

#### Tampering event

Try to put some large object in front of camera lens, but not blocking out all light.

#### Day/night event

If you work in a dark room, turn on the light. If you work in a light room, cover completely with a cloth, which should also trigger a tampering event.

#### PTZ event

In device web page, set up a view area in tab **View area** >  Click **New** and then click the newly created **View area 1** > In the popup, enable PTZ and click Done.

Now clicking on another tab, e.g. **PTZ** or **Apps** tab, the window with streamed video should have a zoom slider, moving this will zoom in/out and cause a PTZ Move event.

#### Audio event

Depending on Axis product, enable/connect a microphone. If there is a physical *Audio port*, a simple mobile phone headset with microphone will do.

In device web page, click on tab **Audio** and click **Allow Audio** > Play around with settings, especially **Automatic gain control** and see if you can trigger an event by talking or shouting.

### Find events using wrapper

See general information about wrapper "get_eventlist.py" in [README](../README.md).

```sh
../get_eventlist.py getlist --help
```

Compare how the subscriptions in subscribe_to_events.c are made to the output in the .xml-file from the wrapper.

## License

**[Apache License 2.0](../../LICENSE)**
