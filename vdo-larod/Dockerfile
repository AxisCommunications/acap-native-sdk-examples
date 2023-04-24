ARG ARCH=armv7hf
ARG VERSION=1.8
ARG UBUNTU_VERSION=22.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

RUN apt-get update && apt-get install -y --no-install-recommends \
    unzip \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/app/
ARG CHIP

# Download the pretrained model
ARG MODEL_BUCKET=https://acap-artifacts.s3.eu-north-1.amazonaws.com/models
RUN if [ "$CHIP" = artpec8 ]; then \
        curl -o model.zip $MODEL_BUCKET/models.aarch64.artpec8.zip ; \
    elif [ "$CHIP" = cpu ]; then \
        curl -o model.zip $MODEL_BUCKET/models.aarch64.artpec8.zip ; \
    elif [ "$CHIP" = edgetpu ]; then \
        curl -o model.zip $MODEL_BUCKET/models.armv7hf.edgetpu.zip ; \    
    elif [ "$CHIP" = cv25 ]; then \
        curl -o model.zip $MODEL_BUCKET/tensorflow_to_larod_models.cv25.zip ; \        
    else \
        printf "Error: '%s' is not a valid value for the CHIP variable\n", "$CHIP"; \
        exit 1; \
    fi && \
    unzip -q model.zip && rm -f model.zip

# Copy the library to application folder
WORKDIR /opt/app
COPY ./app .

# Build the ACAP application
RUN cp /opt/app/manifest.json.${CHIP} /opt/app/manifest.json && \
    . /opt/axis/acapsdk/environment-setup* && \
    if [ "$CHIP" = artpec8 ] || [ "$CHIP" = cpu ]; then \
        acap-build . -a 'models/converted_model.tflite'; \
    elif [ "$CHIP" = edgetpu ]; then \
        acap-build . -a 'models/converted_model_edgetpu.tflite'; \
    elif [ "$CHIP" = cv25 ]; then \
        acap-build . -a 'models/car_human_model_cavalry.bin'; \
    else \
        printf "Error: '%s' is not a valid value for the CHIP variable\n", "$CHIP"; \
        exit 1; \
fi

