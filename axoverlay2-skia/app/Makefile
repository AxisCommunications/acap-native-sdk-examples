# This makefile shall be run inside the Axis ACAP SDK environment.
#
# The Skia graphics library is downloaded and built by the Dockerfile. The
# location of the Skia headers and the prebuilt static library are provided via
# the SKIA_DIR and SKIA_LIB variables (set as environment variables by the
# Dockerfile). This makefile only compiles and links the application against
# that prebuilt library.

# Default to a skia/ checkout next to the application sources, but allow the
# Dockerfile to override these with absolute paths so that Skia does not need to
# live inside the packaged application directory.
SKIA_DIR ?= skia
SKIA_LIB ?= build_skia/libskia.a

CXXFLAGS += \
    -Wall \
    -Wextra \
    -Wformat=2 \
    -Wpointer-arith \
    -Wdisabled-optimization \
    -Wno-unused-parameter \
    -Werror \
    -std=c++20

prog := $(shell jq -r .acapPackageConf.setup.appName <manifest.json)

sources := \
    axoverlay2_skia.cc

depfiles := $(sources:.cc=.d)

deps := glib-2.0 glesv2 egl vdostream axoverlay2

dep_cflags := $(shell pkg-config --cflags $(deps))
dep_ldlibs := $(shell pkg-config --libs-only-l $(deps))

-include $(depfiles)

all: $(prog)

$(prog): $(sources) $(SKIA_LIB)
	$(CXX) $(dep_cflags) -I$(SKIA_DIR) $(CXXFLAGS) -MMD -MP $(LDFLAGS) $(sources) $(dep_ldlibs) $(SKIA_LIB) -o $@
	mkdir -p debug
	cp $@ debug/$@
	$(STRIP) $@

.PHONY: clean
clean:
	rm -rf $(prog) build_skia *.eap *_LICENSE.txt package.conf* param.conf tmp* debug *.d
