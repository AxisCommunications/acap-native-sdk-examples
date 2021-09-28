# Specify the architecture at build time: armv7hf/aarch64
# Should be used for getting API image
# Currently, only armv7hf is supported
ARG ARCH=armv7hf
ARG VERSION=latest
ARG UBUNTU_VERSION=20.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Building the ACAP application
COPY ./app /opt/app/
WORKDIR /opt/app

RUN . /opt/axis/acapsdk/environment-setup* && acap-build . -a 'sobel_nv12.cl'