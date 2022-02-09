ARG ARCH=armv7hf
ARG REPO=axisecp
ARG SDK=acap-native-sdk
ARG UBUNTU_VERSION=20.04
ARG VERSION=1.1

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

ARG OPENCV_VERSION=4.5.1
ARG TARGET_ROOT=/target-root

# Add a sources.list file that contains the armhf repositories
# Get crosscompilation toolchain
ARG ARCH
ENV DEBIAN_FRONTEND=noninteractive
COPY sources.list /etc/apt

RUN if [ "$ARCH" = armv7hf ]; then \
        apt-get update && \
        dpkg --add-architecture armhf && \
        apt-get update && apt-get install -yf --no-install-recommends \
        ca-certificates \
        crossbuild-essential-armhf && \
        apt-get clean && \
        rm -rf /var/lib/apt/lists/* ; \
    elif [ "$ARCH" = aarch64 ]; then \
        apt-get update && \
        dpkg --add-architecture arm64 && \
        apt-get update && apt-get install -yf --no-install-recommends \
        ca-certificates \
        crossbuild-essential-arm64 && \
        apt-get clean && \
        rm -rf /var/lib/apt/lists/* ;\
    else \
        printf "Error: '%s' is not a valid value for the ARCH variable\n", "$ARCH"; \
        exit 1; \
    fi

RUN apt-get update && apt-get install -y -f --no-install-recommends \
    cmake \
    curl \
    pkg-config && \
    update-ca-certificates && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN if [ "$ARCH" = armv7hf ]; then \
        export ARCHDIR=arm-linux-gnueabihf && \
        export STRIP=arm-linux-gnueabihf-strip; \
    elif [ "$ARCH" = aarch64 ]; then \
        export ARCHDIR=g++-aarch64-linux-gnu && \
        export STRIP=aarch64-linux-gnu-strip ;\
    else \
        printf "Error: '%s' is not a valid value for the ARCH variable\n", "$ARCH"; \
        exit 1; \
    fi
ENV PKG_CONFIG_LIBDIR=/usr/share/pkgconfig:/usr/lib/$ARCHDIR/pkgconfig:/usr/lib/$ARCHDIR/pkgconfig

# Download OpenCV
WORKDIR /workspace
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
RUN curl -fsSL https://github.com/opencv/opencv/archive/$OPENCV_VERSION.tar.gz | tar -xz

WORKDIR /workspace/opencv-$OPENCV_VERSION/build
# Configure OpenCV
# Platform specific optimizations in the form of NEON and VFPV3 are enabled
# hadolint ignore=SC2086
RUN COMMON_CMAKE_FLAGS="-D CMAKE_BUILD_TYPE=RELEASE \
        -D CMAKE_INSTALL_PREFIX=$TARGET_ROOT/usr \
        -D WITH_OPENEXR=OFF \
        -D WITH_GTK=OFF \
        -D WITH_V4L=OFF \
        -D WITH_FFMPEG=OFF \
        -D WITH_GSTREAMER=OFF \
        -D WITH_GSTREAMER_0_10=OFF \
        -D BUILD_LIST=core,imgproc,video \
        -D BUILD_EXAMPLES=OFF \
        -D BUILD_OPENCV_APPS=OFF \
        -D BUILD_DOCS=OFF \
        -D BUILD_JPEG=ON \
        -D BUILD_PNG=OFF \
        -D WITH_JASPER=OFF \
        -D BUILD_PROTOBUF=OFF \
        -D OPENCV_GENERATE_PKGCONFIG=ON " && \
    if [ "$ARCH" = armv7hf ]; then \
        cmake -D CMAKE_TOOLCHAIN_FILE=../platforms/linux/arm-gnueabi.toolchain.cmake \
        -D ENABLE_NEON=ON \
        -D CPU_BASELINE=NEON,VFPV3 \
        -D ENABLE_VFPV3=ON \
        $COMMON_CMAKE_FLAGS \
        .. && \
        # Build openCV libraries and other tools
        make -j "$(nproc)" install ; \
    elif [ "$ARCH" = aarch64 ]; then \
        #Here -D CPU_BASELINE has been removed, OpenCVCompilerOptimizations.cmake does not use any specific option for AARCH64, since, NEON and VFP are implicitly present in an any standard armv8-a implementation 
        cmake -D CMAKE_TOOLCHAIN_FILE=../platforms/linux/aarch64-gnu.toolchain.cmake \
        $COMMON_CMAKE_FLAGS \
        .. && \
        # Build openCV libraries and other tools
        make -j "$(nproc)" install ; \
    else \
        printf "Error: '%s' is not a valid value for the ARCH variable\n", "$ARCH"; \
        exit 1; \
    fi
COPY app/ /opt/app
WORKDIR /opt/app
RUN . /opt/axis/acapsdk/environment-setup* && acap-build .
