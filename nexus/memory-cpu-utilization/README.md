*Copyright (C) 2025, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP application that prints the memory and CPU utilization

This application subscribes to two public topics and in this way gets the memory and CPU
utilization, which are printed to the log.

> [!NOTE]
> The acap-communication example contains a more in-depth introduction.

## Introduction

The purpose of this example is to show how to consume memory and CPU utilization data by
subscribing to two public topics. It is also shown how to include the *nlohmann*
library in an ACAP application and use it to parse JSON data.

Below are the two topics and the structure of the JSON data that is received by this application
for each topic. Click on the topic to expand the JSON schema. Note that for CPU utilization,
only the total utilization is printed.

<details>
<summary>axis.device.memory_utilization_v1</summary>

```json
{
    "mem_total": {
        "type": "integer"
    },
    "mem_used": {
        "type": "integer"
    },
    "mem_available": {
        "type": "integer"
    },
    "mem_used_pct": {
        "type": "integer"
    }
}
```

</details>

<details>
<summary>axis.device.cpu_utilization_v1</summary>

```json
{
    "total_utilization": {
        "type": "integer"
    },
    "core_utilization": {
        "type": "array",
        "items": {
            "type": "integer"
        }
    },
    "number_of_cores": {
        "type": "integer"
    }
}
```

</details>
<br>

The application begins with creating a client and connecting. The `Connect` function will use the
current Linux user as the username.

The application will then create a `TopicDataSubscriber` and use it to subscribe to the two
topics described above. The `ResourceUtilizationLogger` listener is used to receive data that is
written to the topics that have been subscribed to.

For the memory utilization, the received JSON data is printed exactly as it is. For the CPU
utilization, we parse the received JSON data in order to show how to use the *nlohmann*
library. Then we get the total utilization and print it.

## Directory structure

The files for building the application are organized in the following structure.

```sh
memory-cpu-utilization
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── memory_cpu_utilization.cpp
├── Dockerfile
└── README.md
```

- **app/LICENSE** - List of all open source licensed source code distributed with the application.
- **app/Makefile** - Build and link instructions for the application.
- **app/manifest.json** - Definition of the application and its configuration.
- **app/memory_cpu_utilization.cpp** - Application source code.
- **Dockerfile** - Dockerfile with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.

## The manifest

This application subscribes to public topics, and to be able to do that, Nexus first needs
to be enabled for this application. Below is the JSON code in the manifest that does that.

```json
"resources": {
    "nexus_beta1": {
        "enabled": true
    }
}
```

## Nexus API documentation

[Link to documentation](https://developer.axis.com/acap/api/src/api/nexus-client-cpp/html/index.html)

## Build the application

> [!NOTE]
>
> For detailed information on how to build, install, and run ACAP applications, refer to the official ACAP documentation: [Build, install, and run](https://developer.axis.com/acap/develop/build-install-run/).
>
> Depending on the network your local build machine is connected to, you may need to add proxy
> settings for Docker. See
> [Proxy in build time](https://developer.axis.com/acap/develop/proxy/#proxy-in-build-time).

Standing in your working directory, run the following commands:

```sh
docker build --platform=linux/amd64 --build-arg ARCH=<ARCH> --tag <APP_IMAGE> .
```

- `<ARCH>` is the SDK architecture, `armv7hf` or `aarch64`.
- `<APP_IMAGE>` is the name to tag the image with, e.g., `memcpu:1.0`

Copy the result from the container image to a local directory `build`:

```sh
docker cp $(docker create --platform=linux/amd64 <APP_IMAGE>):/opt/app ./build
```

The `build` directory contains the build artifacts, where the ACAP application
is found with the suffix `.eap`. Depending on which SDK architecture that was
chosen, one of following files should be found:

- `memory_cpu_utilization_1_0_0_aarch64.eap`
- `memory_cpu_utilization_1_0_0_armv7hf.eap`

## Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

- Click on the tab `Apps` in the device GUI
- Enable `Allow unsigned apps` toggle
- Click `(+ Add app)` button to upload the application file
- Browse to the newly built ACAP application, and select it
- Click `Install`
- Run the application by enabling the `Start` switch

## Expected output

To find the application log, browse to the *Apps* page, click on the
three dots to the right of the application and select `App log`.

The application will print a memory utilization message every five seconds and a CPU utilization message
every five seconds. Here is an example of what you may see in the log:

```text
[log prefix] Received memory utilization message: {"mem_available":694108,"mem_total":981716,"mem_used":287608,"mem_used_pct":29}
[log prefix] Received CPU utilization message. Total utilization: 14
[log prefix] Received memory utilization message: {"mem_available":694360,"mem_total":981716,"mem_used":287356,"mem_used_pct":29}
[log prefix] Received CPU utilization message. Total utilization: 8
[log prefix] Received memory utilization message: {"mem_available":694112,"mem_total":981716,"mem_used":287604,"mem_used_pct":29}
[log prefix] Received CPU utilization message. Total utilization: 6
```

## License

**[MIT License](./app/LICENSE)**
