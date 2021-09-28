ARG ARCH=armv7hf
ARG REPO=axisecp
ARG SDK=acap-native-sdk
ARG UBUNTU_VERSION=20.04
ARG VERSION=latest

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Build Monkey server and library
WORKDIR /opt
COPY monkey.patch .

RUN git clone -b v1.5.6 https://github.com/monkey/monkey &&\
    cd monkey &&\
    git apply ../monkey.patch &&\
    . /opt/axis/acapsdk/environment-setup* &&\
    ./configure \
    --enable-shared \
    --malloc-libc \
    --prefix=/usr/local \
    --bindir=/usr/local/bin \
    --libdir=/usr/local/lib \
    --sysconfdir=/usr/local/packages/monkey/html \
    --datadir=/usr/local/packages/monkey/html \
    --mandir=/usr/local/man \
    --logdir=/tmp \
    --plugdir=/usr/local/packages/monkey/lib \
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
    mkdir -p $SDKTARGETSYSROOT/usr/local &&\
    cp -r /usr/local $SDKTARGETSYSROOT/usr &&\
    mkdir -p lib &&\
    cp /usr/local/lib/libmonkey.so.1.5 /usr/local/packages/monkey/lib/*.so -t lib/ &&\
    cp -r /usr/local/packages/monkey/html . &&\
    acap-build . -a reverseproxy.conf
