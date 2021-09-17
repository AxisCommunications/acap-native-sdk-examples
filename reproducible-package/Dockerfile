ARG ARCH=armv7hf
ARG VERSION=latest
ARG UBUNTU_VERSION=20.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

ARG TIMESTAMP=

# Building the ACAP application
COPY ./app /opt/app/
WORKDIR /opt/app
RUN . /opt/axis/acapsdk/environment-setup* && \
	export SOURCE_DATE_EPOCH=${TIMESTAMP} && \
	acap-build ./
