# ACAP application developer topics

This page is a collection of common topics related to ACAP application
development.

## Access the device

To log in to an Axis device shell to perform command-line options, **SSH** is
used. Instructions to set up SSH and how to log in to shell are found in the
ACAP documentation section [Access the device through
SSH][access-device-through-ssh].

## Developer Mode

Developer Mode is a feature introduced in AXIS OS 11.11 that grants SSH access
to an ACAP application's dynamic user on an Axis device. When Developer Mode is
enabled, the ACAP application's dynamic user is added to the `ssh` group.

To enable Developer Mode on Axis devices running AXIS OS 11.11 or later, install
the `axis-unlock-acap-devmode` Custom Firmware Certificate (CFC) on the Axis
device.

For detailed instructions on how to obtain and install the CFC, as well as how to
verify and disable Developer Mode, please refer to the [Developer Mode][developer-mode]
section in the ACAP documentation.

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

<!-- Links to external references -->
<!-- markdownlint-disable MD034 -->
[access-device-through-ssh]: https://axiscommunications.github.io/acap-documentation/docs/get-started/set-up-developer-environment/set-up-device-advanced.html#access-the-device-through-ssh
[developer-mode]: https://axiscommunications.github.io/acap-documentation/docs/get-started/set-up-developer-environment/set-up-device-advanced.html#developer-mode
[server-report-cgi]: https://www.axis.com/vapix-library/subjects/t10175981/section/t10036044/display?section=t10036044-t10003915
[system-log-cgi]: https://www.axis.com/vapix-library/subjects/t10175981/section/t10036044/display?section=t10036044-t10003913
<!-- markdownlint-enable MD034 -->
