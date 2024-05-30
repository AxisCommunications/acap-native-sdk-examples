*Copyright (C) 2024, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# ACAP application using parameters

This example shows how to handle system-wide and application-defined parameters using the
[AXParameter library](https://axiscommunications.github.io/acap-documentation/docs/api/native-sdk-api.html#parameter-api).
Emphasis has been put on the use of callback functions and some of the limitations they impose.

## Project structure

THe files for building the application are organized in the following structure.

```sh
axparameter
├── app
│   ├── axparameter.c
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── Dockerfile
└── README.md
```

- **app/axparameter.c** - Application source code.
- **app/LICENSE** - List of all open source licensed source code distributed with the application.
- **app/Makefile** - Build and link instructions for the application.
- **app/manifest.json** - Definition of the application and its configuration, including pre-defined parameters.
- **Dockerfile** - Assembles an image containing the ACAP SDK toolchain and builds the application using it.
- **README.md** - Step by step instructions on how to run the example.

## Program structure and behavior

In the `paramConfig` section of `manifest.json`, two parameters have been defined;
`IsCustomized` and `BackupValue`.
These will exist on the device as soon as the application has been installed,
even before any application code has run,
and can be accessed using VAPIX calls to
[param.cgi](https://www.axis.com/vapix-library/subjects/t10175981/section/t10036014/display).

Since `BackupValue` is tagged with the control word `hidden`,
it will not show up in the application's *Settings* dialog,
but `IsCustomized` will be shown there as a checkbox.

Immediately upon startup, the application reads a system property using `ax_parameter_get()`.
By specifying a qualified parameter name like `Properties.System.SerialNumber`,
it is possible to read parameters that do not belong to this application.

Before entering the main loop, the program uses `ax_parameter_register_callback()` to subscribe to
parameter changes through a callback to `parameter_changed()`.
This callback will drive the behavior for the rest of the application's life time.

`parameter_changed()` will get called immediately when a parameter is modified through VAPIX or GUI.
It will receive the name and the new value of the modified parameter,
but it must not make any calls to the AXParameter library itself.
In order to work around this limitation, `g_timeout_add_seconds()` is used to schedule a call
`monitor_parameters()` one second later.

`monitor_parameters()` will use the AXParameter library to solve its tasks.
It inspects the values of all parameters, before performing one of two actions:

- If `IsCustomized` is `yes` it will, if necessary, use `ax_parameter_add()` to add parameter
  `CustomValue` and give it the current value stored in `BackupValue`.
- If `IsCustomized` is `no` it will, if necessary, store the current value of `CustomValue` in
  `BackupValue` and then use `ax_parameter_remove()` to remove parameter `CustomValue`.

Whenever `CustomValue` is added or removed at runtime, this also controls whether the input field
for `CustomValue` is shown in the application's *Settings* dialog or not.
However, the user also needs to reload the *Apps* page in order to see or get rid of the field in the
*Settings* dialog.

## Build the application

Standing in your working directory run the following commands:

> [!NOTE]
>
> Depending on the network you are connected to, you may need to add proxy settings.
> The file that needs these settings is: `~/.docker/config.json`. For reference please see
> https://docs.docker.com/network/proxy and a
> [script for Axis devices](https://axiscommunications.github.io/acap-documentation/docs/develop/build-install-run.html#configure-network-proxy-settings)
> in the ACAP documentation.

```sh
docker build --tag <APP_IMAGE> --build-arg ARCH=<ARCH> .
```

- `<APP_IMAGE>` is the name to tag the image with, e.g., `axparameter:1.0`
- `<ARCH>` is the SDK architecture, `armv7hf` or `aarch64`.

Copy the result from the container image to a local directory `build`:

```sh
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

The `build` directory contains the build artifacts, where the ACAP application
is found with suffix `.eap`, depending on which SDK architecture that was
chosen, one of these files should be found:

- `axparameter_1_0_0_aarch64.eap`
- `axparameter_1_0_0_armv7hf.eap`

## Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#/apps
```

- Click on the tab `App` in the device GUI.
- Click `(+)` sign to upload the application file.
- Browse to the newly built ACAP application, depending on architecture:
  - `axparameter_1_0_0_aarch64.eap`
  - `axparameter_1_0_0_armv7hf.eap`
- Click `Install`.
- Run the application by enabling the `Start` switch.

## Expected output

The application log can be found by either

- Browsing to `http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=axparameter`.
- Browsing to the *Apps* page and select `App log`.

Initially, the log will show the the device serial number, read from the system parameters.

```text
[ INFO    ] axparameter[1234567]: SerialNumber: 'BA9876543210'
```

Open the *Settings* dialog, check the *Is customized* checkbox, and click *Save*.
The log immediately shows:

```text
[ INFO    ] axparameter[1234567]: IsCustomized was changed to 'yes' just now
```

follow one second later by:

```text
[ INFO    ] axparameter[1234567]: IsCustomized was changed to 'yes' one second ago
[ INFO    ] axparameter[1234567]: App has a parameter named BackupValue
[ INFO    ] axparameter[1234567]: App has a parameter named IsCustomized
[ INFO    ] axparameter[1234567]: Parameter CustomValue was not found
[ INFO    ] axparameter[1234567]: The parameter CustomValue was added, but won't be visible in the Settings page until the Apps page is reloaded.
[ INFO    ] axparameter[1234567]: Custom value: 'restored from backup'
```

Note that the order of these messages will vary.
The order above is the order that they are written by the application.

Reload the *Apps* page and open the *Settings* dialog.
It will now show a field *Custom value* with the value "restored from backup".
Change it to "my customization" and click *Save*.

The log immediately shows:

```text
[ INFO    ] axparameter[1234567]: CustomValue was changed to 'my customization' just now
```

follow one second later by:

```text
[ INFO    ] axparameter[1234567]: CustomValue was changed to 'my customization' one second ago
[ INFO    ] axparameter[1234567]: App has a parameter named BackupValue
[ INFO    ] axparameter[1234567]: App has a parameter named CustomValue
[ INFO    ] axparameter[1234567]: App has a parameter named IsCustomized
[ INFO    ] axparameter[1234567]: Parameter CustomValue was found
[ INFO    ] axparameter[1234567]: Custom value: 'my customization'
```

Open the *Settings* dialog, uncheck the *Is customized* checkbox, and click *Save*.
The log immediately shows:

```text
[ INFO    ] axparameter[1234567]: IsCustomized was changed to 'no' just now
```

follow one second later by:

```text
[ INFO    ] axparameter[1234567]: IsCustomized was changed to 'no' one second ago
[ INFO    ] axparameter[1234567]: App has a parameter named BackupValue
[ INFO    ] axparameter[1234567]: App has a parameter named CustomValue
[ INFO    ] axparameter[1234567]: App has a parameter named IsCustomized
[ INFO    ] axparameter[1234567]: Parameter CustomValue was found
[ INFO    ] axparameter[1234567]: The parameter CustomValue was removed, but will be visible in the Settings page until the Apps page is reloaded.
[ INFO    ] axparameter[1234567]: Not customized
```

Reload the *Apps* page and open the *Settings* dialog.
The field *Custom value* is now gone.
Check the *Is customized* checkbox, and click *Save*.

The log immediately shows:

```text
[ INFO    ] axparameter[1234567]: IsCustomized was changed to 'yes' just now
```

follow one second later by:

```text
[ INFO    ] axparameter[1234567]: IsCustomized was changed to 'yes' one second ago
[ INFO    ] axparameter[1234567]: App has a parameter named BackupValue
[ INFO    ] axparameter[1234567]: App has a parameter named IsCustomized
[ INFO    ] axparameter[1234567]: Parameter CustomValue was not found
[ INFO    ] axparameter[1234567]: Custom value: 'my customization'
[ INFO    ] axparameter[1234567]: The parameter CustomValue was added, but won't be visible in the Settings page until the Apps page is reloaded.
```

Reload the *Apps* page and open the *Settings* dialog.
The field *Custom value* is back again, with the value "my customization" restored from the backup.

## License

**[Apache License 2.0](../LICENSE)**
