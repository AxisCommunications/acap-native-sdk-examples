ARG ARCH=aarch64
ARG VERSION=1.15
ARG UBUNTU_VERSION=22.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Build libjpeg-turbo
WORKDIR /opt/build
RUN apt-get update && apt-get install --no-install-recommends -y cmake && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
RUN git clone --branch 2.0.6 https://github.com/libjpeg-turbo/libjpeg-turbo.git

WORKDIR /opt/build/libjpeg-turbo/build
RUN CC=/usr/bin/aarch64-linux-gnu-gcc cmake -G"Unix Makefiles" .. && make

# Copy the built libraries to application folder
ARG BUILD_DIR_JPEG=/opt/build/libjpeg-turbo
WORKDIR /opt/app/lib
RUN cp ${BUILD_DIR_JPEG}/build/*.so* .
WORKDIR /opt/app/include
RUN cp ${BUILD_DIR_JPEG}/build/*.h . && \
    cp ${BUILD_DIR_JPEG}/*.h . 

WORKDIR /opt/app
COPY ./app /opt/app/

# Download the labels
RUN mkdir -p label && \
    curl -L -o label/labels.txt https://github.com/google-coral/test_data/raw/master/coco_labels.txt

# Setup the model directory and get pretrained models
RUN mkdir -p model && \
    curl -o model/converted_model.bin https://acap-ml-model-storage.s3.amazonaws.com/SSD_Mobilenet_Ambarella/ambarella_ssd.bin && \
    curl -o model/anchor_boxes.bin  https://acap-ml-model-storage.s3.amazonaws.com/SSD_Mobilenet_Ambarella/anchor_boxes.bin

# Building the ACAP application
RUN cp /opt/app/manifest.json.cv25 /opt/app/manifest.json && \
    . /opt/axis/acapsdk/environment-setup* && acap-build . \
    -a 'label/labels.txt' -a 'model/converted_model.bin' -a 'model/anchor_boxes.bin'
