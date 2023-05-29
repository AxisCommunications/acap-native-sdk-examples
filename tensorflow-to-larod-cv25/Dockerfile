# --- Base Stage ---
FROM tensorflow/tensorflow:latest-gpu-py3
ARG DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

WORKDIR /env
RUN mkdir -p /env/app /env/models /env/training/models

# Install 
RUN rm /etc/apt/sources.list.d/cuda.list && \
    rm /etc/apt/sources.list.d/nvidia-ml.list && \
    apt-get update && apt-get install -y --no-install-recommends \
    gnupg && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Install cuda keyring
ARG CUDA_KEYRING_URL=https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/cuda-keyring_1.0-1_all.deb
RUN curl --limit-rate 1G -L -O $CUDA_KEYRING_URL && \
    dpkg -i cuda-keyring_1.0-1_all.deb && \
    rm cuda-keyring_1.0-1_all.deb

WORKDIR /env/data/images
ARG MODEL_URL=http://images.cocodataset.org/zips/val2017.zip
RUN curl --limit-rate 1G -o val2017.zip $MODEL_URL && \
    unzip -q val2017.zip && \
    rm val2017.zip

WORKDIR /env/data/
ARG ANNOTATION_URL=http://images.cocodataset.org/annotations/annotations_trainval2017.zip
RUN curl --limit-rate 1G -o annotations_trainval2017.zip $ANNOTATION_URL && \
    unzip -q annotations_trainval2017.zip && \
    rm annotations_trainval2017.zip

# Get pretrained models
WORKDIR /env
ARG MODEL_URL=https://acap-ml-model-storage.s3.amazonaws.com/tensorflow_to_larod_models.cv25.zip
RUN curl -o models.zip $MODEL_URL && \
   unzip -q models.zip  && \
   rm models.zip && \
   cp /env/models/car_human_model_cavalry.bin /env/app/

RUN pip3 install --no-cache-dir Pillow==8.0.0

COPY env/ /env/

# --- Training Stage ---

WORKDIR /env
ARG TRAIN_EPOCHS
RUN python training/train.py \
      -i data/images/val2017/ \
      -a data/annotations/instances_val2017.json \
      -e ${TRAIN_EPOCHS} && \
    python convert_model.py \
      -i models/saved_model \
      -d data/images/val2017 \
      -o training/models/converted_model.tflite
