# Specify the architecture at build time: armv7hf
# Should be used for getting API and toolchain image
# Currently, only armv7hf is supported
ARG http_proxy
ARG https_proxy
ARG ARCH=armv7hf
ARG VERSION=latest
ARG UBUNTU_VERSION=20.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Build libyuv
WORKDIR /opt/build
ARG libyuv_version=5b6042fa0d211ebbd8b477c7f3855977c7973048
# TODO: Investigate why server certs can't be verified
RUN GIT_SSL_NO_VERIFY=1 git clone -n https://chromium.googlesource.com/libyuv/libyuv && \
    cd libyuv && git checkout ${libyuv_version}

WORKDIR /opt/build/libyuv
COPY yuv/*.patch /opt/build/libyuv
RUN git apply *.patch && \
    CXXFLAGS=' -O2 -mthumb -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a9 -fomit-frame-pointer' \
    make -f linux.mk CXX=arm-linux-gnueabihf-g++ CC=arm-linux-gnueabihf-gcc && \
    arm-linux-gnueabihf-strip --strip-unneeded libyuv.so*

# Build libjpeg-turbo
WORKDIR /opt/build
RUN apt-get update && apt-get install -y cmake
RUN git clone --branch 2.0.6 https://github.com/libjpeg-turbo/libjpeg-turbo.git
WORKDIR /opt/build/libjpeg-turbo/build
RUN CFLAGS=' -O2 -mthumb -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a9 -fomit-frame-pointer' \
  CC=arm-linux-gnueabihf-gcc cmake -G"Unix Makefiles" ..
RUN make
WORKDIR /opt/app
RUN mkdir -p lib include
RUN cp /opt/build/libjpeg-turbo/build/*.so* lib/
RUN cp /opt/build/libjpeg-turbo/build/*.h include/
RUN cp /opt/build/libjpeg-turbo/*.h include/

# Copy the libyuv to application folder
ARG BUILDDIR=/opt/build/libyuv
WORKDIR /opt/app
COPY ./app /opt/app/
RUN mkdir -p lib include && \
    cp ${BUILDDIR}/libyuv.so* lib/ && \
    cp -a ${BUILDDIR}/include/. include && \
    cd lib/ && \
    ln -s libyuv.so.1 libyuv.so && \
    ln -s libyuv.so.1 libyuv.so.1.0

# Get pretrained models
ADD https://github.com/google-coral/test_data/raw/master/ssd_mobilenet_v2_coco_quant_postprocess_edgetpu.tflite converted_model_edgetpu.tflite

# Download the labels
RUN mkdir -p label
RUN	curl -L -o label/labels.txt https://github.com/google-coral/test_data/raw/master/coco_labels.txt

# Setup the model directory
RUN	mkdir -p model
RUN cp converted_model_edgetpu.tflite model/converted_model.tflite


# Building the ACAP application
RUN . /opt/axis/acapsdk/environment-setup* && acap-build .  -a 'label/labels.txt' -a 'model/converted_model.tflite'
