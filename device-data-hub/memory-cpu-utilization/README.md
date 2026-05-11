*Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP application that prints the memory and CPU utilization

This application subscribes to two public Device Data Hub topics to receive memory and CPU utilization data using the [Device Data Hub API](https://developer.axis.com/acap/api/#device-data-hub-api), which is printed to the log.

> [!NOTE]
> The acap-communication example contains a more in-depth introduction.

## Project structure

The files for building the application are organized in the following structure.

```sh
memory-cpu-utilization
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── memory_cpu_utilization.c
├── Dockerfile
└── README.md
```

- **app/LICENSE** - Open source licensed source code distributed with the application.
- **app/Makefile** - Build and link instructions for the application.
- **app/manifest.json** - Definition of the application and its configuration.
- **app/memory_cpu_utilization.c** - Application source code.
- **Dockerfile** - Dockerfile with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.

## Program structure and behavior

### Connecting to Device Data Hub

The application starts by creating a Device Data Hub client and connecting to the service. The `dh_client_connect` function uses the current Linux user as the username.

### Subscribing to a topic

It then creates a `DHSubscriber` and subscribes to the two topics described above. A `DHSubscriberListener` is also configured to receive data published on those subscribed topics. The received JSON data is printed as-is.

### Output JSON data structure of the topics

Below are the two topics and the structure of the JSON data that is received by this application
for each topic.

<details>
<summary>com.axis.device.memory_utilization.v1</summary>

```json
{
    "mem_available": {
        "type": "integer"
    },
    "mem_total": {
        "type": "integer"
    },
    "mem_used": {
        "type": "integer"
    },
    "mem_used_pct": {
        "type": "integer"
    }
}
```

</details>

<details>
<summary>com.axis.device.cpu_utilization.v1</summary>

```json
{
    "core_utilization": {
        "type": "array",
        "items": {
            "type": "integer"
        }
    },
    "total_utilization": {
        "type": "integer"
    },
    "number_of_cores": {
        "type": "integer"
    }
}
```

</details>
<br>

## The manifest

This application subscribes to public topics. To enable that behavior, Device Data Hub must first be enabled for the application. The following manifest JSON snippet shows how to do this.

```json
"resources": {
    "deviceDataHub_beta2": {
        "enabled": true
    }
}
```

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
chosen, one of the following files should be found:

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
[log prefix] memory_cpu_utilization[14478]: Application started
[log prefix] Received mem cpu data: {"mem_available":587080,"mem_total":981716,"mem_used":394636,"mem_used_pct":40}
[log prefix] Received mem cpu data: {"core_utilization":[17,15,18,17],"number_of_cores":4,"total_utilization":17}
[log prefix] Received mem cpu data: {"mem_available":587196,"mem_total":981716,"mem_used":394520,"mem_used_pct":40}
[log prefix] Received mem cpu data: {"core_utilization":[27,23,27,28],"number_of_cores":4,"total_utilization":26}
[log prefix] memory_cpu_utilization[14478]: Application terminated
```

## License

**[MIT License](./app/LICENSE)**
