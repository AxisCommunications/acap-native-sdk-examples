# syntax=docker/dockerfile:1

ARG ARCH=armv7hf
ARG VERSION=12.5.0
ARG UBUNTU_VERSION=24.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Building the ACAP application
COPY ./app /opt/app/
WORKDIR /opt/app
RUN <<EOF
. /opt/axis/acapsdk/environment-setup*
acap-build .
EOF