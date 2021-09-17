ARG ARCH=armv7hf
ARG VERSION=latest
ARG UBUNTU_VERSION=20.04
ARG REPO=axisecp

FROM ${REPO}/acap-native-sdk:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

COPY ./* /opt/app/
WORKDIR /opt/app
RUN . /opt/axis/acapsdk/environment-setup* && \
 acap-build -a alpine.tar -a docker-compose.yml  .
