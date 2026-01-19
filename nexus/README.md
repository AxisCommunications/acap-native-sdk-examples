*Copyright (C) 2025, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP applications using the Nexus API

This directory contains ACAP application examples that interact with the Nexus API.

## About the Nexus API

Nexus is a publish/subscribe system in AXIS OS. It uses a model which focuses on the data as
the primary element of interaction. This model enables asynchronous communication and decoupling
of producers and consumers. Nexus is not intended to be used for streaming binary data.

An example of a real-world scenario where Nexus would be used is when clients are interested in
information on objects detected by the camera. Information such as object type, position, shape
and color can be published on Nexus, and clients interested in this type of information can
subscribe on Nexus to get this information.

## Example applications

Each example has a README file in its directory which gives an introduction to the
example, shows the directory structure and gives step-by-step instructions on how
to run the application on an Axis device.

- [ACAP Communication](./acap-communication/README.md)
  - This example consists of two applications that communicate with each other.
- [Memory and CPU Utilization](./memory-cpu-utilization/README.md)
  - This example subscribes to two topics and prints the memory and CPU utilization.

## Nexus API documentation

[Link to documentation](https://developer.axis.com/acap/api/src/api/nexus-client-cpp/html/index.html)

## License

**[MIT License](./LICENSE)**
