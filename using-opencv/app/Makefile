# Application Name
TARGET := opencv_app

OBJECTS := example.cpp

PKGS = gio-2.0 vdostream gio-unix-2.0

LDFLAGS += -L$(SDKTARGETSYSROOT)/usr/lib
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))
CXXFLAGS += -Os -pipe -std=c++11
CXXFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags-only-I $(PKGS))
CXXFLAGS += -I/target-root/usr/include/opencv4

SDK_TARGET_LIBS=$(TARGET_ROOT)/usr/lib

SHLIB_DIR = ./lib

LDFLAGS = -L$(SHLIB_DIR) -Wl,--no-as-needed,-rpath,'$$ORIGIN/lib'
SHLIBS += -lm -lopencv_video -lopencv_imgproc -lopencv_core -lpthread

LIBCORE_FILES = $(wildcard $(SDK_TARGET_LIBS)/libopencv_core.so*)
LIBPROC_FILES = $(wildcard $(SDK_TARGET_LIBS)/libopencv_imgproc.so*)
LIBVIDEO_FILES = $(wildcard $(SDK_TARGET_LIBS)/libopencv_video.so*)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS) libscopy
	$(CXX) $< $(CXXFLAGS) $(LDFLAGS) $(SHLIBS) $(LDLIBS) imgprovider.cpp -o $@ && $(STRIP) --strip-unneeded $@

libscopy:
	mkdir -p $(SHLIB_DIR)
	cp $(LIBCORE_FILES) $(SHLIB_DIR)
	cp $(LIBPROC_FILES) $(SHLIB_DIR)
	cp $(LIBVIDEO_FILES) $(SHLIB_DIR)

clean:
	$(RM) *.o $(TARGET)
