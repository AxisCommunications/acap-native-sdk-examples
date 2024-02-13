*Copyright (C) 2024, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP application consuming metadata stream Analytics Scene Description

> [!IMPORTANT]
>
> - The Metadata Broker API is released as a [Beta
> API](https://axiscommunications.github.io/acap-documentation/docs/api/beta-api.html)
> in ACAP Native SDK 1.13 which maps to AXIS OS 11.9.
> - At the time of the release, there's only one topic that can be subscribed.
>   See more information in the [API documentation](https://axiscommunications.github.io/acap-documentation/docs/api/src/api/metadata-broker/html/standard_topics.html).

## Use case

- Consume streamed analytics metadata of detected objects in the scene.

## Introduction

This example showcases how an ACAP application can `consume` metadata in AXIS OS
by using the [Metadata Broker API](https://axiscommunications.github.io/acap-documentation/docs/api/native-sdk-api.html#metadata-broker-api).

The ACAP application is a **consumer** that subscribes to the `topic`
`com.axis.analytics_scene_description.v0.beta`.

Example data with multiple detections:

```json
{
   "frame" : {
      "observations" : [
         {
            "bounding_box" : {
               "bottom" : 0.7413,
               "left" : 0.4396,
               "right" : 0.7661,
               "top" : 0.4234
            },
            "class" : {
               "score" : 0.74,
               "type" : "Car"
            },
            "timestamp" : "2024-02-14T15:37:21.040577Z",
            "track_id" : "25"
         },
         {
            "bounding_box" : {
               "bottom" : 0.9431,
               "left" : 0.9656,
               "right" : 0.9989,
               "top" : 0.8365
            },
            "timestamp" : "2024-02-14T15:37:21.040577Z",
            "track_id" : "26"
         },
         {
            "bounding_box" : {
               "bottom" : 0.839,
               "left" : 0.8295,
               "right" : 0.9782,
               "top" : 0.2037
            },
            "class" : {
               "score" : 0.75,
               "type" : "Human"
            },
            "timestamp" : "2024-02-14T15:37:21.040577Z",
            "track_id" : "37"
         },
         {
            "bounding_box" : {
               "bottom" : 0.9395,
               "left" : 0.0219,
               "right" : 0.6531,
               "top" : 0.002
            },
            "timestamp" : "2024-02-14T15:37:21.040577Z",
            "track_id" : "46"
         },
         {
            "bounding_box" : {
               "bottom" : 0.3181,
               "left" : 0.7094,
               "right" : 0.8114,
               "top" : 0.1012
            },
            "timestamp" : "2024-02-14T15:37:21.040577Z",
            "track_id" : "48"
         }
      ],
      "operations" : [
         {
            "id" : "35",
            "type" : "DeleteOperation"
         },
         {
            "id" : "47",
            "type" : "DeleteOperation"
         }
      ],
      "timestamp" : "2024-02-14T15:37:21.040577Z"
   }
}
```

## Getting started

These instructions will guide you on how to execute the code. Below is the
structure and scripts used in the example:

```sh
consume-analytics-scene-description
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── consume_analytics_metadata.c
├── Dockerfile
└── README.md
```

- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json** - Defines the application and its configuration.
- **app/consume_analytics_metadata.c** - Application source code.
- **Dockerfile** - Dockerfile with the specified Axis toolchain and API container to build the example specified.
- **README.md** - Step by step instructions on how to run the example.

### How to run the code

Below is the step by step instructions on how to execute the program. So
basically starting with the generation of the .eap file to running it on a
device:

#### Build the application

Standing in your working directory run the following commands:

> [!NOTE]
>
> Depending on the network you are connected to, you may need to add proxy settings.
> The file that needs these settings is: `~/.docker/config.json`. For reference please see
> https://docs.docker.com/network/proxy and a
> [script for Axis devices](https://axiscommunications.github.io/acap-documentation/docs/develop/build-install-run.html#configure-network-proxy-settings) in the ACAP documentation.

```sh
docker build --tag <APP_IMAGE> --build-arg ARCH=<ARCH> .
```

- `<APP_IMAGE>` is the name to tag the image with, e.g., `consume_analytics_metadata:1.0`
- `<ARCH>` is the SDK architecture, `armv7hf` or `aarch64`.

Copy the result from the container image to a local directory build:

```sh
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

The working directory now contains a build folder with the following files:

```sh
consume-analytics-scene-description
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── consume_analytics_metadata.c
├── build
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── param.conf
│   ├── consume_analytics_metadata*
│   ├── consume_analytics_metadata_1_0_0_armv7hf.eap
│   ├── consume_analytics_metadata_1_0_0_LICENSE.txt
│   └── consume_analytics_metadata.c
├── Dockerfile
└── README.md
```

- **build/manifest.json** - Defines the application and its configuration.
- **build/package.conf** - Defines the application and its configuration.
- **build/param.conf** - File containing application parameters.
- **build/consume_analytics_metadata*** - Application executable binary file.
- **build/consume_analytics_metadata_1_0_0_armv7hf.eap** - Application package .eap file.
- **build/consume_analytics_metadata_1_0_0_LICENSE.txt** - Copy of LICENSE file.

#### Install your application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/#settings/apps
```

- Click on the tab `App` in the device GUI
- Click `(+)` sign to upload the application file
- Browse to the newly built `consume_analytics_metadata_1_0_0_armv7hf.eap`
- Click `Install`
- Run the application by enabling the `Start` switch

#### The expected output

The application log can be found by either

- Browse to `http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=consume_analytics_metadata`.
- Browse to the application page and click the `App log`.
- Extract the logs using following commands in the terminal.
  > [!IMPORTANT]
  > Please make sure SSH is enabled on the device to run the following commands.

  ```sh
  ssh root@<AXIS_DEVICE_IP>
  cd /var/log/
  head -50 info.log
  ```

> [!IMPORTANT]
> The logged output can have data which is empty or has a detection.
> To get a detection, make sure to have an object that can be detected by
> the Axis device scene data. Examples are persons and cars.

```sh
----- Contents of SYSTEM_LOG for 'consume_analytics_metadata' -----

consume_analytics_metadata[3844]: Subscribed to com.axis.analytics_scene_description.v0.beta (1)...
consume_analytics_metadata[3844]: Subscriber started...
consume_analytics_metadata[3844]: metadata received from topic: com.axis.analytics_scene_description.v0.beta on source: 1: Monotonic time - 483.054847000. Data - {"frame":{"observations":[{"bounding_box":{"bottom":0.7384,"left":0.4254,"right":0.7552,"top":0.4216},"class":{"score":0.74,"type":"Car"},"timestamp":"2024-02-14T15:37:20.940582Z","track_id":"25"},{"bounding_box":{"bottom":0.9431,"left":0.9656,"right":0.9989,"top":0.8384},"timestamp":"2024-02-14T15:37:20.940582Z","track_id":"26"},{"bounding_box":{"bottom":0.8378,"left":0.8102,"right":0.9693,"top":0.1988},"class":{"score":0.75,"type":"Human"},"timestamp":"2024-02-14T15:37:20.940582Z","track_id":"37"},{"bounding_box":{"bottom":0.0553,"left":0.727,"right":0.7499,"top":0.0443},"timestamp":"2024-02-14T15:37:20.940582Z","track_id":"44"},{"bounding_box":{"bottom":0.7997,"left":0.9833,"right":0.9989,"top":0.7262},"timestamp":"2024-02-14T15:37:20.940582Z","track_id":"45"},{"bounding_box":{"bottom":0.9689,"left":0.0208,"right":0.6354,"top":0.002},"timestamp":"2024-02-14T15:37:20.940582Z","track_id":"46"},{"bounding_box":{"bottom":0.3236,"left":0.7083,"right":0.8041,"top":0.0994},"timestamp":"2024-02-14T15:37:20.940582Z","track_id":"48"}],"operations":[],"timestamp":"2024-02-14T15:37:20.940582Z"}}
consume_analytics_metadata[3844]: metadata received from topic: com.axis.analytics_scene_description.v0.beta on source: 1: Monotonic time - 483.154843000. Data - {"frame":{"observations":[{"bounding_box":{"bottom":0.7413,"left":0.4396,"right":0.7661,"top":0.4234},"class":{"score":0.74,"type":"Car"},"timestamp":"2024-02-14T15:37:21.040577Z","track_id":"25"},{"bounding_box":{"bottom":0.9431,"left":0.9656,"right":0.9989,"top":0.8365},"timestamp":"2024-02-14T15:37:21.040577Z","track_id":"26"},{"bounding_box":{"bottom":0.839,"left":0.8295,"right":0.9782,"top":0.2037},"class":{"score":0.75,"type":"Human"},"timestamp":"2024-02-14T15:37:21.040577Z","track_id":"37"},{"bounding_box":{"bottom":0.9395,"left":0.0219,"right":0.6531,"top":0.002},"timestamp":"2024-02-14T15:37:21.040577Z","track_id":"46"},{"bounding_box":{"bottom":0.3181,"left":0.7094,"right":0.8114,"top":0.1012},"timestamp":"2024-02-14T15:37:21.040577Z","track_id":"48"}],"operations":[{"id":"35","type":"DeleteOperation"},{"id":"47","type":"DeleteOperation"}],"timestamp":"2024-02-14T15:37:21.040577Z"}}
```

The format of a detection shown in a more readable way.

```json
{
   "frame" : {
      "observations" : [
         {
            "bounding_box" : {
               "bottom" : 0.7413,
               "left" : 0.4396,
               "right" : 0.7661,
               "top" : 0.4234
            },
            "class" : {
               "score" : 0.74,
               "type" : "Car"
            },
            "timestamp" : "2024-02-14T15:37:21.040577Z",
            "track_id" : "25"
         },
         {
            "bounding_box" : {
               "bottom" : 0.9431,
               "left" : 0.9656,
               "right" : 0.9989,
               "top" : 0.8365
            },
            "timestamp" : "2024-02-14T15:37:21.040577Z",
            "track_id" : "26"
         },
         {
            "bounding_box" : {
               "bottom" : 0.839,
               "left" : 0.8295,
               "right" : 0.9782,
               "top" : 0.2037
            },
            "class" : {
               "score" : 0.75,
               "type" : "Human"
            },
            "timestamp" : "2024-02-14T15:37:21.040577Z",
            "track_id" : "37"
         },
         {
            "bounding_box" : {
               "bottom" : 0.9395,
               "left" : 0.0219,
               "right" : 0.6531,
               "top" : 0.002
            },
            "timestamp" : "2024-02-14T15:37:21.040577Z",
            "track_id" : "46"
         },
         {
            "bounding_box" : {
               "bottom" : 0.3181,
               "left" : 0.7094,
               "right" : 0.8114,
               "top" : 0.1012
            },
            "timestamp" : "2024-02-14T15:37:21.040577Z",
            "track_id" : "48"
         }
      ],
      "operations" : [
         {
            "id" : "35",
            "type" : "DeleteOperation"
         },
         {
            "id" : "47",
            "type" : "DeleteOperation"
         }
      ],
      "timestamp" : "2024-02-14T15:37:21.040577Z"
   }
}
```

## License

**[Apache License 2.0](../LICENSE)**
