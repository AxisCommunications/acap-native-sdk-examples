ARG ARCH=armv7hf
ARG VERSION=1.12_rc1
ARG UBUNTU_VERSION=22.04
ARG REPO=axisecp

FROM ${REPO}/acap-native-sdk:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}
ARG ARCH
COPY ./* /opt/app/
COPY ./config/env.${ARCH} /opt/app/env.config
WORKDIR /opt/app
RUN . /opt/axis/acapsdk/environment-setup* && \
 acap-build -a alpine.tar -a docker-compose.yml -a env.config .
