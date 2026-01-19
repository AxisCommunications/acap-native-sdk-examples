PROG1	= $(shell jq -r '.acapPackageConf.setup.appName' manifest.json)
OBJS1	= $(PROG1).cpp
PROGS	= $(PROG1)
DEBUG_DIR = debug

PKGS = nexus-client-cpp

CXXFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

CXXFLAGS += -Wall \
			-Wextra \
			-Wpedantic \
			-Wfloat-equal \
			-Winline \
			-Wstack-protector \
			-Wpointer-arith \
			-Werror \
			-std=c++23

all: $(PROGS)

$(PROG1): $(OBJS1)
	# Create debug directory and build binary with debug symbols
	install -d $(DEBUG_DIR)

	$(CXX) $^ $(CXXFLAGS) $(LIBS) $(LDFLAGS) $(LDLIBS) -o $(DEBUG_DIR)/$@
	cp $(DEBUG_DIR)/$@ .
	$(STRIP) $@

clean:
	rm -rf $(PROGS) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp* $(DEBUG_DIR)
