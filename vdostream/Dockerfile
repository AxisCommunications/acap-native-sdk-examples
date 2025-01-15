ARG ARCH=armv7hf
ARG VERSION=12.2.0
ARG UBUNTU_VERSION=24.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}
ARG VDO_FORMAT

# Building the ACAP application
COPY ./app /opt/app/
WORKDIR /opt/app
RUN cp /opt/app/manifest.json.${VDO_FORMAT} manifest.json && \
    . /opt/axis/acapsdk/environment-setup* && \
    acap-build .
