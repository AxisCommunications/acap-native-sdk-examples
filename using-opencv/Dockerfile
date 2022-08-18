ARG ARCH=armv7hf
ARG REPO=axisecp
ARG SDK=acap-native-sdk
ARG UBUNTU_VERSION=20.04
ARG VERSION=1.2

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Set general arguments
ARG ARCH
ARG SDK_LIB_PATH_BASE=/opt/axis/acapsdk/sysroots/${ARCH}/usr
ARG BUILD_DIR=/opt/build

#-------------------------------------------------------------------------------
# Prepare build environment
#-------------------------------------------------------------------------------

# Install build dependencies for cross compiling OpenCV
RUN DEBIAN_FRONTEND=noninteractive \
    apt-get update && apt-get install -y -f --no-install-recommends \
    cmake \
    ca-certificates && \
    update-ca-certificates && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

#-------------------------------------------------------------------------------
# Build OpenCV libraries
#-------------------------------------------------------------------------------

ARG OPENCV_VERSION=4.5.1
ARG OPENCV_DIR=${BUILD_DIR}/opencv
ARG OPENCV_SRC_DIR=${OPENCV_DIR}/opencv-${OPENCV_VERSION}
ARG OPENCV_BUILD_DIR=${OPENCV_DIR}/build

WORKDIR ${OPENCV_DIR}
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
RUN curl -fsSL https://github.com/opencv/opencv/archive/$OPENCV_VERSION.tar.gz | tar -xz

WORKDIR ${OPENCV_BUILD_DIR}
# No need to set NEON and VFP for aarch64 since they are implicitly present in
# an any standard armv8-a implementation.
# hadolint ignore=SC2086
RUN COMMON_CMAKE_FLAGS="-S $OPENCV_SRC_DIR \
        -B $OPENCV_BUILD_DIR \
        -D CMAKE_INSTALL_PREFIX=$SDK_LIB_PATH_BASE \
        -D CMAKE_BUILD_TYPE=RELEASE \
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
        # Configure build with CMake
        cmake \
        -D CMAKE_TOOLCHAIN_FILE=${OPENCV_SRC_DIR}/platforms/linux/arm-gnueabi.toolchain.cmake \
        -D CPU_BASELINE=NEON,VFPV3 \
        -D ENABLE_NEON=ON \
        -D ENABLE_VFPV3=ON \
        $COMMON_CMAKE_FLAGS && \
        # Build and install OpenCV
        make -j "$(nproc)" install ; \
    elif [ "$ARCH" = aarch64 ]; then \
        # Configure build with CMake
        cmake \
        -D CMAKE_TOOLCHAIN_FILE=${OPENCV_SRC_DIR}/platforms/linux/aarch64-gnu.toolchain.cmake \
        $COMMON_CMAKE_FLAGS && \
        # Build and install OpenCV
        make -j "$(nproc)" install ; \
    else \
        printf "Error: '%s' is not a valid value for the ARCH variable\n", "$ARCH"; \
        exit 1; \
    fi

#-------------------------------------------------------------------------------
# Copy the built library files to application directory
#-------------------------------------------------------------------------------

WORKDIR /opt/app
COPY ./app .
RUN mkdir lib && \
    cp -P ${OPENCV_BUILD_DIR}/lib/lib*.so* ./lib/

#-------------------------------------------------------------------------------
# Finally build the ACAP application
#-------------------------------------------------------------------------------

RUN . /opt/axis/acapsdk/environment-setup* && acap-build .
