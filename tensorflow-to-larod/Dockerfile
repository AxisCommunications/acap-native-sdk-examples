FROM tensorflow/tensorflow:latest-gpu-py3

ARG http_proxy
ARG https_proxy

ENV HTTPS_PROXY ${https_proxy}
ENV HTTP_PROXY ${http_proxy}
ENV https_proxy ${https_proxy}
ENV http_proxy ${http_proxy}
ENV TF_FORCE_GPU_ALLOW_GROWTH true

# Get docker to be able to build ACAP inside this container
RUN apt-get update
RUN apt-get install -y apt-transport-https \
                       ca-certificates \
                       curl \
                       software-properties-common
RUN curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add -
RUN add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu bionic stable"
RUN apt-get update && apt-get install docker-ce -y

# Create skeleton
RUN mkdir -p /env/app

# Get dataset
RUN apt-get install -y wget
RUN mkdir -p /env/data/images && mkdir -p /env/data/annotations
RUN curl -o /env/data/images/val2017.zip http://images.cocodataset.org/zips/val2017.zip && \
    unzip -q /env/data/images/val2017.zip -d /env/data/images/ && \
    rm /env/data/images/val2017.zip
RUN curl -o /env/data/annotations_trainval2017.zip http://images.cocodataset.org/annotations/annotations_trainval2017.zip && \
    unzip -q /env/data/annotations_trainval2017.zip -d /env/data/ && \
    rm /env/data/annotations_trainval2017.zip

# Get pretrained models
WORKDIR /env
COPY env/models.sha512 /env/models.sha512
RUN mkdir -p /env/models
ADD https://www.axis.com/techsup/developer_doc/acap3/examples/models.zip /env/
RUN sha512sum -c /env/models.sha512
RUN rm /env/models.sha512
RUN unzip -q /env/models.zip -d /env/ && rm /env/models.zip
RUN cp /env/models/converted_model_edgetpu.tflite /env/app/

# Edge TPU Compiler installation
RUN curl https://packages.cloud.google.com/apt/doc/apt-key.gpg | apt-key add -
RUN echo "deb https://packages.cloud.google.com/apt coral-edgetpu-stable main" | tee /etc/apt/sources.list.d/coral-edgetpu.list
RUN apt-get update && apt-get install edgetpu-compiler -y

ARG DEBIAN_FRONTEND=noninteractive

# Get specific python packages
RUN pip3 install Pillow tensorflow==2.3.0

ENV NVIDIA_VISIBLE_DEVICES all
ENV NVIDIA_DRIVER_CAPABILITIES all

# Copy the rest of the local files
ADD env/ /env/
