ARG ARCH=armv7hf
ARG VERSION=latest
ARG UBUNTU_VERSION=20.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}
# Building the ACAP application
COPY ./app /opt/app/
WORKDIR /opt/app

# Setup model and download files
RUN mkdir -p model && \
    curl -o model/mobilenet_v1_1.0_224_quant.tgz http://download.tensorflow.org/models/mobilenet_v1_2018_08_02/mobilenet_v1_1.0_224_quant.tgz && \
    tar -xvf model/mobilenet_v1_1.0_224_quant.tgz -C model && \
    rm -f model/*.tgz  model/*.pb* model/*.ckpt* model/*.txt && \
    curl -L -o model/labels_mobilenet_quant_v1_224.txt https://github.com/google-coral/edgetpu/raw/master/test_data/imagenet_labels.txt

RUN . /opt/axis/acapsdk/environment-setup* && acap-build . \
    -a 'input/veiltail-11457_640_RGB_224x224.bin' \
    -a 'model/mobilenet_v1_1.0_224_quant.tflite' \
    -a 'model/labels_mobilenet_quant_v1_224.txt'