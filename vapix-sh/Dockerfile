ARG ARCH=aarch64
ARG VERSION=12.1.0
ARG UBUNTU_VERSION=24.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

WORKDIR /opt/app
COPY ./app .
RUN . /opt/axis/acapsdk/environment-setup* && \
    acap-build .
