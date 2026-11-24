*Copyright (C) 2024, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

<!-- omit from toc -->
# Remote debug an ACAP application

This guide will walk you through the process of debugging an ACAP application
running on an Axis device using GDB, either from Visual Studio Code or from a
plain terminal. Follow the shared steps and choose your method in steps 2
and 5.

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
- **app/Makefile** - Build and link instructions for the application.with debug options added.
- **app/manifest.json** - Defines the application and its configuration.
- **app/remote_debug.c** - Application demonstrating a simple debugging scenario.
- **Dockerfile** - Assembles an image containing the ACAP Native SDK and builds the application using it.
- **README.md** - Step by step instructions on how to run the example.

<!-- omit from toc -->
## Table of contents

<!-- ToC GFM -->

- [Project structure](#project-structure)
- [The setup](#the-setup)
- [Prerequisites](#prerequisites)
- [1. Build the container image (both methods)](#1-build-the-container-image-both-methods)
- [2. Open a container shell (choose your method)](#2-open-a-container-shell-choose-your-method)
  - [Visual Studio Code: open a dev container](#visual-studio-code-open-a-dev-container)
  - [Terminal: start a container shell](#terminal-start-a-container-shell)
- [3. Build and install the application (both methods)](#3-build-and-install-the-application-both-methods)
  - [Build the ACAP application](#build-the-acap-application)
  - [Install the ACAP application to device](#install-the-acap-application-to-device)
- [4. Upload and start gdbserver (both methods)](#4-upload-and-start-gdbserver-both-methods)
  - [Upload gdbserver to device via SCP](#upload-gdbserver-to-device-via-scp)
  - [Start gdbserver on device](#start-gdbserver-on-device)
- [5. Connect the debugger (choose your method)](#5-connect-the-debugger-choose-your-method)
  - [Visual Studio Code: start the graphical debugger](#visual-studio-code-start-the-graphical-debugger)
  - [Terminal: connect with gdb-multiarch](#terminal-connect-with-gdb-multiarch)
  - [Terminal: debug session walk-through](#terminal-debug-session-walk-through)
- [6. Rebuild and restart (both methods)](#6-rebuild-and-restart-both-methods)
- [Cleanup](#cleanup)
- [Troubleshooting](#troubleshooting)
- [License](#license)

<!-- /ToC -->

## The setup

- **Axis device** (also referred to as **device** or **target** in this guide)
  - Will run `gdbserver` and attach to an installed ACAP application to debug.
- **User machine** (also referred to as **desktop** in this guide)
  - Runs `gdb-multiarch` in a container built from this example and connects
    to the `gdbserver` on the device.
  - Needs access to the application binary with debug symbols (not stripped).
- **Container** is used either as a
    [dev container](https://code.visualstudio.com/docs/devcontainers/containers)
    in Visual Studio Code, or directly from a terminal with `docker run`.

## Prerequisites

- [Docker](https://docs.docker.com/get-docker/) installed on desktop.
- [Developer Mode](../DEV.md#developer-mode) enabled in the target device.
- Only when debugging in Visual Studio Code:
  - [Visual Studio Code](https://code.visualstudio.com) installed on desktop.
  - The [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
    extension (ms-vscode-remote.remote-containers).

## 1. Build the container image (both methods)

Open a terminal, move to the example directory and run one or both of the
following commands to build the container image for the architecture of
your Axis device:

```sh
docker build . --platform=linux/amd64 --build-arg ARCH=armv7hf -t dev_container:armv7hf
docker build . --platform=linux/amd64 --build-arg ARCH=aarch64 -t dev_container:aarch64
```

The container image includes the `gdbserver` binary for remote debugging.

## 2. Open a container shell (choose your method)

Choose one of the following methods. Both provide a shell in `/opt/app`
for the shared build, installation and gdbserver steps.

Both methods will use two terminals:

1. **Terminal A** builds and installs the application. In the terminal method,
   it also runs the `gdb-multiarch` client.
2. **Terminal B** runs `gdbserver` over SSH from the same container.

### Visual Studio Code: open a dev container

1. In Visual Studio Code, run `Dev Containers: Open Folder in Container...`
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
6. Terminal A will open inside the dev container in the `/opt/app` directory
   and is now ready for development.

### Terminal: start a container shell

In **terminal A**, from the example directory on the desktop, run one of the
following commands for your device architecture. The `app` directory is mounted
so the debug binary ends up on the desktop:

```sh
docker run --name remote-debug --rm -it -v "$PWD/app":/opt/app -w /opt/app dev_container:aarch64

docker run --name remote-debug --rm -it -v "$PWD/app":/opt/app -w /opt/app dev_container:armv7hf
```

Keep this container running throughout the debugging session. You will open
terminal B in step 4.

## 3. Build and install the application (both methods)

Run the following commands in terminal A, opened in step 2.

### Build the ACAP application

To build the ACAP application, run the following command from the same terminal:

> [!NOTE]
>
> - You need to
>   [Set your vendorId in the manifest](../DEV.md#set-your-vendorid-in-the-manifest)
>   in order to run the examples unsigned with
>   [Developer Mode](https://developer.axis.com/acap/get-started/set-up-developer-environment/set-up-device-advanced/#developer-mode).
> - Depending on the network your local build machine is connected to, you may need to add proxy
>   settings for Docker. See
>   [Proxy in build time](https://developer.axis.com/acap/develop/proxy/#proxy-in-build-time).

   ```sh
   acap-build .
   ```

The `/opt/app` directory now contains build artifacts, where the ACAP application
is found with suffix `.eap` and depending on which SDK architecture that was
chosen, one of these files should be found:

- `remote_debug_1_0_0_aarch64.eap`
- `remote_debug_1_0_0_armv7hf.eap`

The `Makefile` keeps the unstripped binary in `debug/`, so
`app/debug/remote_debug` is what the debugger loads while the `.eap` ships the
stripped binary.

> [!NOTE]
> When developing interactively in a dev container, the build files will end up
> in the source code directory of the repository, here called `app`. It's
> recommended to use a `.gitignore` file to not commit build files in a
> production repository. An example can be seen in the
> [`.gitignore`](.gitignore) file of this repository.

### Install the ACAP application to device

To install the ACAP application on your device, run the following command from the same terminal:

```sh
eap-install.sh <AXIS_DEVICE_IP> <ADMIN_ACCOUNT> <PASSWORD> install
```

## 4. Upload and start gdbserver (both methods)

Before uploading the `gdbserver` to your device, ensure that [Developer
Mode](../DEV.md#developer-mode) is enabled. Developer Mode grants SSH access to
the ACAP application's dynamic user on the Axis device.

When the ACAP application has been uploaded to your device,
*provided `Developer Mode` is enabled*, an ACAP application user named
`acap-remote_debug` with SSH access should have been created.

Make sure to set the password for the application user before continuing to
next section.

### Upload gdbserver to device via SCP

For Visual Studio Code, open a second terminal, **terminal B**, inside the dev container using
`Terminal → New Terminal` or `Terminal → Split Terminal`.

For the terminal method, open a second terminal, **terminal B**, and start a second
shell in the same container used in terminal A:

```sh
docker exec -it remote-debug bash
```

From that container shell, upload the `gdbserver` binary to the Axis device
directory `/tmp` by using `scp`:

```sh
scp /opt/build/gdb/bin/gdbserver acap-remote_debug@<AXIS_DEVICE_IP>:/tmp/
```

### Start gdbserver on device

In terminal B (used for `scp` in previous section), log into the device
with `ssh`, start the `gdbserver` with the application binary, and open a TCP
connection to a debug port on the Axis device, here port `1234`:

```sh
ssh acap-remote_debug@<AXIS_DEVICE_IP>
/tmp/gdbserver :1234 /usr/local/packages/remote_debug/remote_debug
```

> [!NOTE]
> If your `manifest.json` file contains runtime options under
> `acapPackageConf.setup.runOptions`, these are not automatically propagated to
> the gdbserver. You must explicity include them when starting the `gdbserver`.
>
> For example, if your `manifest.json` contains:
>
> ```json
> "runOptions": "--arg1 value1 --arg2 value2"
> ```
>
> Start the gdbserver with:
>
> ```sh
> /tmp/gdbserver :1234 /usr/local/packages/remote_debug/remote_debug --arg1 value1 --arg2 value2
> ```

You should see output similar to:

```sh
Process /usr/local/packages/remote_debug/remote_debug created; pid = 7423
Listening on port 1234
```

## 5. Connect the debugger (choose your method)

### Visual Studio Code: start the graphical debugger

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
3. In terminal B, you should see output similar to:

   ```text
   Remote debugging from host ::ffff:192.1.2.3, port 1234
   ```

### Terminal: connect with gdb-multiarch

In **terminal A**, connect the debug client to the device:

```sh
gdb-multiarch -q debug/remote_debug \
  -ex "set sysroot $SDKTARGETSYSROOT" \
  -ex "target remote <AXIS_DEVICE_IP>:1234"
```

`set sysroot` points the debugger at the SDK sysroot so symbols for the device
libraries are resolved. Terminal B now prints similar to:

```text
Remote debugging from host ::ffff:192.1.2.3, port 33556
```

### Terminal: debug session walk-through

These `gdb` commands replace the buttons of a graphical debugger.

| Command | Purpose |
| ------- | ------- |
| `break <function or file:line>` | Set a breakpoint |
| `continue` | Run until the next breakpoint or signal |
| `next` | Step over the current line |
| `step` | Step into a function call |
| `print <expression>` | Inspect a variable |
| `info locals` | List the local variables of the current frame |
| `backtrace` | Show the call stack |
| `detach` / `quit` | End the session |

Run them at the `(gdb)` prompt to reach the bug in this example:

```text
(gdb) break main
Breakpoint 1 at 0x5555550760: file remote_debug.c, line 21.
(gdb) continue
Continuing.
Breakpoint 1, main () at remote_debug.c:21
21          char* message = "Hello World!";
(gdb) next
22          message[0]    = 'h';  // This will cause a segmentation fault
(gdb) print message
$1 = 0x55555507b8 "Hello World!"
(gdb) next
Program received signal SIGSEGV, Segmentation fault.
0x0000005555550774 in main () at remote_debug.c:22
22          message[0]    = 'h';  // This will cause a segmentation fault
(gdb) backtrace
#0  0x0000005555550774 in main () at remote_debug.c:22
(gdb) quit
A debugging session is active.

        Inferior 1 [process 344661] will be killed.
```

The fault is the write to a string literal, which lives in read-only memory.
To try a fix, edit the source, then follow step 6.

## 6. Rebuild and restart (both methods)

Stop the current debug session with `Shift+F5` in Visual Studio Code or `quit`
at the `(gdb)` prompt. After making changes, run the following commands in the
container shell in `/opt/app`, not in the SSH session on the device:

```sh
make clean
acap-build .
eap-install.sh <AXIS_DEVICE_IP> <ADMIN_ACCOUNT> <PASSWORD> install
```

Restart `gdbserver` on the device as described in
[Start gdbserver on device](#start-gdbserver-on-device), then reconnect using
your method in [step 5](#5-connect-the-debugger-choose-your-method).

> [!NOTE]
> If this simple example is fixed, it will run once and exit, which will also
> exit the `gdbserver` running on the Axis device. Restart `gdbserver` for each
> new debug session.

## Cleanup

To remove the built files and stop the debugging session:

1. Press `Shift+F5` in Visual Studio Code to stop debugging, or type `quit` at
   the `(gdb)` prompt when debugging from the terminal.
2. Remove the uploaded `gdbserver` from the device:

   ```sh
   ssh acap-remote_debug@<AXIS_DEVICE_IP> rm /tmp/gdbserver
   ```

3. Remove the built files from terminal A:

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
