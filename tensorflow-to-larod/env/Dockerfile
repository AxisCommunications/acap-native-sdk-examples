ARG http_proxy
ARG https_proxy
ARG ARCH=armv7hf
ARG VERSION=latest
ARG UBUNTU_VERSION=20.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Build libyuv
WORKDIR /opt/build
ARG libyuv_version=5b6042fa0d211ebbd8b477c7f3855977c7973048
# TODO: Investigate why server certs can't be verified
RUN GIT_SSL_NO_VERIFY=1 git clone -n https://chromium.googlesource.com/libyuv/libyuv && \
  cd libyuv && git checkout ${libyuv_version}

WORKDIR /opt/build/libyuv
COPY yuv/*.patch /opt/build/libyuv
RUN git apply *.patch && \
  CXXFLAGS=' -O2 -mthumb -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a9 -fomit-frame-pointer' \
  make -f linux.mk CXX=arm-linux-gnueabihf-g++ CC=arm-linux-gnueabihf-gcc && \
  arm-linux-gnueabihf-strip --strip-unneeded libyuv.so*

# Copy the library to application folder
ARG BUILDDIR=/opt/build/libyuv
WORKDIR /opt/app
COPY ./app /opt/app/
RUN mkdir -p lib include && \
  cp ${BUILDDIR}/libyuv.so* lib/ && \
  cp -a ${BUILDDIR}/include/. include && \
  cd lib/ && \
  ln -s libyuv.so.1 libyuv.so && \
  ln -s libyuv.so.1 libyuv.so.1.0

# Building the ACAP application
RUN . /opt/axis/acapsdk/environment-setup* && create-package.sh
