ARG ARCH=armv7hf
ARG VERSION=1.12
ARG UBUNTU_VERSION=22.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION} as sdk

# Set arguments used in build of both libraries
ARG ARCH=armv7hf
ARG SDK_LIB_PATH_BASE=/opt/axis/acapsdk/sysroots/${ARCH}/usr
ARG APP_RPATH=/usr/local/packages/vapix_example
ARG BUILD_DIR=/opt/build

# Library versions
ARG CURL_VER=8_4_0
ARG JANSSON_VER=2.14

#-------------------------------------------------------------------------------
# Prepare build environment
#-------------------------------------------------------------------------------

# Delete libraries in SDK to avoid linking to them at build time. This is a
# safety precaution since all shared libraries should use the libc version from
# the SDK in build time.
RUN bash -c 'rm -vf  ${SDK_LIB_PATH_BASE}/lib/lib{curl,jansson}.so*' && \
    bash -c 'rm -vf  ${SDK_LIB_PATH_BASE}/lib/pkgconfig/{libcurl,jansson}.pc' && \
    bash -c 'rm -vfr ${SDK_LIB_PATH_BASE}/include/{curl,jansson}*'

# Install build dependencies for cross compilation
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    autoconf \
    libtool \
    automake && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

#-------------------------------------------------------------------------------
# Build jansson library
#-------------------------------------------------------------------------------

ARG JANSSON_BUILD_PATH=${BUILD_DIR}/jansson
ARG JANSSON_BUILD_DIR=${JANSSON_BUILD_PATH}/jansson-${JANSSON_VER}
ARG JANSSON_INSTALL_DIR=${JANSSON_BUILD_PATH}/install

WORKDIR ${JANSSON_BUILD_PATH}
RUN curl -LO https://github.com/akheron/jansson/releases/download/v$JANSSON_VER/jansson-$JANSSON_VER.tar.gz && \
    tar xzvf jansson-$JANSSON_VER.tar.gz

WORKDIR ${JANSSON_BUILD_DIR}
# hadolint ignore=SC2086
RUN . /opt/axis/acapsdk/environment-setup* && \
    ./configure \
    --prefix="${JANSSON_INSTALL_DIR}" \
    $CONFIGURE_FLAGS && \
    make && \
    make install


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
    --disable-alt-svc \
    --disable-ares \
    --disable-aws \
    --disable-bearer-auth \
    --disable-cookies \
    --disable-dateparse \
    --disable-digest-auth \
    --disable-dnsshuffle \
    --disable-doh \
    --disable-get-easy-options \
    --disable-hsts \
    --disable-ipv6 \
    --disable-kerberos-auth \
    --disable-libcurl-option \
    --disable-manual \
    --disable-negotiate-auth \
    --disable-netrc \
    --disable-ntlm-wb \
    --disable-progress-meter \
    --disable-proxy \
    --disable-pthreads \
    --disable-socketpair \
    --disable-threaded-resolver \
    --disable-tls-srp \
    --disable-unix-sockets \
    --disable-verbose \
    --disable-versioned-symbols \
    --enable-symbol-hiding \
    --without-brotli \
    --without-libidn2 \
    --without-libpsl \
    --without-librtmp \
    --without-nghttp2 \
    --without-ngtcp2 \
    --without-ssl \
    --without-zlib \
    --without-zstd \
    --prefix="${CURL_INSTALL_DIR}" \
    ${CONFIGURE_FLAGS} && \
    make && \
    make install

#-------------------------------------------------------------------------------
# Finally build the ACAP application
#-------------------------------------------------------------------------------

# The libraries will be copied in the Makefile
WORKDIR /opt/app
COPY ./app .
RUN . /opt/axis/acapsdk/environment-setup* && \
    acap-build .