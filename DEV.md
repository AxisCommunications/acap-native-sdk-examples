# ACAP application developer topics

This page is a collection of common topics related to ACAP application
development.

## Access the device

To log in to an Axis device shell to perform command-line options, **SSH** is
used. Instructions to set up SSH and how to log in to shell are found in the
ACAP documentation section [Access the device through
SSH][access-device-through-ssh].

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
[server-report-cgi]: https://www.axis.com/vapix-library/subjects/t10175981/section/t10036044/display?section=t10036044-t10003915
[system-log-cgi]: https://www.axis.com/vapix-library/subjects/t10175981/section/t10036044/display?section=t10036044-t10003913
<!-- markdownlint-enable MD034 -->
