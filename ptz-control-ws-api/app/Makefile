PROG1 = $(shell jq -r '.acapPackageConf.setup.appName' manifest.json)
OBJS1 = main.c \
        listener.c \
        subscriber.c \
        utils.c \
        websocket_client.c
PROGS = $(PROG1)
DEBUG_DIR = debug

PKGS = glib-2.0 jansson libcurl device-data-hub-client-c

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))
LDLIBS += -lsystemd

CFLAGS += -Wall \
          -Wextra \
          -Wstrict-prototypes \
          -Wmissing-prototypes \
          -Wformat=2 \
          -Wpointer-arith \
          -Wdisabled-optimization \
          -Wfloat-equal \
          -Wbad-function-cast \
          -Winline \
          -W \
          -Werror

all: $(PROGS)

$(PROG1): $(OBJS1)
	# Create debug directory and build binary with debug symbols
	install -d $(DEBUG_DIR)

	$(CC) $^ $(CFLAGS) $(LIBS) $(LDFLAGS) $(LDLIBS) -o $(DEBUG_DIR)/$@
	cp $(DEBUG_DIR)/$@ .
	$(STRIP) $@

clean:
	rm -rf $(PROGS) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp* $(DEBUG_DIR)
