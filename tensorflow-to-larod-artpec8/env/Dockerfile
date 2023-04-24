ARG ARCH=aarch64
ARG VERSION=1.8
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
RUN git apply ./*.patch && \
   make -j -f linux.mk CXX=/usr/bin/aarch64-linux-gnu-g++ CC=/usr/bin/aarch64-linux-gnu-gcc && \
   aarch64-linux-gnu-strip --strip-unneeded libyuv.so*

# Copy the library to application folder
WORKDIR /opt/app
COPY ./app /opt/app/
ARG BUILDDIR=/opt/build/libyuv
RUN mkdir -p lib include && \
   cp ${BUILDDIR}/libyuv.so* lib/ && \
   cp -a ${BUILDDIR}/include/. include && \
   ln -s libyuv.so.1 lib/libyuv.so && \
   ln -s libyuv.so.1 lib/libyuv.so.1.0

# Setup the model directory
RUN mkdir -p model && cp converted_model.tflite model/converted_model.tflite

RUN . /opt/axis/acapsdk/environment-setup* && acap-build . -a 'model/converted_model.tflite'