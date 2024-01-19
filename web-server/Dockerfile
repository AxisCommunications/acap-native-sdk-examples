ARG ARCH=armv7hf
ARG REPO=axisecp
ARG SDK=acap-native-sdk
ARG UBUNTU_VERSION=22.04
ARG VERSION=1.12

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Build Monkey server and library
WORKDIR /opt
COPY monkey.patch .

RUN git clone -b v1.5.6 https://github.com/monkey/monkey
WORKDIR /opt/monkey
RUN git apply --ignore-space-change --ignore-whitespace ../monkey.patch &&\
    . /opt/axis/acapsdk/environment-setup* &&\
    ./configure \
    --enable-shared \
    --malloc-libc \
    --prefix=/usr/local \
    --bindir=/usr/local/bin \
    --libdir=/usr/local/lib \
    --sysconfdir=/usr/local/packages/list/html \
    --datadir=/usr/local/packages/list/html \
    --mandir=/usr/local/man \
    --logdir=/tmp \
    --plugdir=/usr/local/packages/list/lib \
    --pidfile=/tmp/monkey.pid \
    --incdir=/usr/local/include/monkey \
    --systemddir=/usr/lib/systemd/system &&\
    make &&\
    make install

WORKDIR /opt/monkey/examples
COPY ./app .
RUN cp /usr/local/bin/monkey .

# Build examples
RUN . /opt/axis/acapsdk/environment-setup* &&\
    mkdir -p "$SDKTARGETSYSROOT"/usr/local &&\
    cp -r /usr/local "$SDKTARGETSYSROOT"/usr &&\
    mkdir -p lib &&\
    cp /usr/local/lib/libmonkey.so.1.5 /usr/local/packages/list/lib/*.so -t lib/ &&\
    cp -r /usr/local/packages/list/html . &&\
    acap-build .

ENV PATH="${PATH}:/opt/axis/acapsdk/sysroots/x86_64-pokysdk-linux/usr/bin"
ENTRYPOINT [ "/bin/bash" ]
