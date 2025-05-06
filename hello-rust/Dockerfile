ARG ARCH=armv7hf
ARG VERSION=12.3.0
ARG UBUNTU_VERSION=24.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN apt-get update \
 && apt-get install --no-install-recommends -y \
      build-essential \
      curl \
 && rm -rf /var/lib/apt/lists/* \
 && curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y

ENV PATH="/root/.cargo/bin:${PATH}"

RUN . /opt/axis/acapsdk/environment-setup* \
 && if [ "$OECORE_TARGET_ARCH" = "armv7hf" ]; then \
        rustup target add armv7-unknown-linux-gnueabihf && \
        echo "export CARGO_BUILD_TARGET=armv7-unknown-linux-gnueabihf" >> /opt/axis/acapsdk/rust-environment-setup && \
        echo "export CARGO_TARGET_THUMBV7NEON_UNKNOWN_LINUX_GNUEABIHF_LINKER=${CC%% *}" >> /opt/axis/acapsdk/rust-environment-setup; \
    elif [ "$OECORE_TARGET_ARCH" = "aarch64" ]; then \
        rustup target add aarch64-unknown-linux-gnu && \
        echo "export CARGO_BUILD_TARGET=aarch64-unknown-linux-gnu" >> /opt/axis/acapsdk/rust-environment-setup && \
        echo "export CARGO_TARGET_AARCH64_UNKNOWN_LINUX_GNU_LINKER=${CC%% *}" >> /opt/axis/acapsdk/rust-environment-setup; \
    fi \
 && echo "export CARGO_TARGET_RUSTFLAGS='-C link-args=--sysroot=${OECORE_TARGET_SYSROOT}'" >> /opt/axis/acapsdk/rust-environment-setup

COPY ./app /opt/app/
WORKDIR /opt/app
# hadolint ignore=SC1091
RUN . /opt/axis/acapsdk/environment-setup* \
 && . /opt/axis/acapsdk/rust-environment-setup \
 && acap-build ./ \
 && rm -r ./target/
