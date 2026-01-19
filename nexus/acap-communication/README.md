*Copyright (C) 2025, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# Asynchronous communication between two applications

This is an example that shows the communication between two applications, one that
writes data to a topic and another that receives the data written to the topic. The two
applications in this example are *object_detector* and *object_consumer*.

## Introduction

The purpose of this example is to show the communication between two applications that
use the Nexus library. The application *object_detector* will create a topic and then
write data to the topic if there are any consumers. The application *object_consumer*
will subscribe to the topic and print the received data to the system log.

[Link to *object_detector*](./object-detector/README.md)

[Link to *object_consumer*](./object-consumer/README.md)

## How to run this example

1. First, read and build [object_detector](./object-detector/README.md) - this creates the topic and publishes data.
2. Then, read and build [object_consumer](./object-consumer/README.md) - this subscribes and receives data.
3. Install both applications on the same Axis device.
4. Start the applications.

## Directory structure

The files for building the example are organized in the following structure.

```sh
acap-communication
├── object-consumer
│   ├── app
│   │   ├── LICENSE
│   │   ├── Makefile
│   │   ├── manifest.json
│   │   └── object_consumer.cpp
│   ├── Dockerfile
│   └── README.md
├── object-detector
│   ├── app
│   │   ├── LICENSE
│   │   ├── Makefile
│   │   ├── manifest.json
│   │   └── object_detector.cpp
│   ├── Dockerfile
│   └── README.md
└── README.md
```

- **object-consumer/app/LICENSE** - List of all open source licensed source code distributed with the specified application.
- **object-consumer/app/Makefile** - Build and link instructions for the specified application.
- **object-consumer/app/manifest.json** - Definition of the *object_consumer* application and its configuration.
- **object-consumer/app/object_consumer.cpp** - Source code for the *object_consumer* application.
- **object-consumer/Dockerfile** - Dockerfile with the specified Axis toolchain and API container to build the application specified.
- **object-consumer/README.md** - Step by step instructions on how to run the *object_consumer* application.
- **object-detector/app/LICENSE** - List of all open source licensed source code distributed with the specified application.
- **object-detector/app/Makefile** - Build and link instructions for the specified application.
- **object-detector/app/manifest.json** - Definition of the *object_detector* application and its configuration.
- **object-detector/app/object_detector.cpp** - Source code for the *object_detector* application.
- **object-detector/Dockerfile** - Dockerfile with the specified Axis toolchain and API container to build the application specified.
- **object-detector/README.md** - Step by step instructions on how to run the *object_detector* application.
- **README.md** - Information about the *acap-communication example*.

## Nexus API documentation

[Link to documentation](https://developer.axis.com/acap/api/src/api/nexus-client-cpp/html/index.html)

## License

**[MIT License](../LICENSE)**
