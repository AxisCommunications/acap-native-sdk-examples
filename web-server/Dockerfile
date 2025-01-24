ARG ARCH=armv7hf
ARG REPO=axisecp
ARG SDK=acap-native-sdk
ARG UBUNTU_VERSION=24.04
ARG VERSION=12.2.0

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Set general arguments
ARG ARCH
ARG SDK_LIB_PATH_BASE=/opt/axis/acapsdk/sysroots/${ARCH}/usr
ARG APP_RPATH=/usr/local/packages/web_server_rev_proxy
ARG BUILD_DIR=/opt/build

#-------------------------------------------------------------------------------
# Build Monkey library
#-------------------------------------------------------------------------------

ARG MONKEY_VERSION=v1.5.6
ARG MONKEY_GIT_REPO=https://github.com/monkey/monkey
ARG MONKEY_DIR=${BUILD_DIR}/monkey
ARG MONKEY_SRC_DIR=${MONKEY_DIR}/src
ARG MONKEY_BUILD_DIR=${MONKEY_DIR}/build

WORKDIR ${MONKEY_DIR}
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
RUN git clone -b $MONKEY_VERSION $MONKEY_GIT_REPO ${MONKEY_SRC_DIR##*/}

# Remove unused directories
WORKDIR ${MONKEY_SRC_DIR}
RUN rm -rf \
      deps/jemalloc \
      debian* \
      src/contrib \
      examples

# Configure, build and install Monkey files
COPY monkey.patch .
RUN git apply --ignore-space-change --ignore-whitespace monkey.patch && \
    . /opt/axis/acapsdk/environment-setup* && \
    ./configure \
    --enable-shared \
    --malloc-libc \
    --prefix=${SDK_LIB_PATH_BASE} \
    --libdir=${SDK_LIB_PATH_BASE}/lib \
    --bindir=${MONKEY_BUILD_DIR}/bin \
    --incdir=${MONKEY_BUILD_DIR}/include \
    --plugdir=${APP_RPATH}/lib/plugins \
    --datadir=${APP_RPATH}/html \
    --sysconfdir=${APP_RPATH}/html \
    --logdir=${APP_RPATH}/localhost/logs \
    --mandir=${MONKEY_BUILD_DIR}/man \
    --pidfile=/tmp/monkey.pid \
    --systemddir=/usr/lib/systemd/system && \
    make && \
    make install

# Gather files in Monkey build directory to make it easy for the application
# Makefile to copy required files to the ACAP application build directory
WORKDIR ${MONKEY_BUILD_DIR}/html
ARG MONKEY_HTML_FILES="monkey.mime monkey.conf  sites plugins.load"
RUN for htmlfile in $MONKEY_HTML_FILES; do \
      cp -r "$APP_RPATH/html/$htmlfile" . ; \
    done

WORKDIR ${MONKEY_BUILD_DIR}/lib
RUN cp -P ${SDK_LIB_PATH_BASE}/lib/libmonkey* .

WORKDIR ${MONKEY_BUILD_DIR}/lib/plugins
ARG MONKEY_PLUGINS="monkey-liana"
RUN for plug in $MONKEY_PLUGINS; do \
      cp "$APP_RPATH/lib/plugins/$plug.so" . ; \
    done


#-------------------------------------------------------------------------------
# Build ACAP application
#-------------------------------------------------------------------------------

WORKDIR /opt/app
COPY ./app .
RUN . /opt/axis/acapsdk/environment-setup* && acap-build .