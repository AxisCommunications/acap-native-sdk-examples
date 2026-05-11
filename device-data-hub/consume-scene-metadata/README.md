*Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP application consuming AXIS Scene Metadata

This application subscribes to public Device Data Hub topic `com.axis.scene.object_track.v1` using the [Device Data Hub API](https://developer.axis.com/acap/api/#device-data-hub-api) and
prints received frame-by-frame scene metadata to the log.

> [!NOTE]
> The acap-communication example contains a more in-depth introduction.

## Project structure

The files for building the application are organized in the following structure.

```sh
consume-scene-metadata
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── consume_scene_metadata.c
├── Dockerfile
└── README.md
```

- **app/LICENSE** - Open source licensed source code distributed with the application.
- **app/Makefile** - Build and link instructions for the application.
- **app/manifest.json** - Definition of the application and its configuration.
- **app/consume_scene_metadata.c** - Application source code.
- **Dockerfile** - Dockerfile with the specified Axis toolchain and API container to build the example.
- **README.md** - Step by step instructions on how to run the example.

## Program structure and behavior

### Connecting to Device Data Hub

The application begins by creating a Device Data Hub client and connecting. The `dh_client_connect` function uses the current Linux user as the username.

### Subscribing to a topic

The application then creates a `DHSubscriber` and uses it to subscribe to the Device Data Hub topic described above. It also creates a `DHSubscriberListener`, which is used to receive data written to the subscribed topic. The received JSON data is printed as-is.

### Output JSON data structure of the topics

Below is an example payload structure that this application receives.

<details>
<summary>com.axis.scene.object_track.v1</summary>

```json
{
  "channel_id": 1,
  "classes": [
    {
      "colors": [
        { "name": "blue", "score": 0.7316 },
        { "name": "gray", "score": 0.1604 },
        { "name": "black", "score": 0.154 },
        { "name": "white", "score": 0.1204 },
        { "name": "green", "score": 0.0952 },
        { "name": "red", "score": 0.052 },
        { "name": "yellow", "score": 0.0348 }
      ],
      "score": 0.4848,
      "type": "Truck"
    },
    {
      "colors": [
        { "name": "blue", "score": 0.7316 },
        { "name": "gray", "score": 0.1604 },
        { "name": "black", "score": 0.154 },
        { "name": "white", "score": 0.1204 },
        { "name": "green", "score": 0.0952 },
        { "name": "red", "score": 0.052 },
        { "name": "yellow", "score": 0.0348 }
      ],
      "score": 0.4812,
      "type": "Car"
    },
    {
      "colors": [
        { "name": "blue", "score": 0.7316 },
        { "name": "gray", "score": 0.1604 },
        { "name": "black", "score": 0.154 },
        { "name": "white", "score": 0.1204 },
        { "name": "green", "score": 0.0952 },
        { "name": "red", "score": 0.052 },
        { "name": "yellow", "score": 0.0348 }
      ],
      "score": 0.064,
      "type": "Bus"
    },
    {
      "score": 0.0376,
      "type": "VehicleOther"
    }
  ],
  "duration": 2.499,
  "end_time": "2026-03-20T10:17:48.024761Z",
  "id": "25436c51-6a75-4f23-99dc-7ddd638e4e61",
  "image": {
    "crop_box": {
      "bottom": 0.7349,
      "left": 0.6769,
      "right": 0.7983,
      "top": 0.6139
    },
    "data": "/9j/4AAQSkZJRgABAQAAAQABAAD/.../2Q=="
  },
  "parts": [
    {
      "object_track_id": "7ef28e6e-7cae-4186-b630-23819d0bfd98"
    }
  ],
  "path": [
    {
      "bounding_box": {
        "bottom": 0.786,
        "left": 0.0199,
        "right": 0.0937,
        "top": 0.7161
      },
      "timestamp": "2026-03-20T10:17:45.524804Z"
    },
    .
    .
    .
    {
      "bounding_box": {
        "bottom": 0.653,
        "left": 0.9492,
        "right": 0.999,
        "top": 0.5683
      },
      "timestamp": "2026-03-20T10:17:47.924763Z"
    },
    {
      "bounding_box": {
        "bottom": 0.6446,
        "left": 0.9833,
        "right": 0.999,
        "top": 0.5598
      },
      "timestamp": "2026-03-20T10:17:48.024761Z"
    }
  ],
  "start_time": "2026-03-20T10:17:45.524804Z"
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
- `<APP_IMAGE>` is the name to tag the image with, e.g., `consume_scene_metadata:1.0`

Copy the result from the container image to a local directory `build`:

```sh
docker cp $(docker create --platform=linux/amd64 <APP_IMAGE>):/opt/app ./build
```

The `build` directory contains the build artifacts, where the ACAP application
is found with the suffix `.eap`. Depending on which SDK architecture that was
chosen, one of the following files should be found:

- `consume_scene_metadata_1_0_0_aarch64.eap`
- `consume_scene_metadata_1_0_0_armv7hf.eap`

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

> [!IMPORTANT]
> The logged output can have data which is empty or has a detection.
> To get a detection, make sure to have an object that can be detected by
> the Axis device scene data. Examples are persons and cars.

Here is an example of what you may see in the log:

```json
{
  "channel_id": 1,
  "classes": [
    {
      "colors": [
        { "name": "blue", "score": 0.7316 },
        { "name": "gray", "score": 0.1604 },
        { "name": "black", "score": 0.154 },
        { "name": "white", "score": 0.1204 },
        { "name": "green", "score": 0.0952 },
        { "name": "red", "score": 0.052 },
        { "name": "yellow", "score": 0.0348 }
      ],
      "score": 0.4848,
      "type": "Truck"
    },
    {
      "colors": [
        { "name": "blue", "score": 0.7316 },
        { "name": "gray", "score": 0.1604 },
        { "name": "black", "score": 0.154 },
        { "name": "white", "score": 0.1204 },
        { "name": "green", "score": 0.0952 },
        { "name": "red", "score": 0.052 },
        { "name": "yellow", "score": 0.0348 }
      ],
      "score": 0.4812,
      "type": "Car"
    },
    {
      "colors": [
        { "name": "blue", "score": 0.7316 },
        { "name": "gray", "score": 0.1604 },
        { "name": "black", "score": 0.154 },
        { "name": "white", "score": 0.1204 },
        { "name": "green", "score": 0.0952 },
        { "name": "red", "score": 0.052 },
        { "name": "yellow", "score": 0.0348 }
      ],
      "score": 0.064,
      "type": "Bus"
    },
    {
      "score": 0.0376,
      "type": "VehicleOther"
    }
  ],
  "duration": 2.499,
  "end_time": "2026-03-20T10:17:48.024761Z",
  "id": "25436c51-6a75-4f23-99dc-7ddd638e4e61",
  "image": {
    "crop_box": {
      "bottom": 0.7349,
      "left": 0.6769,
      "right": 0.7983,
      "top": 0.6139
    },
    "data": "/9j/4AAQSkZJRgABAQAAAQABAAD/.../2Q=="
  },
  "parts": [
    {
      "object_track_id": "7ef28e6e-7cae-4186-b630-23819d0bfd98"
    }
  ],
  "path": [
    {
      "bounding_box": {
        "bottom": 0.786,
        "left": 0.0199,
        "right": 0.0937,
        "top": 0.7161
      },
      "timestamp": "2026-03-20T10:17:45.524804Z"
    },
    {
      "bounding_box": {
        "bottom": 0.79,
        "left": 0.0558,
        "right": 0.1322,
        "top": 0.7098
      },
      "timestamp": "2026-03-20T10:17:45.624800Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7895,
        "left": 0.0819,
        "right": 0.1649,
        "top": 0.708
      },
      "timestamp": "2026-03-20T10:17:45.724799Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7857,
        "left": 0.1174,
        "right": 0.2008,
        "top": 0.7051
      },
      "timestamp": "2026-03-20T10:17:45.824796Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7823,
        "left": 0.1476,
        "right": 0.2372,
        "top": 0.6998
      },
      "timestamp": "2026-03-20T10:17:45.924796Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7815,
        "left": 0.1837,
        "right": 0.2731,
        "top": 0.6964
      },
      "timestamp": "2026-03-20T10:17:46.024794Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7804,
        "left": 0.2181,
        "right": 0.3126,
        "top": 0.6921
      },
      "timestamp": "2026-03-20T10:17:46.124793Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7783,
        "left": 0.2549,
        "right": 0.3517,
        "top": 0.6888
      },
      "timestamp": "2026-03-20T10:17:46.224790Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7776,
        "left": 0.2961,
        "right": 0.3948,
        "top": 0.6821
      },
      "timestamp": "2026-03-20T10:17:46.324790Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7732,
        "left": 0.3372,
        "right": 0.4392,
        "top": 0.677
      },
      "timestamp": "2026-03-20T10:17:46.424787Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7698,
        "left": 0.3785,
        "right": 0.4817,
        "top": 0.6718
      },
      "timestamp": "2026-03-20T10:17:46.524788Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7653,
        "left": 0.4231,
        "right": 0.5265,
        "top": 0.6665
      },
      "timestamp": "2026-03-20T10:17:46.624812Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7591,
        "left": 0.4675,
        "right": 0.5732,
        "top": 0.6592
      },
      "timestamp": "2026-03-20T10:17:46.724784Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7538,
        "left": 0.5109,
        "right": 0.6194,
        "top": 0.6519
      },
      "timestamp": "2026-03-20T10:17:46.824781Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7489,
        "left": 0.5567,
        "right": 0.6623,
        "top": 0.647
      },
      "timestamp": "2026-03-20T10:17:46.924780Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7431,
        "left": 0.5996,
        "right": 0.7065,
        "top": 0.6398
      },
      "timestamp": "2026-03-20T10:17:47.024779Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7352,
        "left": 0.6438,
        "right": 0.7509,
        "top": 0.6312
      },
      "timestamp": "2026-03-20T10:17:47.124777Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7248,
        "left": 0.687,
        "right": 0.7882,
        "top": 0.624
      },
      "timestamp": "2026-03-20T10:17:47.224775Z"
    },
    {
      "bounding_box": {
        "bottom": 0.714,
        "left": 0.7311,
        "right": 0.8302,
        "top": 0.6151
      },
      "timestamp": "2026-03-20T10:17:47.324773Z"
    },
    {
      "bounding_box": {
        "bottom": 0.7026,
        "left": 0.7699,
        "right": 0.8711,
        "top": 0.6078
      },
      "timestamp": "2026-03-20T10:17:47.424771Z"
    },
    {
      "bounding_box": {
        "bottom": 0.6912,
        "left": 0.8084,
        "right": 0.9036,
        "top": 0.5999
      },
      "timestamp": "2026-03-20T10:17:47.524772Z"
    },
    {
      "bounding_box": {
        "bottom": 0.6797,
        "left": 0.8438,
        "right": 0.9391,
        "top": 0.59
      },
      "timestamp": "2026-03-20T10:17:47.624768Z"
    },
    {
      "bounding_box": {
        "bottom": 0.6706,
        "left": 0.879,
        "right": 0.9697,
        "top": 0.5824
      },
      "timestamp": "2026-03-20T10:17:47.724767Z"
    },
    {
      "bounding_box": {
        "bottom": 0.6623,
        "left": 0.9137,
        "right": 0.999,
        "top": 0.5758
      },
      "timestamp": "2026-03-20T10:17:47.824765Z"
    },
    {
      "bounding_box": {
        "bottom": 0.653,
        "left": 0.9492,
        "right": 0.999,
        "top": 0.5683
      },
      "timestamp": "2026-03-20T10:17:47.924763Z"
    },
    {
      "bounding_box": {
        "bottom": 0.6446,
        "left": 0.9833,
        "right": 0.999,
        "top": 0.5598
      },
      "timestamp": "2026-03-20T10:17:48.024761Z"
    }
  ],
  "start_time": "2026-03-20T10:17:45.524804Z"
}
```

## License

**[MIT License](./app/LICENSE)**
