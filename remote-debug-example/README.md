*Copyright (C) 2024, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

<!-- omit from toc -->
# Remote debug an ACAP application

This guide will walk you through the process of debugging an ACAP application running on an Axis device using GDB and Visual Studio Code.

## Project structure

The files for building the application and the debug tools are organized in the following structure.

```sh
remote-debug-example
├── .gitignore
├── app
│   ├── .devcontainer
│   │   ├── aarch64-container
│   │   │   └── devcontainer.json
│   │   └── armv7hf-container
│   │       └── devcontainer.json
│   ├── .vscode
│   │   └── launch.json
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   └── remote_debug.c
├── Dockerfile
└── README.md
```

- **.gitignore** - gitignore file for the specific example, to avoid any build files being accidentally committed.
- **app/.devcontainer/aarch64-container/devcontainer.json** - Configuration file for the aarch64 development container.
- **app/.devcontainer/armv7hf-container/devcontainer.json** - Configuration file for the armv7hf development container.
- **app/.vscode/launch.json** - Configuration file for the Visual Studio Code debugger.
- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application with debug options added.
- **app/manifest.json** - Defines the application and its configuration.
- **app/remote_debug.c** - Application demonstrating a simple debugging scenario.
- **Dockerfile** - Docker file with the specified Axis toolchain, API container and the additional gdbserver to build the example specified.
- **README.md** - Step by step instructions on how to run the example.

<!-- omit from toc -->
## Table of contents

<!-- ToC GFM -->

- [Project structure](#project-structure)
- [The setup](#the-setup)
- [Prerequisites](#prerequisites)
- [Build and start a dev container](#build-and-start-a-dev-container)
  - [Build dev container](#build-dev-container)
  - [Start dev container](#start-dev-container)
- [Build and install ACAP application](#build-and-install-acap-application)
  - [Build the ACAP application](#build-the-acap-application)
  - [Install the ACAP application to device](#install-the-acap-application-to-device)
- [Upload and start gdbserver](#upload-and-start-gdbserver)
  - [Upload gdbserver to device via SCP](#upload-gdbserver-to-device-via-scp)
  - [Start gdbserver on device](#start-gdbserver-on-device)
- [Start debugging in dev container](#start-debugging-in-dev-container)
- [Cleanup](#cleanup)
- [Troubleshooting](#troubleshooting)
- [License](#license)

<!-- /ToC -->

## The setup

- **Axis device** (also referred to as **device** or **target** in this guide)
  - Will run `gdbserver` and attach to an installed ACAP application to debug.
- **User machine** (also referred to as **desktop** in this guide)
  - Use Visual Studio Code to run `gdb-multiarch` and connect to the `gdbserver` on the device.
  - Needs access to the application binary with debug symbols (not stripped).
  - Debugging is done in a [dev container](https://code.visualstudio.com/docs/devcontainers/containers)
    (also referred to as *development container* and the extension is called  *Dev Container*) in Visual Studio Code.

## Prerequisites

- [Visual Studio Code](https://code.visualstudio.com) installed on desktop.
- [Developer Mode](../DEV.md#developer-mode) enabled in the target device.
- Install Visual Studio Code extensions
  - [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) (ms-vscode-remote.remote-containers)

## Build and start a dev container

A **dev container** can be attached to Visual Studio Code and simplifies
the debugging process.

### Build dev container

Open a terminal, move to the example directory and run one or both of the
following commands to build the dev container for the architecture of
your Axis device:

```sh
docker build . --platform=linux/amd64 --build-arg ARCH=armv7hf -t dev_container:armv7hf
docker build . --platform=linux/amd64 --build-arg ARCH=aarch64 -t dev_container:aarch64
```

The dev container includes the `gdbserver` binary for remote debugging.

### Start dev container

1. Start Visual Studio Code and run `Dev Containers: Open Folder in Container...`
   via the Command Palette available by pressing `F1` or `Ctrl+Shift+P`.
2. Browse to the `app` folder in the repository and choose `Open`.
3. In the dev container, make sure to install the recommended Visual Studio Code
   extensions (not the one stated in prerequisites), usually a dialog
   prompts the user to install recommended extensions shortly after the
   dev container is started. See field `extensions` in files
   `app/.devcontainer/<ARCH>-container/devcontainer.json` for more information.
4. Depending on the architecture of the device you like to debug, choose the
   architecture of the dev container when prompted.
5. If you don't have a terminal open by default in Visual Studio Code, open one
   via the `Terminal → New Terminal` menu.
6. The terminal will open inside the dev container in the `/opt/app` directory
   and is now ready for development.

## Build and install ACAP application

This part shows how to build and install the ACAP application of the example in
the dev container started in previous section.

### Build the ACAP application

To build the ACAP application, run the following command from the same terminal:

   ```sh
   acap-build .
   ```

The `/opt/app` directory now contains build artifacts, where the ACAP application
is found with suffix `.eap` and depending on which SDK architecture that was
chosen, one of these files should be found:

- `remote_debug_1_0_0_aarch64.eap`
- `remote_debug_1_0_0_armv7hf.eap`

> [!NOTE]
> When developing interactively in a dev container, the build files will end up
> in the source code directory of the repository, here called `app`. It's
> recommended to use a `.gitignore` file to not commit build files in a
> production repository. An example can be seen in the
> [`.gitignore`](.gitignore) file of this repository.

### Install the ACAP application to device

To install the ACAP application on your device, make sure to enable the
`Allow unsigned apps` toggle through the web interface and run the following
command from the same terminal:

```sh
eap-install.sh <AXIS_DEVICE_IP> <ADMIN_ACCOUNT> <PASSWORD> install
```

## Upload and start gdbserver

Before uploading the `gdbserver` to your device, ensure that [Developer
Mode](../DEV.md#developer-mode) is enabled. Developer Mode grants SSH access to
the ACAP application's dynamic user on the Axis device.

When the ACAP application has been uploaded to your device,
*provided `Developer Mode` is enabled*, an ACAP application user named
`acap-remote_debug` with SSH access should have been created.

Make sure to set the password for the application user before continuing to
next section.

### Upload gdbserver to device via SCP

1. Open a new terminal in Visual Studio Code, using either
   `Terminal → New Terminal` or `Terminal → Split Terminal`. This terminal will
   also be running inside the dev container, just like the existing terminal.
   Using a second terminal allows for better visualization and keeps the
   debugging process within the IDE.

2. In the new terminal, upload the `gdbserver` binary to the Axis device
   directory `/tmp` by using `scp`:

   ```sh
   scp /opt/build/gdb/bin/gdbserver acap-remote_debug@<AXIS_DEVICE_IP>:/tmp/
   ```

### Start gdbserver on device

In the same terminal (used for `scp` in previous section), log into the device
with `ssh`, start the `gdbserver` with the application binary, and open a TCP
connection to a debug port on the Axis device, here port `1234`:

```sh
ssh acap-remote_debug@<AXIS_DEVICE_IP>
/tmp/gdbserver :1234 /usr/local/packages/remote_debug/remote_debug
```

You should see output similar to:

```sh
Process /usr/local/packages/remote_debug/remote_debug created; pid = 7423
Listening on port 1234
```

> [!TIP]
> Keep both terminals open during the debugging process: one for building and
> installing the application, and one for the SSH connection to the device.

## Start debugging in dev container

> [!NOTE]
> The `launch.json` file contains a hardcoded IP address (`192.168.0.90`) for
> the Axis device. This usually works if the target device is connected directly
> to the computer, as it will typically have this IP address. However, if you
> are debugging a device with a different IP address, make sure to update the
> `miDebuggerServerAddress` field in `launch.json` with the correct IP address
> and port number.

1. In the dev container, open `remote_debug.c` and press `F5` to start
   debugging.
2. You can now set breakpoints, step through the code, and inspect variables
   using Visual Studio Code's debugging features.
3. In the SSH session, you should see output similar to:

   ```text
   Remote debugging from host ::ffff:192.1.2.3, port 1234
   ```

To rebuild and reinstall the ACAP application after making changes, run the
following commands in the dev container terminal:

```sh
make clean
acap-build .
eap-install.sh <AXIS_DEVICE_IP> <ADMIN_ACCOUNT> <PASSWORD> install
```

> [!NOTE]
> If this simple example is fixed, it will run once and exit, which will also
> exit the `gdbserver` running on the Axis device. To run a new remote
> debug the `gdbserver` needs to be restarted.

## Cleanup

To remove the built files and stop the debugging session:

1. Press `Shift+F5` in Visual Studio Code to stop debugging.
2. In the dev container terminal, run the following command to remove the built files:

   ```sh
   make clean
   ```

## Troubleshooting

- If SSH access is not working after enabling `Developer Mode`, try restarting
  the device and then reinstall the ACAP application.
- If SSH access is not working after upgrading the ACAP application, make sure
  to set the SSH password for the ACAP application user again.
- If you encounter permission issues when uploading the `gdbserver` or starting
  the debugging session, make sure that the ACAP application user has SSH access
  enabled and a password set.
- If the debugging session doesn't start, verify that the `gdbserver` is running
  on the device and listening on the correct port.
- If you see an error message about missing debug symbols, make sure that the
  ACAP application was built with debug symbols and that the correct binary is
  being used for debugging.

## License

**[Apache License 2.0](../LICENSE)**
