ARG ARCH=aarch64
ARG VERSION=12.10.0
ARG UBUNTU_VERSION=24.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

#===============================================================================
# Stage 1: Extract model parameters using TensorFlow
#===============================================================================

FROM tensorflow/tensorflow:2.21.0 AS tf-stage

RUN apt-get update && apt-get install -y --no-install-recommends \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Download pre-trained YOLOv5 model
WORKDIR /opt/app/model
ARG CHIP=artpec8
ARG MODEL_BUCKET=https://acap-ml-models.s3.amazonaws.com/yolov5
RUN if [ "$CHIP" = cpu ] || [ "$CHIP" = artpec8 ]; then \
        curl -L -o model.tflite \
        $MODEL_BUCKET/yolov5n_artpec8_coco_640.tflite ; \
    elif [ "$CHIP" = artpec9 ]; then \
        curl -L -o model.tflite \
        $MODEL_BUCKET/yolov5n_artpec9_coco_640.tflite ; \
    else \
        printf "Error: '%s' is not a valid value for the CHIP variable\n", "$CHIP"; \
        exit 1; \
    fi

# Download labels for model
WORKDIR /opt/app/label
RUN curl -L -o labels.txt \
    https://github.com/google-coral/test_data/raw/master/coco_labels.txt

# Extract model parameters
WORKDIR /opt/app
COPY ./app/parameter_finder.py .
RUN python parameter_finder.py 'model/model.tflite'

#===============================================================================
# Stage 2: Build ACAP application
#===============================================================================

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

WORKDIR /opt/app
COPY ./app .

# Copy artifacts from the TensorFlow stage
COPY --from=tf-stage /opt/app/model/model.tflite model/model.tflite
COPY --from=tf-stage /opt/app/label/labels.txt label/labels.txt
COPY --from=tf-stage /opt/app/model_params.h .

ARG CHIP=artpec8
RUN cp /opt/app/manifest.json.${CHIP} /opt/app/manifest.json && \
    . /opt/axis/acapsdk/environment-setup* && acap-build . \
    -a 'label/labels.txt' \
    -a 'model/model.tflite'
