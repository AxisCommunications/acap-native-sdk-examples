ARG ARCH=armv7hf
ARG VERSION=3.4.2
ARG UBUNTU_VERSION=20.04
ARG REPO=axisecp
ARG SDK=acap-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Copy the library to application folder
WORKDIR /opt/app
COPY ./app /opt/app/

# Build custom library
WORKDIR /opt/app/custom_build
RUN . /opt/axis/acapsdk/environment-setup* && make

# Copy library to application lib directory
WORKDIR /opt/app
ARG BUILDDIR=/opt/app/custom_build
RUN mkdir lib && mv ${BUILDDIR}/lib* lib

# Building the ACAP application
RUN . /opt/axis/acapsdk/environment-setup* && acap-build ./
