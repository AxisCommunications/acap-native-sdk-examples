FROM tensorflow/tensorflow:latest-gpu-py3
ENV TF_FORCE_GPU_ALLOW_GROWTH true

# Get docker to be able to build ACAP inside this container
ARG DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN rm /etc/apt/sources.list.d/cuda.list && rm /etc/apt/sources.list.d/nvidia-ml.list
RUN apt-get update && apt-get install -y --no-install-recommends \
   curl && \
   apt-get clean && \
   rm -rf /var/lib/apt/lists/* && \
   curl --limit-rate 1G -L -O https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/cuda-keyring_1.0-1_all.deb && \
   dpkg -i cuda-keyring_1.0-1_all.deb

RUN apt-get update && apt-get install -y --no-install-recommends \
   apt-transport-https \
   ca-certificates \
   software-properties-common && \
   # Install Docker
   curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add - && \
   add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu focal stable" && \
   apt-get update && apt-get install -y --no-install-recommends docker-ce && \
   # Edge TPU Compiler installation
   curl https://packages.cloud.google.com/apt/doc/apt-key.gpg | apt-key add - && \
   echo "deb https://packages.cloud.google.com/apt coral-edgetpu-stable main" | tee /etc/apt/sources.list.d/coral-edgetpu.list && \
   apt-get update && apt-get install -y --no-install-recommends edgetpu-compiler && \
   apt-get clean && \
   rm -rf /var/lib/apt/lists/*

# Create directories and get dataset
WORKDIR /env
RUN mkdir -p /env/app /env/data/images /env/data/annotations /env/models && \
   curl --limit-rate 1G -o /env/data/images/val2017.zip \
   http://images.cocodataset.org/zips/val2017.zip && \
   unzip -q /env/data/images/val2017.zip -d /env/data/images/ && \
   rm /env/data/images/val2017.zip && \
   curl --limit-rate 1G -o /env/data/annotations_trainval2017.zip \
   http://images.cocodataset.org/annotations/annotations_trainval2017.zip && \
   unzip -q /env/data/annotations_trainval2017.zip -d /env/data/ && \
   rm /env/data/annotations_trainval2017.zip

# Get pretrained models
COPY models.armv7hf.edgetpu.sha512 /env/models.armv7hf.edgetpu.sha512
RUN curl -o /env/models.armv7hf.edgetpu.zip \
   https://acap-artifacts.s3.eu-north-1.amazonaws.com/models/models.armv7hf.edgetpu.zip && \
   sha512sum -c /env/models.armv7hf.edgetpu.sha512 && \
   rm /env/models.armv7hf.edgetpu.sha512 && \
   unzip -q /env/models.armv7hf.edgetpu.zip -d /env/ && rm /env/models.armv7hf.edgetpu.zip && \
   cp /env/models/converted_model_edgetpu.tflite /env/app/

# Copy the rest of the local files
COPY env/ /env/

# Get specific python packages
RUN pip3 install --no-cache-dir Pillow==8.0.0 tensorflow==2.3.0

ENV NVIDIA_VISIBLE_DEVICES all
ENV NVIDIA_DRIVER_CAPABILITIES all

