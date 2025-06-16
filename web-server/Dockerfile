ARG ARCH=armv7hf
ARG REPO=axisecp
ARG SDK=acap-native-sdk
ARG UBUNTU_VERSION=24.04
ARG VERSION=12.4.0

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Set general arguments
ARG ARCH
ARG SDK_LIB_PATH_BASE=/opt/axis/acapsdk/sysroots/${ARCH}/usr
ARG APP_RPATH=/usr/local/packages/web_server_rev_proxy
ARG BUILD_DIR=/opt/build

#-------------------------------------------------------------------------------
# Build Civetweb library
#-------------------------------------------------------------------------------
ARG CIVETWEB_BUILD_DIR=${BUILD_DIR}/civetweb

WORKDIR ${BUILD_DIR}
RUN git clone https://github.com/civetweb/civetweb.git

WORKDIR ${CIVETWEB_BUILD_DIR}
RUN . /opt/axis/acapsdk/environment-setup* && make lib
WORKDIR /opt/app
RUN mkdir lib && cp ${CIVETWEB_BUILD_DIR}/libcivetweb.a lib

#-------------------------------------------------------------------------------
# Build ACAP application
#-------------------------------------------------------------------------------

COPY ./app .
RUN . /opt/axis/acapsdk/environment-setup* && acap-build .
