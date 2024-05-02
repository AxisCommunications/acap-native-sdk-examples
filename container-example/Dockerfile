# syntax=docker/dockerfile:1

ARG ARCH=armv7hf
ARG VERSION=1.14_rc3
ARG UBUNTU_VERSION=22.04
ARG REPO=axisecp

FROM ${REPO}/acap-native-sdk:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION} AS build
ARG ARCH
COPY ./* /opt/app/

WORKDIR /opt/app
RUN <<EOF
    . /opt/axis/acapsdk/environment-setup*
    acap-build . \
        -a alpine.tar \
        -a docker-compose.yml
EOF

FROM scratch AS binaries

COPY --from=build /opt/app/*.eap /
