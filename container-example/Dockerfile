# syntax=docker/dockerfile:1

ARG ARCH=aarch64
ARG VERSION=12.8.0
ARG UBUNTU_VERSION=24.04
ARG REPO=axisecp

FROM ${REPO}/acap-native-sdk:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}
ARG ARCH
COPY ./* /opt/app/

WORKDIR /opt/app
RUN <<EOF
    . /opt/axis/acapsdk/environment-setup*
    acap-build . \
        -a alpine.tar \
        -a docker-compose.yml
EOF
