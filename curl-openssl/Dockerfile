ARG ARCH=armv7hf
ARG VERSION=1.14_rc3
ARG UBUNTU_VERSION=22.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION} as sdk

# Set arguments used in build of both libraries
ARG ARCH=armv7hf
ARG PEM_CERT_FILE=cacert.pem

# (Optional) If the network has a proxy
ARG APP_PROXY
ENV APP_PROXY ${APP_PROXY}

# (Optional) Get more verbose logging when running the application
ARG APP_DEBUG
ENV APP_DEBUG ${APP_DEBUG}

#-------------------------------------------------------------------------------
# Prepare build environment
#-------------------------------------------------------------------------------

WORKDIR /opt/app
COPY ./app .

#-------------------------------------------------------------------------------
# Get CA certificate for the web server we want to transfer data from
#-------------------------------------------------------------------------------

# Use the 'openssl' tool from the Ubuntu container to get a CA certificate from
# the web server of interest. Why not use the compiled 'openssl' binary to do
# this? It's cross compiled and will not run on a standard desktop.
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
# APP_PROXY_SETTING need to be split
# hadolint ignore=SC2086
RUN APP_PROXY_SETTING= ; \
    [ -z "$APP_PROXY" ] || APP_PROXY_SETTING="-proxy $APP_PROXY" ; \
    echo quit | openssl s_client \
    -showcerts \
    -servername www.example.com \
    -connect www.example.com:443 \
    $APP_PROXY_SETTING \
    > ${PEM_CERT_FILE}

#-------------------------------------------------------------------------------
# Finally build the ACAP application
#-------------------------------------------------------------------------------

RUN . /opt/axis/acapsdk/environment-setup* && \
    acap-build . -a ${PEM_CERT_FILE}
