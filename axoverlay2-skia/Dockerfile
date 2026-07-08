ARG ARCH=aarch64
ARG VERSION=12.11.0-rc.1
ARG UBUNTU_VERSION=24.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

ARG BUILD_DIR=/opt/build
ARG SKIA_VERSION=chrome/m137

#-------------------------------------------------------------------------------
# Prepare build environment
#-------------------------------------------------------------------------------

# Install build dependencies for compiling Skia. Skia is configured with GN
# (generate-ninja) and built with Ninja.
RUN DEBIAN_FRONTEND=noninteractive \
    apt-get update && apt-get install -y --no-install-recommends \
    generate-ninja ninja-build git && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

#-------------------------------------------------------------------------------
# Download and build the Skia graphics library
#
# These layers only depend on SKIA_VERSION, so they are cached and reused across
# rebuilds as long as the Skia version is unchanged. Editing the application
# source below does not trigger a Skia rebuild.
#-------------------------------------------------------------------------------

WORKDIR ${BUILD_DIR}/skia

# Clone Skia, fetch its third-party dependencies, configure with GN and build
# with Ninja in a single layer.
#
# git-sync-deps fetches Skia's third-party dependencies. On a slow or
# unreliable network this step may fail. git-sync-deps starts one thread per
# dependency with no concurrency limit, which can overload the connection or
# trigger server-side rate limiting (HTTP 429). If that happens, serialize the
# fetches by joining each worker thread right after starting it, for example
# by prepending:
#   sed -i 's/^\( *\)thread.start()/\1thread.start()\n\1thread.join()/' tools/git-sync-deps
# git-sync-deps is idempotent and skips already fetched dependencies, so
# simply re-running it also lets a partial download converge.
#
# Minimal Skia GN configuration with most modules disabled.
#
#   cc, cxx:            Force GN to use the SDK-provided cross compilers.
#   extra_cflags:       Disable NEON optimisations in Skia's bundled libpng.
#                       These are not needed here and cause linking errors.
#   target_cpu:         Prevent GN from guessing the target CPU and injecting
#                       the wrong -march option.
#   is_component_build: Produce a static Skia library.
RUN git clone https://skia.googlesource.com/skia.git . \
        --branch ${SKIA_VERSION} --depth 1 && \
    python3 tools/git-sync-deps && \
    . /opt/axis/acapsdk/environment-setup* && \
    gn gen --root=. --args=" \
        cc=\"$CC\" \
        cxx=\"$CXX\" \
        extra_cflags=[\"-DPNG_ARM_NEON_OPT=0\"] \
        target_cpu=\"dont_worry_about_it\" \
        is_official_build=true \
        is_component_build=false \
        is_debug=false \
        skia_enable_ganesh=true \
        skia_enable_pdf=false \
        skia_enable_skshaper=false \
        skia_enable_skunicode=false \
        skia_enable_svg=false \
        skia_enable_tools=false \
        skia_pdf_subset_harfbuzz=false \
        skia_use_angle=false \
        skia_use_egl=true \
        skia_use_expat=false \
        skia_use_fontconfig=false \
        skia_use_freetype=false \
        skia_use_gl=true \
        skia_use_harfbuzz=false \
        skia_use_icu=false \
        skia_use_libjpeg_turbo_decode=false \
        skia_use_libjpeg_turbo_encode=false \
        skia_use_lua=false \
        skia_use_libpng_decode=false \
        skia_use_libpng_encode=true \
        skia_use_libwebp_decode=false \
        skia_use_libwebp_encode=false \
        skia_use_system_libpng=false \
        skia_use_system_zlib=false \
        skia_use_wuffs=false \
        skia_use_x11=false \
        skia_use_xps=false \
        skia_use_zlib=false \
        " build_skia && \
    ninja -C build_skia

#-------------------------------------------------------------------------------
# Build the ACAP application
#-------------------------------------------------------------------------------

WORKDIR /opt/app
COPY ./app .

# Point the application Makefile at the prebuilt Skia headers and static
# library. Skia is statically linked, so it is only needed at build time and is
# kept outside the application directory to avoid bloating the package.
ENV SKIA_DIR=${BUILD_DIR}/skia
ENV SKIA_LIB=${BUILD_DIR}/skia/build_skia/libskia.a

RUN . /opt/axis/acapsdk/environment-setup* && acap-build .
