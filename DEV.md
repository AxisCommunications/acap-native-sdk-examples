<!-- omit from toc -->
# ACAP application developer topics

This page is a collection of common topics related to ACAP application
development.

<!-- omit from toc -->
## Table of contents

- [Access the device](#access-the-device)
- [Developer Mode](#developer-mode)
  - [Set your vendor ID in the manifest](#set-your-vendorid-in-the-manifest)
- [Logs and reports](#logs-and-reports)
  - [Access the system log](#access-the-system-log)
  - [Access the application log](#access-the-application-log)
  - [Access the server report](#access-the-server-report)
- [Proxy](#proxy)

## Access the device

To log in to an Axis device shell to perform command-line options, **SSH** is
used. Instructions to set up SSH and how to log in to shell are found in the
ACAP documentation section [Access the device through
SSH][access-device-through-ssh].

## Developer Mode

Starting with AXIS OS 13.0, all ACAP applications must be signed to be installed
on an Axis device. During development, this requirement can be bypassed by
enabling Developer Mode, which also provides SSH access for the application's
respective user.

For detailed instructions on how to request, enable, and disable Developer Mode,
refer to the [Developer Mode][developer-mode] section in the ACAP documentation.

### Set your vendorId in the manifest

To run an example from this repository as an unsigned ACAP application using the Developer Mode setup,
you need to set the `vendorId` field in the example's `manifest.json` file.
Replace the `vendorId` with the **case-sensitive** value shown on the Developer Mode ACAP application page provided by the ACAP Service Portal.
See [Get vendorId to use][get-vendorid] for more information.

Update the manifest file in all examples by standing in the root of this
repository and running:

```sh
find . -type f -name "*manifest*" -exec sed -i -e 's/12345abcde/<VENDOR_ID>/g' {} \;
```

## Logs and reports

There are different types of logs and reports that are of interest for an ACAP
application.

### Access the system log

The system log can be accessed,

- from a web browser by going to `http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi`.
- by making a call to VAPIX API [systemlog.cgi][system-log-cgi].

  ```sh
  curl --anyauth -u <username>:<password> http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi
  ```

### Access the application log

The application log is a subset of the system log that filter on entries from
the ACAP application.

For an application with application name `hello_world` (set in `manifest.json`
field `acapPackageConf.setup.appName`), the application log can be accessed,

- from a web browser by going to `http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=hello_world`.
- by making a call to VAPIX API [systemlog.cgi][system-log-cgi] and filter on application name.

   ```sh
   curl --anyauth -u <username>:<password> http://<AXIS_DEVICE_IP>/axis-cgi/admin/systemlog.cgi?appname=hello_world
   ```

### Access the server report

The system report can be accessed,

- from a web browser by going to `http://<AXIS_DEVICE_IP>/axis-cgi/serverreport.cgi`.
- by making a call to VAPIX API [serverreport.cgi][server-report-cgi].

   ```sh
   curl --anyauth -u <username>:<password> http://<AXIS_DEVICE_IP>/axis-cgi/serverreport.cgi
   ```

## Proxy

To develop or run ACAP applications from inside a network with proxy, additional
configuration is needed. More information about proxy and instructions on
how to configure are available in the ACAP documentation [Proxy][proxy] page, or
specifically in the [Proxy in build time][proxy-in-build-time] and
[Proxy in runtime][proxy-in-runtime] sections.

<!-- Links to external references -->
<!-- markdownlint-disable MD034 -->
[access-device-through-ssh]: https://developer.axis.com/acap/get-started/set-up-developer-environment/set-up-device-advanced/#access-the-device-through-ssh
[developer-mode]: https://developer.axis.com/acap/get-started/set-up-developer-environment/set-up-device-advanced/#developer-mode
[get-vendorid]: https://developer.axis.com/acap/get-started/set-up-developer-environment/set-up-device-advanced/#step-2-get-vendorid-to-use
[server-report-cgi]: https://www.axis.com/vapix-library/subjects/t10175981/section/t10036044/display?section=t10036044-t10003915
[system-log-cgi]: https://www.axis.com/vapix-library/subjects/t10175981/section/t10036044/display?section=t10036044-t10003913
[proxy]: https://developer.axis.com/acap/develop/proxy
[proxy-in-build-time]: https://developer.axis.com/acap/develop/proxy/#proxy-in-build-time
[proxy-in-runtime]: https://developer.axis.com/acap/develop/proxy/#proxy-in-runtime
<!-- markdownlint-enable MD034 -->
