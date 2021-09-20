ARG ARCH=armv7hf
ARG VERSION=3.4.2
ARG UBUNTU_VERSION=20.04
ARG REPO=axisecp
ARG SDK=acap-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    autoconf \
    libtool \
    automake

WORKDIR /opt/build
RUN git clone https://github.com/curl/curl.git

# Curl lib generate
# - Source the SDK environment script to get links to cross compiler through
# - CONFIGURE_FLAGS environment variable
WORKDIR /opt/build/curl
RUN . /opt/axis/acapsdk/environment-setup* && \
    autoreconf -fi && \
    ./configure --prefix=${PWD}/build ${CONFIGURE_FLAGS} --with-openssl && \
    make && \
    make install

# Delete manual directory from curl built result
RUN rm -rf /opt/build/curl/build/share/man

# Copy the library to application folder
ARG BUILDDIR=/opt/build/curl/build
COPY ./app /opt/app/
WORKDIR /opt/app
RUN mkdir lib && cp -r ${BUILDDIR}/lib/libcurl.so* lib

# Proxy setting for curl
ARG CURL_PROXY
ENV CURL_PROXY ${CURL_PROXY}

# Building the ACAP application
RUN . /opt/axis/acapsdk/environment-setup* && acap-build ./
