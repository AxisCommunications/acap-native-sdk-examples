ARG ARCH=armv7hf
ARG VERSION=12.0.0
ARG UBUNTU_VERSION=24.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION} as sdk

# Set arguments used in build of both libraries
ARG ARCH=armv7hf
ARG SDK_LIB_PATH_BASE=/opt/axis/acapsdk/sysroots/${ARCH}/usr
ARG APP_RPATH=/usr/local/packages/openssl_curl_example
ARG BUILD_DIR=/opt/build
ARG PEM_CERT_FILE=cacert.pem

# Library versions
ARG OPENSSL_VER=1.1.1m
ARG CURL_VER=7_83_1

# (Optional) If the network has a proxy
ARG APP_PROXY
ENV APP_PROXY ${APP_PROXY}

# (Optional) Get more verbose logging when running the application
ARG APP_DEBUG
ENV APP_DEBUG ${APP_DEBUG}

#-------------------------------------------------------------------------------
# Prepare build environment
#-------------------------------------------------------------------------------

# Delete OpenSSL and curl libraries in SDK to avoid linking to them in build
# time. This is a safety precaution since all shared libraries should use the
# libc version from the SDK in build time.
WORKDIR ${SDK_LIB_PATH_BASE}/lib
RUN [ -z "$(ls libcrypto.so* libssl.so* libcurl.so*)" ] || \
    rm -f libcrypto.so* libssl.so* libcurl.so*

WORKDIR ${SDK_LIB_PATH_BASE}/lib/pkgconfig
RUN [ -z "$(ls libssl.pc libcrypto.pc openssl.pc libcurl.pc)" ] || \
    rm -f libssl.pc libcrypto.pc openssl.pc libcurl.pc

WORKDIR ${SDK_LIB_PATH_BASE}/include
RUN [ -z "$(ls openssl crypto curl)" ] || \
    rm -rf openssl crypto curl

# Install build dependencies for cross compiling OpenSSL and curl
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    autoconf \
    libtool \
    automake && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

#-------------------------------------------------------------------------------
# Build OpenSSL libraries
#-------------------------------------------------------------------------------

ARG OPENSSL_BUILD_PATH=${BUILD_DIR}/openssl
ARG OPENSSL_BUILD_DIR=${OPENSSL_BUILD_PATH}/openssl-${OPENSSL_VER}
ARG OPENSSL_INSTALL_DIR=${SDK_LIB_PATH_BASE}

WORKDIR ${OPENSSL_BUILD_PATH}
RUN curl -O https://www.openssl.org/source/openssl-${OPENSSL_VER}.tar.gz && \
    tar xzvf openssl-${OPENSSL_VER}.tar.gz

WORKDIR ${OPENSSL_BUILD_DIR}
RUN if [ "$ARCH" = "armv7hf" ]; then \
        ARCH_LIB=linux-armv4 ;\
    elif [ "$ARCH" = "aarch64" ]; then \
        ARCH_LIB=linux-aarch64 ;\
    else \
        echo "Error: ARCH '${ARCH}' is not supported"; \
        exit 1; \
    fi && \
    # Sourcing the SDK environment to get correct cross compiler, but unset
    # conflicting environment variables used by OpenSSL like CROSS_COMPILE
    . /opt/axis/acapsdk/environment-setup* && \
    unset CROSS_COMPILE && \
    # Configure build options
    ./Configure \
        ${ARCH_LIB} \
        no-deprecated \
        shared \
        --strict-warnings \
        # Install the OpenSSL shared object(.so), header(.h) and pkgconfig(.pc)
        # files to the SDK library path in order to help curl link to them and
        # other dependencies like libc correctly in build time.
        --prefix=${OPENSSL_INSTALL_DIR} \
        --openssldir=${APP_RPATH} \
        "-Wl,-rpath,${APP_RPATH}/lib" && \
    ./configdata.pm --dump
RUN make && \
    make install_sw

#-------------------------------------------------------------------------------
# Build curl library
#-------------------------------------------------------------------------------

ARG CURL_BUILD_PATH=${BUILD_DIR}/curl
ARG CURL_BUILD_DIR=${CURL_BUILD_PATH}/curl-${CURL_VER}
ARG CURL_INSTALL_DIR=${CURL_BUILD_PATH}/install

# Clone a curl tag in to a versioned directory
WORKDIR ${CURL_BUILD_PATH}
RUN git clone https://github.com/curl/curl.git --branch=curl-${CURL_VER} curl-${CURL_VER}

WORKDIR ${CURL_BUILD_DIR}
# CONFIGURE_FLAGS need to be split
# hadolint ignore=SC2086
RUN . /opt/axis/acapsdk/environment-setup* && \
    autoreconf -fi && \
    LDFLAGS="${LDFLAGS} -Wl,-rpath,${APP_RPATH}/lib" \
    ./configure \
      --with-openssl \
      --without-zlib \
      --without-zstd \
      --prefix="${CURL_INSTALL_DIR}" \
      ${CONFIGURE_FLAGS} && \
    make && \
    make install

#-------------------------------------------------------------------------------
# Copy the built library files to application directory
#-------------------------------------------------------------------------------

WORKDIR /opt/app
COPY ./app .
RUN mkdir lib && \
    cp -r ${CURL_INSTALL_DIR}/lib/libcurl.so* lib && \
    cp -r ${OPENSSL_BUILD_DIR}/libssl.so* lib && \
    cp -r ${OPENSSL_BUILD_DIR}/libcrypto.so* lib

#-------------------------------------------------------------------------------
# Get CA certificate for the web server we want to transfer data from
#-------------------------------------------------------------------------------

# Use the 'openssl' tool from the Ubuntu container to get a CA certificate from
# the web server of interest. Why not use the compiled 'openssl' binary to do
# this? It's cross compiled and will not run on a standard desktop.
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
# APP_PROXY_SETTING need to be split
# hadolint ignore=SC2086
RUN APP_PROXY_SETTING= ; \
    [ -z "$APP_PROXY" ] || APP_PROXY_SETTING="-proxy $APP_PROXY" ; \
    echo quit | openssl s_client \
    -showcerts \
    -servername www.example.com \
    -connect www.example.com:443 \
    $APP_PROXY_SETTING \
    > ${PEM_CERT_FILE}

#-------------------------------------------------------------------------------
# Finally build the ACAP application
#-------------------------------------------------------------------------------

RUN . /opt/axis/acapsdk/environment-setup* && \
    acap-build . -a ${PEM_CERT_FILE}
