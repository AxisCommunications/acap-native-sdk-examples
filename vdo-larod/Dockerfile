ARG ARCH=armv7hf
ARG VERSION=1.0
ARG UBUNTU_VERSION=20.04
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
RUN git apply ./*.patch && \
    CXXFLAGS=' -O2 -mthumb -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a9 -fomit-frame-pointer' \
    make -j -f linux.mk CXX=arm-linux-gnueabihf-g++ CC=arm-linux-gnueabihf-gcc && \
    arm-linux-gnueabihf-strip --strip-unneeded libyuv.so*

# Copy the library to application folder
WORKDIR /opt/app
COPY ./app /opt/app/
ARG BUILDDIR=/opt/build/libyuv
RUN mkdir -p lib include && \
    cp ${BUILDDIR}/libyuv.so* lib/ && \
    cp -a ${BUILDDIR}/include/. include && \
    ln -s libyuv.so.1 lib/libyuv.so && \
    ln -s libyuv.so.1 lib/libyuv.so.1.0

# Download models and labels
RUN mkdir -p model && \
    curl -o model/mobilenet_v2_1.0_224_quant.tgz \
    http://download.tensorflow.org/models/tflite_11_05_08/mobilenet_v2_1.0_224_quant.tgz && \
    tar -xvf model/mobilenet_v2_1.0_224_quant.tgz -C model && \
    rm -f model/*.tgz model/*.pb* model/*.ckpt* model/*.meta model/*.txt && \
    curl -L -o model/mobilenet_v2_1.0_224_quant_edgetpu.tflite \
    https://github.com/google-coral/edgetpu/raw/master/test_data/mobilenet_v2_1.0_224_quant_edgetpu.tflite

RUN mkdir -p label && \
    curl -L -o label/imagenet_labels.txt \
    https://github.com/google-coral/edgetpu/raw/master/test_data/imagenet_labels.txt

ARG CHIP=
# Building the ACAP application
RUN if [ "$CHIP" = cpu ]; then \
    . /opt/axis/acapsdk/environment-setup* && acap-build . \
    -a 'label/imagenet_labels.txt' -a 'model/mobilenet_v2_1.0_224_quant.tflite'; \
    elif [ "$CHIP" = edgetpu ]; then \
    . /opt/axis/acapsdk/environment-setup* && acap-build . \
    -a 'label/imagenet_labels.txt' -a 'model/mobilenet_v2_1.0_224_quant_edgetpu.tflite'; \
    else \
    printf "Error: '%s' is not a valid value for the CHIP variable\n", "$CHIP"; \
    exit 1; \
    fi
