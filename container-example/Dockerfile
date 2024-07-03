# syntax=docker/dockerfile:1

ARG ARCH=armv7hf
ARG VERSION=1.15
ARG UBUNTU_VERSION=22.04
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
