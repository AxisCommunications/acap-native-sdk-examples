*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP application using license keys

This example shows how to validate an application against an installed license key on device using the
[License Key API](https://developer.axis.com/acap/api/#license-key-api).

A license key is a signed file generated for a specific device ID and application ID.
The [ACAP Service Portal](https://developer.axis.com/acap/service/acap-service-portal)
maintains both license keys and application IDs.

## Project structure

The files for building the application are organized in the following structure.

```sh
licensekey
├── app
│   ├── LICENSE
│   ├── licensekey_handler.c
│   ├── Makefile
│   └── manifest.json
├── Dockerfile
└── README.md
```

- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/licensekey_handler.c** - Application to check licensekey status in C.
- **app/Makefile** - Build and link instructions for the application.
- **app/manifest.json** - Defines the application and its configuration.
- **Dockerfile** - Assembles an image containing the ACAP Native SDK and builds the application using it.
- **README.md** - Step by step instructions on how to run the example.

## Application description

The application verifies the license with:

```c
licensekey_verify(glob_app_name, APP_ID, MAJOR_VERSION, MINOR_VERSION)
```

These values (`glob_app_name`, `APP_ID`, `MAJOR_VERSION`, `MINOR_VERSION`) must match those in `app/manifest.json` for the license validation to succeed.

After the initial check, the app schedules repeated validation every 300 seconds using
`g_timeout_add_seconds(CHECK_SECS, check_license_status, NULL)`.

## Build the application

Standing in your working directory run the following commands:

> [!NOTE]
>
> Depending on the network your local build machine is connected to, you may need to add proxy
> settings for Docker. See
> [Proxy in build time](https://developer.axis.com/acap/develop/proxy/#proxy-in-build-time).

```sh
docker build --platform=linux/amd64 --tag <APP_IMAGE> --build-arg ARCH=<ARCH> .
```

- `<APP_IMAGE>` is the name to tag the image with, e.g., `licensekey:1.0`
- `<ARCH>` is the SDK architecture, `armv7hf` or `aarch64`.

Copy the result from the container image to a local directory build:

```sh
docker cp $(docker create --platform=linux/amd64 <APP_IMAGE>):/opt/app ./build
```

The `build` directory contains the build artifacts, where the ACAP application
is found with suffix `.eap`, depending on which SDK architecture that was
chosen, one of these files should be found:

- `licensekey_handler_1_0_0_aarch64.eap`
- `licensekey_handler_1_0_0_armv7hf.eap`

> [!NOTE]
>
> For detailed information on how to build, install, and run ACAP applications, refer to the official ACAP documentation: [Build, install, and run](https://developer.axis.com/acap/develop/build-install-run/).

## Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

1. Click on the tab **Apps** in the device GUI
2. Enable **Allow unsigned apps** toggle
3. Click **(+ Add app)** button to upload the application file
4. Select the newly built application package, depending on architecture:

   - `licensekey_handler_1_0_0_aarch64.eap`
   - `licensekey_handler_1_0_0_armv7hf.eap`

5. Click **Install**
6. Run the application by enabling the **Start** switch

## Expected output

The application log can be found by either

- Browsing to `http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=licensekey_handler`.
- Browsing to the **Apps** page and select **App log**.

```sh
----- Contents of SYSTEM_LOG for 'licensekey_handler' -----


10:26:42.499 [ INFO ] licensekey_handler[0]: starting licensekey_handler
10:26:42.539 [ INFO ] licensekey_handler[14660]: Licensekey is invalid
10:31:43.058 [ INFO ] licensekey_handler[14660]: Licensekey is invalid
```

A valid license key for a registered application ID is available through
[ACAP Service Portal](https://developer.axis.com/acap/service/acap-service-portal).

Support for installing a license key through the device web page is available when
`acapPackageConf.copyProtection.method` is set to `axis` in `manifest.json`.

Install a license by following these steps in the device web page:

1. Go to the **Apps** tab.
2. Click the installed **licensekey_handler** application.
3. In **Activate the license**, click **Install** and upload the license file.

More instructions on license handling are available on
[Axis Developer Community](https://www.axis.com/developer-community).

## License

**[Apache License 2.0](../LICENSE)**
