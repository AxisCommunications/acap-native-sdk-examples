*Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP application using Recording Notify, Search and Playback D-Bus APIs

This example demonstrates how an ACAP application can search for and retrieve
recordings stored on an SD card, then monitor for new recording segments as they
are written. This is useful for applications that need to process recordings,
for example, for uploading to a cloud service.

A recording is logically organized into spans and segments:

- A span is a logical grouping of recording segments with a duration
  defined by the `spanDuration` property in the recording group configuration.

- A segment is an individual recording file within a span with a duration
  and size defined by the `segmentDuration` and `segmentSize` properties in the
  recording group configuration. The properties have both a target and maximum
  value. For a recording with both a video and audio track, these will either be
  muxed together in a segment or stored in separate segments depending on the
  `containerFormat` property.

For more information on the recording group configuration, refer to the
[Recording group API](https://developer.axis.com/vapix/device-configuration/recording-group/).

The example covers the following case:

1. Call `ListSpans` on `com.axis.RecordingSearch1.Search1` to retrieve all
   recording spans.
2. Call `GetSpanAttributes` on `com.axis.RecordingSearch1.Search1` to retrieve
   recording container and track information for span candidates found in the
   `ListSpans` response.
3. Call `GetSegment` on `com.axis.RecordingPlayback1.Playback1` repeatedly to
   retrieve all recording segments for span candidates found in the `ListSpans`
   response and container candidates found in the `GetSpanAttributes` response.
4. Receive `SegmentCompleted` signals on `com.axis.RecordingNotify1.Notify1` to
   monitor new recording segments.

## APIs used

- Recording Notify D-Bus service: `com.axis.RecordingNotify1` ([API documentation](https://developer.axis.com/acap/reference/supported-apis/edge-storage-apis/afd-recording-notify/))
- Interface: `com.axis.RecordingNotify1.Notify1`
- Signals: `SegmentCompleted`
- Recording Search D-Bus service: `com.axis.RecordingSearch1` ([API documentation](https://developer.axis.com/acap/reference/supported-apis/edge-storage-apis/afd-recording-search/))
- Interface: `com.axis.RecordingSearch1.Search1`
- Methods: `ListSpans`, `GetSpanAttributes`
- Recording Playback D-Bus service: `com.axis.RecordingPlayback1` ([API documentation](https://developer.axis.com/acap/reference/supported-apis/edge-storage-apis/afd-recording-playback/))
- Interface: `com.axis.RecordingPlayback1.Playback1`
- Methods: `GetSegment`
- sd-bus C library: `libsystemd`

## Getting started

These instructions guide you through building and running the example.

```sh
recording-playback
├── app
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── recording_playback.c
├── Dockerfile
└── README.md
```

- **app/recording_playback.c** - Application source code in C.
- **app/LICENSE** - Text file listing open source licensed source code distributed with the application.
- **app/Makefile** - Build and link instructions for the application.
- **app/manifest.json** - Defines the application and D-Bus method permissions.
- **Dockerfile** - Assembles an image containing the ACAP Native SDK and builds the application.
- **README.md** - Step-by-step build and run instructions.

### Prerequisites

- The device has an SD card installed and available as `SD_DISK`.
- Recording is configured and enabled so new spans and segments are written to the SD card.
- Existing recordings on the SD card are required for meaningful output during the initial retrieval phase (`ListSpans`, `GetSpanAttributes`, `GetSegment`).
- Ongoing recordings are required for meaningful output during the monitoring phase (`SegmentCompleted` signals).

### Build the application

Standing in your working directory run the following commands:

> [!NOTE]
>
> Depending on the network your local build machine is connected to, you may need to add proxy
> settings for Docker. See
> [Proxy in build time](https://developer.axis.com/acap/develop/proxy/#proxy-in-build-time).

```sh
docker build --platform=linux/amd64 --tag <APP_IMAGE> --build-arg ARCH=<ARCH> .
```

- `<APP_IMAGE>` is the name to tag the image with, e.g., `recording_playback:1.0`
- `<ARCH>` is the SDK architecture, `armv7hf` or `aarch64`.

Copy the result from the container image to a local directory `build`:

```sh
docker cp $(docker create --platform=linux/amd64 <APP_IMAGE>):/opt/app ./build
```

The `build` directory contains the build artifacts where the ACAP package is found
with suffix `.eap`, depending on selected architecture:

- `recording_playback_1_0_0_aarch64.eap`
- `recording_playback_1_0_0_armv7hf.eap`

> [!NOTE]
>
> For detailed information on build, install, and run of ACAP applications,
> refer to ACAP documentation: [Build, install, and run](https://developer.axis.com/acap/develop/build-install-run/).

### Install and start the application

Browse to:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

1. Click **Apps**.
2. Enable **Allow unsigned apps**.
3. Click **(+ Add app)** and upload the package.
4. Select one package:

   - `recording_playback_1_0_0_aarch64.eap`
   - `recording_playback_1_0_0_armv7hf.eap`

5. Click **Install**.
6. Start the application.

### Expected output

The application log can be found from:

- `http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=recording_playback`
- Application page -> `App log`

For meaningful output, the initial retrieval lines require existing recordings
on the SD card, and the monitoring lines require continued recording activity.

Expected log flow:

```text
[ INFO ] recording_playback[1234]: Get all recording spans for storage 'SD_DISK' with ListSpans
[ INFO ] recording_playback[1234]: Span 'S1': ...
[ INFO ] recording_playback[1234]: ListSpans returned N span(s)
[ INFO ] recording_playback[1234]: Get containers and tracks for span 'S1' with GetSpanAttributes
[ INFO ] recording_playback[1234]: Span attributes: ...
[ INFO ] recording_playback[1234]:   Container ...
[ INFO ] recording_playback[1234]:     Track ...
[ INFO ] recording_playback[1234]: GetSpanAttributes returned 1 container(s)
[ INFO ] recording_playback[1234]: Get all segments for span 'S1' and container 'v1' with GetSegment
[ INFO ] recording_playback[1234]: Segment: ...
[ INFO ] recording_playback[1234]: Listening for SegmentCompleted signals
[ INFO ] recording_playback[1234]: Segment: ...
```

## Practical notes

- The output shape of D-Bus responses may vary across AXIS OS versions.
- The example logs the complete method replies with sd-bus container traversal.

## License

**[MIT LICENSE](../LICENSE)**
