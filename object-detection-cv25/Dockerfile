ARG ARCH=aarch64
ARG VERSION=1.6
ARG UBUNTU_VERSION=22.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Build libyuv
WORKDIR /opt/build
# TODO: Investigate why server certs can't be verified
RUN GIT_SSL_NO_VERIFY=1 git clone -n https://chromium.googlesource.com/libyuv/libyuv

WORKDIR /opt/build/libyuv
ARG libyuv_version=5b6042fa0d211ebbd8b477c7f3855977c7973048
RUN git checkout ${libyuv_version}
COPY yuv/*.patch /opt/build/libyuv
RUN git apply  --ignore-space-change --ignore-whitespace ./*.patch && \
        make -j -f linux.mk CXX=/usr/bin/aarch64-linux-gnu-g++ CC=/usr/bin/aarch64-linux-gnu-gcc && \
        aarch64-linux-gnu-strip --strip-unneeded libyuv.so*


# Build libjpeg-turbo
WORKDIR /opt/build
RUN apt-get update && apt-get install --no-install-recommends -y cmake && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
RUN git clone --branch 2.0.6 https://github.com/libjpeg-turbo/libjpeg-turbo.git

WORKDIR /opt/build/libjpeg-turbo/build
RUN CC=/usr/bin/aarch64-linux-gnu-gcc cmake -G"Unix Makefiles" .. && make

# Copy the built libraries to application folder
WORKDIR /opt/app
COPY ./app /opt/app/
ARG BUILDDIR=/opt/build/libyuv
RUN mkdir -p lib include && \
    cp /opt/build/libjpeg-turbo/build/*.so* lib/ && \
    cp /opt/build/libjpeg-turbo/build/*.h include/ && \
    cp /opt/build/libjpeg-turbo/*.h include/ && \
    cp ${BUILDDIR}/libyuv.so* lib/ && \
    cp -a ${BUILDDIR}/include/. include && \
    ln -s libyuv.so.1 lib/libyuv.so && \
    ln -s libyuv.so.1 lib/libyuv.so.1.0



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
