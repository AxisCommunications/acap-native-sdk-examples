 *Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# An example of how to create a reproducible ACAP application

This README file explains how to build a simple and reproducible ACAP application. It is achieved by using the containerized API and toolchain images.

Together with this README file, you should be able to find a directory called app. That directory contains the "reproducible-package" application source code which can easily be compiled and run with the help of the tools and step by step below.

The example follows the guidelines in [Reproducible builds practices](https://reproducible-builds.org/)
and the steps taken in this example are:

* Set *SOURCE_DATE_EPOCH* to a specific date, in Dockerfile for standard
  procedure and in connection to build command `acap-build` if running
  interactively.
* Add a code section in Makefile that if *SOURCE_DATE_EPOCH*  is set, do set
  *BUILD_DATE* to the specified date.

**N.b.** These are the steps for a very basic example but might not be enough
for more complex applications. See the link above for more tips on how to get
your application reproducible.

## Getting started

Below is the structure and scripts used in the example:

```bash
reproducible-package
├── app
│   ├── reproducible_package.c
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── Dockerfile
└── README.md
```

* **app/reproducible_package.c** - Hello World application which writes to system-log.
* **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
* **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
* **app/manifest.json** - Defines the application and its configuration.
* **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
* **README.md** - Step by step instructions on how to run the example.

### Limitations

* The support to create reproducible packages was introduced in acap-sdk 3.4.

### Build the application

The following build steps demonstrates how an application may be reproducible
and not how a real application would be built.

Standing in your working directory follow the commands below:

> **Note**
>
> Depending on the network you are connected to, you may need to add proxy settings.
The file that needs those settings is:* ~/.docker/config.json. *For
reference please see: <https://docs.docker.com/network/proxy/> and a
[script for Axis device here](../FAQs.md#HowcanIset-upnetworkproxysettingsontheAxisdevice?).*

In all examples the option `--no-cache` is used to ensure that image is rebuilt.

Start with an ordinary build which will not give a reproducible package and
copy the result from the container image to a local directory *build1*.

```bash
docker build --no-cache --tag rep:1 .
docker cp $(docker create rep:1):/opt/app ./build1
```

Now let's create a reproducible package. Set the build argument TIMESTAMP which
in turn set [SOURCE_DATE_EPOCH](https://reproducible-builds.org/docs/source-date-epoch/)
to a fix time. The chosen timestamp here is the latest commit in the current
git repository. Copy the output to *build2*.

```bash
docker build --no-cache --build-arg TIMESTAMP="$(git log -1 --pretty=%ct)" --tag rep:2 .
docker cp $(docker create rep:2):/opt/app ./build2
```

Build a second reproducible application and copy the output to *build3*.

```bash
docker build --no-cache --build-arg TIMESTAMP="$(git log -1 --pretty=%ct)" --tag rep:3 .
docker cp $(docker create rep:3):/opt/app ./build3
```

Now you will have three eap-files in your working directory

```bash
reproducible-package
├── build1
│   └── reproducible_package_1_0_0_armv7hf.eap
├── build2
│   └── reproducible_package_1_0_0_armv7hf.eap
├── build3
│   └── reproducible_package_1_0_0_armv7hf.eap
```

* **build1/reproducible_package_1_0_0_armv7hf.eap** - Application package .eap file.
* **build2/reproducible_package_1_0_0_armv7hf.eap** - Reproducible application package .eap file.
* **build3/reproducible_package_1_0_0_armv7hf.eap** - Reproducible application package .eap file.

Compare the files to see if they are identical or not.

```bash
cmp build1/*.eap build2/*.eap
build1/reproducible_package_1_0_0_armv7hf.eap build2/reproducible_package_1_0_0_armv7hf.eap differ: byte 13, line 1

cmp build2/*.eap build3/*.eap

```

All these steps are also available in a utility script

```bash
./reproducible_package.sh
```

#### Build the application interactively

To get reproducible packages running inside a container, make sure to export the
environment variable before running build command.

```bash
SOURCE_DATE_EPOCH=$(git log -1 --pretty=%ct) acap-build .
```

**N.b.** To be able to use the git log as in this example you will have to run
the docker container from the top directory where the `.git` directory is placed.

## License

**[Apache License 2.0](../LICENSE)**
