*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP application sending an ONVIF event on an edge device

This README file explains how to build an ACAP application that uses axevent library for sending a stateful ONVIF event.

An ONVIF event is using the namespace "tns1" as the namespace.
Different namespaces is used for a custom defined stateful event. Axis is using namespace "tnsaxis" for Axis defined events.

This example shows an ProcessorUsage ONVIF event with topic namespace "tns1:Monitoring/ProcessorUsage",
according to chapter "8.8.1 Processor Usage" in
<https://www.onvif.org/specs/core/ONVIF-Core-Specification.pdf>

The ONVIF event is being sent with an updated processor usage value every 10th second.

Building the application is achieved by using the containerized API and toolchain images.

Together with this README file you should be able to find a directory called app.
That directory contains the "send_event" application source code, which can easily
be compiled and run with the help of the tools and step by step below.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
send_event
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── send_event.c
├── Dockerfile
└── README.md
```

- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application "send_event".
- **app/manifest.json** - Defines the application and its configuration.
- **app/send_event.c** - Application which sends events, written in C.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example "send_event".
- **README.md** - Step by step instructions on how to run the example.

### How to run the code

Below is the step by step instructions on how to execute the program. So basically starting with the generation of the .eap files to running it on a device:

#### Build the application

Standing in your working directory run the following commands:

> **Note**
>
> Depending on the network you are connected to, you may need to add proxy settings.
> The file that needs these settings is: `~/.docker/config.json`. For reference please see
> https://docs.docker.com/network/proxy and a
> [script for Axis devices](https://axiscommunications.github.io/acap-documentation/docs/develop/build-install-run.html#configure-network-proxy-settings) in the ACAP documentation.

```sh
docker build --tag <APP_IMAGE> .
```

<APP_IMAGE> is the name to tag the image with, e.g., send-event:1.0

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
send_event
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── send_event.c
├── build
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   ├── param.conf
│   ├── send_event*
│   ├── send_event_1_0_0_armv7hf.eap
│   ├── send_event_1_0_0_LICENSE.txt
│   └── send_event.c
├── Dockerfile
└── README.md
```

- **build/manifest.json** - Defines the application and its configuration.
- **build/package.conf** - Defines the application and its configuration.
- **build/package.conf.orig** - Defines the application and its configuration, original file.
- **build/param.conf** - File containing application parameters.
- **build/send_event*** - Application executable binary file.
- **build/send_event_1_0_0_armv7hf.eap** - Application package .eap file for "send_event".
- **build/send_event_1_0_0_LICENSE.txt** - Copy of LICENSE file.

#### Install your application

Installing your application on an Axis video device is as simple as:

Browse to the following page (replace <axis_device_ip> with the IP number of your Axis video device)

```sh
http://<axis_device_ip>/#settings/apps
```

*Goto your device web page above > Click on the tab **App** in the device GUI > Click Add **(+)** sign and browse to
the newly built **send_event_1_0_0_armv7hf.eap** > Click **Install** > Run the application by enabling the **Start** switch*

Application is now available as an application on the device and has been started to send events.

#### The expected output

Application log can be found directly at:

```sh
http://<axis_device_ip>/axis-cgi/admin/systemlog.cgi?appname=send_event
```

or by clicking on the "**App log**" link in the device GUI or by extracting the logs using following commands
in the terminal.
> [!IMPORTANT]
*> Please make sure SSH is enabled on the device to run the
following commands.*

```sh
ssh root@<axis_device_ip>
cd /var/log/
head -200 info.log
```

```sh
----- Contents of SYSTEM_LOG for 'send_event' -----

16:23:56.628 [ INFO ] send_event[0]: starting send_event
16:23:56.670 [ INFO ] send_event[20562]: Started logging from send event application
16:23:56.783 [ INFO ] send_event[20562]: Declaration complete for : 1
16:24:06.956 [ INFO ] send_event[20562]: Send stateful event with value: 0.000000
16:24:16.956 [ INFO ] send_event[20562]: Send stateful event with value: 10.000000
```

A stateful event will be sent every 10th second, changing its value.

### Find events using wrapper

See general information about wrapper "get_eventlist.py" in [README](../README.md).

Replace <onvifuser>, <onvifpassword> and <axis_device_ip> with the ONVIF user, ONVIF password and IP number of your Axis video device.

#### Find declared events using wrapper

The wrapper helps you save the declared eventlist to an XML-file.

```sh
../get_eventlist.py getlist -h
```

In this case ONVIF API is used and an ONVIF username and password needs to be added to the device. E.g

```sh
../get_eventlist.py getlist -u <onvifuser> -p <onvifpassword> -i <axis_device_ip>
```

This output could be compared to the ONVIF event specification chapter "8.8.1 Processor Usage" in
<https://www.onvif.org/specs/core/ONVIF-Core-Specification.pdf>

XML file "onviflist.xml.new" contains:

```xml
        <tns1:Monitoring>
          <ProcessorUsage wstop:topic="true">
            <tt:MessageDescription IsProperty="true">
              <tt:Source>
                <tt:SimpleItemDescription Name="Token" Type="tt:ReferenceToken"/>
              </tt:Source>
              <tt:Data>
                <tt:SimpleItemDescription Name="Value" Type="xs:float"/>
              </tt:Data>
            </tt:MessageDescription>
          </ProcessorUsage>
        </tns1:Monitoring>
```

#### Find sent events using wrapper

The wrapper helps you save the sent eventlist to an XML-file.

```sh
../get_eventlist.py getsent -h
```

In this case ONVIF APIs are used, which means that an ONVIF username and password needs to be added to the device. E.g

```sh
../get_eventlist.py getsent -u <onvifuser> -p <onvifpassword> -i <axis_device_ip>
```

XML file "sentonviflist.xml.new" contains:

```xml
      <wsnt:NotificationMessage>
        <wsnt:Topic Dialect="http://docs.oasis-open.org/wsn/t-1/TopicExpression/Simple">tns1:Monitoring/ProcessorUsage</wsnt:Topic>
        <wsnt:ProducerReference>
          <wsa5:Address>uri://c76646c5-f62b-45eb-9d51-e88723efced2/ProducerReference</wsa5:Address>
        </wsnt:ProducerReference>
        <wsnt:Message>
          <tt:Message UtcTime="2020-08-11T11:48:43Z" PropertyOperation="Initialized">
            <tt:Source>
              <tt:SimpleItem Name="Token" Value="0"/>
            </tt:Source>
            <tt:Key/>
            <tt:Data>
              <tt:SimpleItem Name="Value" Value="100.000000"/>
            </tt:Data>
          </tt:Message>
        </wsnt:Message>
      </wsnt:NotificationMessage>
```

### Find events using GStreamer

See general information about GStreamer tools in [README](../README.md).

If using GStreamer tools for monitoring events
(replace <user>, <password> and <axis_device_ip> with the username, password and IP number of your Axis video device).

```sh
gst-launch-1.0 rtspsrc location="rtsp://<user>:<password>@<axis_device_ip>/axis-media/media.amp?video=0&audio=0&event=on" ! fdsink
```

Output in XML, which has been formatted manually to show topic "tns1:Monitoring/ProcessorUsage":

```xml
<?xml version="1.0" encoding="UTF-8"?>
<tt:MetadataStream xmlns:tt="http://www.onvif.org/ver10/schema">
   <tt:Event>
      <wsnt:NotificationMessage xmlns:wsnt="http://docs.oasis-open.org/wsn/b-2" xmlns:tns1="http://www.onvif.org/ver10/topics" xmlns:tnsaxis="http://www.axis.com/2009/event/topics" xmlns:wsa5="http://www.w3.org/2005/08/addressing">
         <wsnt:Topic Dialect="http://docs.oasis-open.org/wsn/t-1/TopicExpression/Simple">tns1:Monitoring/ProcessorUsage</wsnt:Topic>
         <wsnt:ProducerReference>
            <wsa5:Address>uri://c76646c5-f62b-45eb-9d51-e88723efced2/ProducerReference</wsa5:Address>
         </wsnt:ProducerReference>
         <wsnt:Message>
            <tt:Message UtcTime="2020-08-11T11:50:23.947409Z" PropertyOperation="Changed">
               <tt:Source>
                  <tt:SimpleItem Name="Token" Value="0" />
               </tt:Source>
               <tt:Key />
               <tt:Data>
                  <tt:SimpleItem Name="Value" Value="90.000000" />
               </tt:Data>
            </tt:Message>
         </wsnt:Message>
      </wsnt:NotificationMessage>
   </tt:Event>
</tt:MetadataStream>
```

### Generate Axis events

As opposed to ONVIF events, Axis events are visible in the device web page for events, reachable at:

```sh
https://<device_ip>/camera/index.html#/system/events/rules.
```

In order to produce an Axis event, you have to use the `tnsaxis` namespace, and rename the `topic0` value to `CameraApplicationPlatform`.
This example use ONVIF events, to modify it to use Axis events, change the event declaration in function `setup_declaration` of [send_event.c](./app/send_event.c) to:

```c
ax_event_key_value_set_add_key_value(key_value_set, "topic0", "tnsaxis",
                                     "CameraApplicationPlatform", AX_VALUE_TYPE_STRING, NULL);
ax_event_key_value_set_add_key_value(key_value_set, "topic1", "tnsaxis", "ProcessorUsage",
                                     AX_VALUE_TYPE_STRING, NULL);
ax_event_key_value_set_add_key_value(key_value_set, "Token", "tnsaxis", &token,
                                     AX_VALUE_TYPE_INT, NULL);
ax_event_key_value_set_add_key_value(key_value_set, "Value", "tnsaxis", &start_value,
                                     AX_VALUE_TYPE_DOUBLE, NULL);
```

## License

**[Apache License 2.0](../../LICENSE)**
