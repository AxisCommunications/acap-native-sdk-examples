ARG ARCH=aarch64
ARG VERSION=12.6.0
ARG UBUNTU_VERSION=24.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Set general arguments
ARG ARCH
ARG SDK_LIB_PATH_BASE=/opt/axis/acapsdk/sysroots/${ARCH}/usr
ARG BUILD_DIR=/opt/build

#-------------------------------------------------------------------------------
# Prepare build environment
#-------------------------------------------------------------------------------

# Install build dependencies for library cross compilation
RUN DEBIAN_FRONTEND=noninteractive \
    apt-get update && apt-get install -y -f --no-install-recommends \
    cmake && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

#-------------------------------------------------------------------------------
# Build libjpeg-turbo
#-------------------------------------------------------------------------------

ARG JPEGTURBO_VERSION=3.0.3
ARG JPEGTURBO_GIT_REPO=https://github.com/libjpeg-turbo/libjpeg-turbo
ARG JPEGTURBO_VERSION_DIR=libjpeg-turbo-${JPEGTURBO_VERSION}
ARG JPEGTURBO_DIR=${BUILD_DIR}/libjpeg-turbo
ARG JPEGTURBO_SRC_DIR=${JPEGTURBO_DIR}/${JPEGTURBO_VERSION_DIR}
ARG JPEGTURBO_BUILD_DIR=${JPEGTURBO_DIR}/build

WORKDIR ${JPEGTURBO_DIR}
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
RUN curl -fsSL ${JPEGTURBO_GIT_REPO}/releases/download/${JPEGTURBO_VERSION}/${JPEGTURBO_VERSION_DIR}.tar.gz | tar -xz

WORKDIR ${JPEGTURBO_SRC_DIR}
RUN . /opt/axis/acapsdk/environment-setup* && \
    pwd && ls -la && \
    cmake -G"Unix Makefiles" \
      -DCMAKE_INSTALL_PREFIX:PATH=${JPEGTURBO_BUILD_DIR} \
      . && \
      make -j && \
      make install

#-------------------------------------------------------------------------------
# Get models and labels
#-------------------------------------------------------------------------------

# Download pretrained models
WORKDIR /opt/app/model
ARG CHIP=cv25
ARG MODEL_BUCKET=https://acap-ml-models.s3.amazonaws.com/ssd_mobilenet
RUN curl -o model.bin $MODEL_BUCKET/ssd_mobilenet_v2_cv25_coco_300.bin && \
    curl -o anchor_boxes.bin $MODEL_BUCKET/ssd_mobilenet_v2_cv25_coco_300_anchor_boxes.bin

WORKDIR /opt/app/label
RUN curl -L -o labels.txt https://github.com/google-coral/test_data/raw/master/coco_labels.txt

#-------------------------------------------------------------------------------
# Build ACAP application
#-------------------------------------------------------------------------------

WORKDIR /opt/app
COPY ./app .
RUN cp /opt/app/manifest.json.${CHIP} /opt/app/manifest.json && \
    . /opt/axis/acapsdk/environment-setup* && acap-build . \
    -a 'label/labels.txt' \
    -a 'model/model.bin' \
    -a 'model/anchor_boxes.bin'
