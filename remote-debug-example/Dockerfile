ARG ARCH=armv7hf
ARG VERSION=12.5.0
ARG UBUNTU_VERSION=24.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION} AS sdk-dev-base

# Install gdb-multiarch to run debug on host by connecting to gdbserver on device
# in addition, install ssh to scp gdbserver to device
RUN DEBIAN_FRONTEND=noninteractive \
    apt-get update && apt-get install -y -f --no-install-recommends \
    gdb-multiarch \
    ssh \
    && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Build GDB server
ARG GDB_VERSION=9.2
ARG GDB_REPOSITORY=https://ftp.gnu.org/gnu/gdb
ARG GDB_ARCHIVE=gdb-${GDB_VERSION}.tar.gz
ARG GDB_BASE_DIR=/opt/build/gdb
ARG GDB_SOURCE_DIR=$GDB_BASE_DIR/src
ARG GDB_SERVER_CONF_DIR=$GDB_SOURCE_DIR/gdb/gdbserver
ARG GDB_BUILD_DIR=${GDB_BASE_DIR}/build
# The gdbserver will be installed to $GDB_INSTALL_DIR/bin/gdbserver
ARG GDB_INSTALL_DIR=${GDB_BASE_DIR}
ARG GDB_SERVER_BIN_DIR=${GDB_BASE_DIR}/bin

WORKDIR $GDB_BASE_DIR
RUN curl -sSL -O $GDB_REPOSITORY/$GDB_ARCHIVE

WORKDIR $GDB_SOURCE_DIR
RUN tar -xf $GDB_BASE_DIR/$GDB_ARCHIVE -C $GDB_SOURCE_DIR --strip-components=1

WORKDIR $GDB_BUILD_DIR
# hadolint ignore=SC2086
RUN . /opt/axis/acapsdk/environment-setup* && \
    $GDB_SERVER_CONF_DIR/configure \
      --host=$($CC -dumpmachine) \
      --prefix=${GDB_INSTALL_DIR} \
      --with-sysroot=${SDKTARGETSYSROOT}

RUN make && \
    make install

# These final steps are added for test purposes and are not required for debugging
# in a container where the application source code is mounted on, thus replacing, this
# path.
FROM sdk-dev-base AS build-app
WORKDIR /opt/app
COPY ./app /opt/app/
RUN . /opt/axis/acapsdk/environment-setup* && acap-build ./
