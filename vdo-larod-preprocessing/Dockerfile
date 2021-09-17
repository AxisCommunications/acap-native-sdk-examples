# Put a proxy URL in the docker_proxy environment variable if needed
ARG DOCKER_PROXY
ARG http_proxy=$DOCKER_PROXY
ARG https_proxy=$DOCKER_PROXY

ARG ARCH=armv7hf
ARG VERSION=latest
ARG UBUNTU_VERSION=20.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk
FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Download model
WORKDIR /opt/app/model
RUN curl -o mobilenet_v2_1.0_224_quant.tgz http://download.tensorflow.org/models/tflite_11_05_08/mobilenet_v2_1.0_224_quant.tgz &&\
	tar -xvf mobilenet_v2_1.0_224_quant.tgz &&\
	curl -L -o mobilenet_v2_1.0_224_quant_edgetpu.tflite https://github.com/google-coral/edgetpu/raw/master/test_data/mobilenet_v2_1.0_224_quant_edgetpu.tflite &&\
	curl -L -o imagenet_labels.txt https://github.com/google-coral/edgetpu/raw/master/test_data/imagenet_labels.txt

# Copy the library to application folder
WORKDIR /opt/app
COPY ./app .

# Building the ACAP application
RUN . /opt/axis/acapsdk/environment-setup* && create-package.sh
