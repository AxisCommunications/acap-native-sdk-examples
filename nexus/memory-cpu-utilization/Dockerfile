ARG ARCH=armv7hf
ARG VERSION=12.8.0
ARG UBUNTU_VERSION=24.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# Download and install nlohmann
RUN curl -sL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz | \
    tar xJ && \
    mv json/single_include/nlohmann /tmp/nlohmann && \
    cp -r /tmp/nlohmann /opt/axis/acapsdk/sysroots/*/usr/include/ && \
    rm -rf /tmp/nlohmann json-develop

# Build the ACAP application
COPY ./app /opt/app/
WORKDIR /opt/app
RUN . /opt/axis/acapsdk/environment-setup* && acap-build .
