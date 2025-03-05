ARG ARCH=armv7hf
ARG VERSION=12.3.0
ARG UBUNTU_VERSION=24.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

RUN apt-get update && apt-get install -y --no-install-recommends \
    unzip \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/app/model
ARG CHIP

# Download the pretrained model
ARG MODEL_BUCKET=https://acap-ml-models.s3.amazonaws.com/tensorflow_to_larod_resnet
RUN if [ "$CHIP" = artpec8 ] || [ "$CHIP" = artpec9 ] || [ "$CHIP" = cpu ]  ; then \
        curl -o model.tflite $MODEL_BUCKET/custom_resnet_artpec8_car_human_480x270.tflite ; \
    elif [ "$CHIP" = edgetpu ]; then \
        curl -o model.tflite $MODEL_BUCKET/custom_resnet_edgetpu_car_human_256.tflite ; \
    elif [ "$CHIP" = cv25 ]; then \
        curl -o model.bin $MODEL_BUCKET/custom_resnet_cv25_car_human_256.bin ; \
    else \
        printf "Error: '%s' is not a valid value for the CHIP variable\n", "$CHIP"; \
        exit 1; \
    fi

# Copy the library to application folder
WORKDIR /opt/app
COPY ./app .

# Build the ACAP application
RUN cp /opt/app/manifest.json.${CHIP} /opt/app/manifest.json && \
    . /opt/axis/acapsdk/environment-setup* && \
    if [ "$CHIP" = artpec8 ] || [ "$CHIP" = artpec9 ] || [ "$CHIP" = cpu ] || [ "$CHIP" = edgetpu ]; then \
        acap-build . -a 'model/model.tflite'; \
    elif [ "$CHIP" = cv25 ]; then \
        acap-build . -a 'model/model.bin'; \
    else \
        printf "Error: '%s' is not a valid value for the CHIP variable\n", "$CHIP"; \
        exit 1; \
fi

