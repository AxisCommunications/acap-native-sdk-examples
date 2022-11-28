ARG ARCH=armv7hf
ARG VERSION=1.5
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
ARG ARCH=armv7hf
RUN if [ "$ARCH" = armv7hf ]; then \
        git apply --ignore-space-change --ignore-whitespace ./*.patch && \
        CXXFLAGS=' -O2 -mthumb -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a9 -fomit-frame-pointer' \
        make -j -f linux.mk CXX=arm-linux-gnueabihf-g++ CC=arm-linux-gnueabihf-gcc && \
        arm-linux-gnueabihf-strip --strip-unneeded libyuv.so* ; \
    elif [ "$ARCH" = aarch64 ]; then \
        git apply --ignore-space-change --ignore-whitespace ./*.patch && \
        make -j -f linux.mk CXX=/usr/bin/aarch64-linux-gnu-g++ CC=/usr/bin/aarch64-linux-gnu-gcc && \
        aarch64-linux-gnu-strip --strip-unneeded libyuv.so* ; \
    else \
        printf "Error: '%s' is not a valid value for the ARCH variable\n", "$ARCH"; \
        exit 1; \
    fi

# Build libjpeg-turbo
WORKDIR /opt/build
RUN apt-get update && apt-get install --no-install-recommends -y cmake && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
RUN git clone --branch 2.0.6 https://github.com/libjpeg-turbo/libjpeg-turbo.git

WORKDIR /opt/build/libjpeg-turbo/build
RUN if [ "$ARCH" = armv7hf ]; then \
        gCFLAGS=' -O2 -mthumb -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a9 -fomit-frame-pointer' \
        CC=arm-linux-gnueabihf-gcc cmake -G"Unix Makefiles" .. && \
        make -j; \
    elif [ "$ARCH" = aarch64 ]; then \
        CC=/usr/bin/aarch64-linux-gnu-gcc cmake -G"Unix Makefiles" .. && \
        make ; \
    else \
        printf "Error: '%s' is not a valid value for the ARCH variable\n", "$ARCH"; \
        exit 1; \
    fi

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

# Get pretrained models
ARG CHIP=
RUN if [ "$CHIP" = cpu ] || [ "$CHIP" = artpec8 ]; then \
        curl -L -o converted_model.tflite \
            https://github.com/google-coral/test_data/raw/master/ssd_mobilenet_v2_coco_quant_postprocess.tflite ; \
    elif [ "$CHIP" = edgetpu ]; then \
        curl -L -o converted_model.tflite \
            https://github.com/google-coral/test_data/raw/master/ssd_mobilenet_v2_coco_quant_postprocess_edgetpu.tflite ; \
    else \
        printf "Error: '%s' is not a valid value for the CHIP variable\n", "$CHIP"; \
        exit 1; \
    fi

# Download the labels
RUN mkdir -p label && \
    curl -L -o label/labels.txt https://github.com/google-coral/test_data/raw/master/coco_labels.txt

# Setup the model directory
RUN mkdir -p model && \
    cp converted_model.tflite model/converted_model.tflite

# Building the ACAP application
RUN cp /opt/app/manifest.json.${CHIP} /opt/app/manifest.json && \
    . /opt/axis/acapsdk/environment-setup* && acap-build . \
    -a 'label/labels.txt' -a 'model/converted_model.tflite'
