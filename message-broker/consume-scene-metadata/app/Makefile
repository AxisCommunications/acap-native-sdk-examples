PROG1	= $(shell jq -r '.acapPackageConf.setup.appName' manifest.json)
OBJS1	= $(PROG1).c
PROGS	= $(PROG1)
DEBUG_DIR = debug

PKGS = mdb

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

CFLAGS += \
					-Wall \
					-Wextra \
					-Wpedantic \
					-Wmissing-prototypes \
					-Wstrict-prototypes \
					-Wfloat-equal \
					-Winline \
					-Wbad-function-cast \
					-Wstack-protector \
					-Wpointer-arith \
					-Werror

CFLAGS_DEBUG = -g3 -O0

all: $(PROGS)

$(PROG1): $(OBJS1)
	# Create debug directory and build binary with debug symbols
	install -d $(DEBUG_DIR)

	$(CC) $^ $(CFLAGS) $(CFLAGS_DEBUG) $(LIBS) $(LDFLAGS) $(LDLIBS) -o $(DEBUG_DIR)/$@
	cp $(DEBUG_DIR)/$@ .
	$(STRIP) $@

clean:
	rm -rf $(PROGS) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp* $(DEBUG_DIR)
