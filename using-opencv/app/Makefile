PROGS = opencv_app
OBJECTS = $(wildcard *.cpp)
DEBUG_DIR = debug

PKGS = gio-2.0 gio-unix-2.0 vdostream

CXXFLAGS += -Os -pipe -std=c++11
CXXFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags-only-I $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

CXXFLAGS += -I$(SDKTARGETSYSROOT)/usr/include/opencv4
LDFLAGS = -L./lib -Wl,--no-as-needed,-rpath,'$$ORIGIN/lib'
LDLIBS += -lm -lopencv_video -lopencv_imgproc -lopencv_core

.PHONY: all clean

all: $(PROGS)

$(PROGS): $(OBJECTS)
	install -d $(DEBUG_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS) $^ -o $(DEBUG_DIR)/$@ ; \
	cp $(DEBUG_DIR)/$@ .
	$(STRIP) --strip-unneeded $@

clean:
	rm -rf $(PROGS) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp* $(DEBUG_DIR)
