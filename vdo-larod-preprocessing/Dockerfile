ARG ARCH=armv7hf
ARG VERSION=1.4
ARG UBUNTU_VERSION=22.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Download model
WORKDIR /opt/app/model
RUN curl -o mobilenet_v2_1.0_224_quant.tgz http://download.tensorflow.org/models/tflite_11_05_08/mobilenet_v2_1.0_224_quant.tgz && \
    tar -xvf mobilenet_v2_1.0_224_quant.tgz && \
    curl -L -o mobilenet_v2_1.0_224_quant_edgetpu.tflite https://github.com/google-coral/edgetpu/raw/master/test_data/mobilenet_v2_1.0_224_quant_edgetpu.tflite && \
    curl -L -o imagenet_labels.txt https://github.com/google-coral/edgetpu/raw/master/test_data/imagenet_labels.txt && \
    curl -L -o mobilenet_v2_cavalry.bin https://acap-ml-model-storage.s3.amazonaws.com/mobilenetv2_cavalry.bin

# Copy the library to application folder
WORKDIR /opt/app
COPY ./app .

ARG CHIP=
# Building the ACAP application
RUN if [ "$CHIP" = cpu ] || [ "$CHIP" = artpec8 ]; then \
        cp /opt/app/manifest.json.${CHIP} /opt/app/manifest.json && \
        . /opt/axis/acapsdk/environment-setup* && acap-build . \
        -a 'model/mobilenet_v2_1.0_224_quant.tflite' -a 'model/imagenet_labels.txt' ; \
    elif [ "$CHIP" = edgetpu ]; then \
        cp /opt/app/manifest.json.${CHIP} /opt/app/manifest.json && \
        . /opt/axis/acapsdk/environment-setup* && acap-build . \
        -a 'model/mobilenet_v2_1.0_224_quant_edgetpu.tflite' -a 'model/imagenet_labels.txt' ; \
    elif [ "$CHIP" = cv25 ]; then \
        cp /opt/app/manifest.json.${CHIP} /opt/app/manifest.json && \
        . /opt/axis/acapsdk/environment-setup* && acap-build . \
        -a 'model/mobilenet_v2_cavalry.bin' -a 'model/imagenet_labels.txt'; \
    else \
        printf "Error: '%s' is not a valid value for the CHIP variable\n", "$CHIP"; \
        exit 1; \
    fi
