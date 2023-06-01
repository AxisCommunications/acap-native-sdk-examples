*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP applications interacting with event system on an edge device

This README file explains how to use the axevent library, which provides an interface to the event system found in Axis device.

The purpose is to provide applications a mechanism to send and receive events.

- **Producer** - Application sends events.
- **Consumer** - Application subscribe events.

Three examples have been added to illustrate some use cases of the event system.

The first example will create an application that subscribes to events, which are predefined on device.
The second example will create an application that declares and sends an ONVIF event that the application in the third example is subscribed to.

A wrapper "get_eventlist.py" is also available for testing the applications. The wrapper supports different ONVIF APIs.
APIs specification is available on <https://www.onvif.org/specs/core/ONVIF-Core-Specification.pdf>

## Getting started

Below is the structure and scripts of the files and folders on the top level:

```sh
axevent
├── get_eventlist.py
├── README.md
├── send_event
├── subscribe_to_event
└── subscribe_to_events
```

- **get_eventlist.py** - Wrapper to find declared and sent events, using ONVIF APIs.
- **README.md** - Step by step instructions on how to use the examples.
- **send_event** - Folder containing files for building ACAP application "send_event".
- **subscribe_to_event** - Folder containing files for building ACAP application "subscribe_to_event".
- **subscribe_to_events** - Folder containing files for building ACAP application "subscribe_to_events".

### Example applications

Each example has as a README file in its directory which shows overview, example directory structure and step-by-step instructions on how to run applications on the device.
Below is the list of examples available in the repository.

- [Send Event](./send_event/README.md)
  - The example code is written in C which sends an ONVIF event periodically.
- [Subscribe to Event](./subscribe_to_event/README.md)
  - The example code is written in C which subscribe to the ONVIF event sent from application "send_event".
- [Subscribe to Events](./subscribe_to_events/README.md)
  - The example code is written in C which subscribes to different predefined events.

### Find events using wrapper

> [!IMPORTANT]
> *The wrapper needs an ONVIF user with password in case wrapper is being used. Browse to the following page (replace <axis_device_ip> with the IP number of your Axis video device)*
>
> ```sh
> http://<axis_device_ip>/#settings/system/security/onvif
> ```

*Goto your device web page above > Click on Add **(+)** sign below the **ONVIF users** in the device GUI >  Add >**Username**, **New password** and **Repeat password** > Click **Save** button*

#### Find declared events using wrapper

The wrapper "get_eventlist.py" helps you save the declared eventlist to an XML-file called "onviflist.xml.new".

```sh
./get_eventlist.py getlist -h
```

#### Find sent events using wrapper

The wrapper also helps you save the sent eventlist to an XML-file called "sentonviflist.xml.new".

```sh
./get_eventlist.py getsent -h
```

### Find events using GStreamer

> [!IMPORTANT]
> *Install GStreamer on your machine by following the instructions here:
<https://gstreamer.freedesktop.org/documentation/installing/on-linux.html?gi-language=c>*

It is also possible to use GStreamer tools for monitoring events, as a complement to the wrapper
(replace \<user\>, \<password\> and \<axis_device_ip\> with the username, password and IP number of your Axis video device).

```sh
gst-launch-1.0 rtspsrc location="rtsp://<user>:<password>@<axis_device_ip>/axis-media/media.amp?video=0&audio=0&event=on" ! fdsink
```

## License

**[Apache License 2.0](../LICENSE)**
