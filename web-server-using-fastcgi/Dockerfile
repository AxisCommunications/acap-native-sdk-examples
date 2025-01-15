ARG ARCH=armv7hf
ARG REPO=axisecp
ARG SDK=acap-native-sdk
ARG UBUNTU_VERSION=24.04
ARG VERSION=12.2.0

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Set general arguments
ARG ARCH
ARG SDK_LIB_PATH_BASE=/opt/axis/acapsdk/sysroots/${ARCH}/usr
ARG BUILD_DIR=/opt/build

#-------------------------------------------------------------------------------
# Prepare build environment
#-------------------------------------------------------------------------------

# Install build dependencies for cross compiling libraries
RUN DEBIAN_FRONTEND=noninteractive \
    apt-get update && apt-get install -y -f --no-install-recommends \
    cmake && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

#-------------------------------------------------------------------------------
# Build uriparser library
#-------------------------------------------------------------------------------

ARG URIPARSER_VERSION=0.9.8
ARG URIPARSER_DIR=${BUILD_DIR}/uriparser
ARG URIPARSER_SRC_DIR=${BUILD_DIR}/uriparser-${URIPARSER_VERSION}
ARG URIPARSER_BUILD_DIR=${URIPARSER_DIR}/build

WORKDIR ${BUILD_DIR}
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
RUN curl -fsSL https://github.com/uriparser/uriparser/releases/download/uriparser-${URIPARSER_VERSION}/uriparser-${URIPARSER_VERSION}.tar.gz | tar -xz

WORKDIR ${URIPARSER_BUILD_DIR}
ENV COMMON_CMAKE_FLAGS="-S $URIPARSER_SRC_DIR \
        -B $URIPARSER_BUILD_DIR \
        -D CMAKE_INSTALL_PREFIX=$URIPARSER_BUILD_DIR \
        -D CMAKE_BUILD_TYPE=Release .. \
        -D URIPARSER_BUILD_DOCS=OFF \
        -D URIPARSER_BUILD_TESTS=OFF "

# hadolint ignore=SC2086
RUN if [ "$ARCH" = armv7hf ]; then \
        # Source SDK environment to get cross compilation tools
        . /opt/axis/acapsdk/environment-setup* && \
        # Configure build with CMake
        cmake \
        -D CMAKE_CXX_COMPILER=${CXX%-g++*}-g++ \
        -D CMAKE_CXX_FLAGS="${CXX#*-g++}" \
        -D CMAKE_C_COMPILER=${CC%-gcc*}-gcc \
        -D CMAKE_C_FLAGS="${CC#*-gcc}" \
        -D CPU_BASELINE=NEON,VFPV3 \
        -D ENABLE_NEON=ON \
        -D ENABLE_VFPV3=ON \
        $COMMON_CMAKE_FLAGS && \
        # Build and install uriparser
        make -j "$(nproc)" install ; \
    elif [ "$ARCH" = aarch64 ]; then \
        # Source SDK environment to get cross compilation tools
        . /opt/axis/acapsdk/environment-setup* && \
        # Configure build with CMake
        # No need to set NEON and VFP for aarch64 since they are implicitly
        # present in an any standard armv8-a implementation.
        cmake \
        -D CMAKE_CXX_COMPILER=${CXX%-g++*}-g++ \
        -D CMAKE_CXX_FLAGS="${CXX#*-g++}" \
        -D CMAKE_C_COMPILER=${CC%-gcc*}-gcc \
        -D CMAKE_C_FLAGS="${CC#*-gcc}" \
        $COMMON_CMAKE_FLAGS && \
        # Build and install uriparser
        make -j "$(nproc)" install ; \
    else \
        printf "Error: '%s' is not a valid value for the ARCH variable\n", "$ARCH"; \
        exit 1; \
    fi

WORKDIR /opt/app/lib
RUN cp -P /opt/build/uriparser/build/liburiparser.so* .

WORKDIR /opt/app
COPY ./app .
RUN . /opt/axis/acapsdk/environment-setup* && acap-build .
