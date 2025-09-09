PROG1	= $(shell jq -r '.acapPackageConf.setup.appName' manifest.json)
OBJS1	= $(PROG1).c argparse.c imgprovider.c imgutils.c postprocessing.c
PROGS	= $(PROG1)
LIBDIR = lib
LIBJPEG_TURBO = /opt/build/libjpeg-turbo/build
DEBUG_DIR = debug

PKGS = gio-2.0 gio-unix-2.0 liblarod vdostream

CFLAGS += -I$(LIBJPEG_TURBO)/include -DLAROD_API_VERSION_3
LDLIBS  += -ljpeg -lm
LDFLAGS += -L./$(LIBDIR) -Wl,-rpath,'$$ORIGIN/$(LIBDIR)'

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

CFLAGS += -Wall \
          -Wextra \
          -Wformat=2 \
          -Wpointer-arith \
          -Wbad-function-cast \
          -Wstrict-prototypes \
          -Wmissing-prototypes \
          -Winline \
          -Wdisabled-optimization \
          -Wfloat-equal \
          -W \
          -Werror

all:	$(PROGS)

$(PROG1): $(OBJS1)
	install -d $(DEBUG_DIR)
	mkdir -p $(LIBDIR)
	cp $(LIBJPEG_TURBO)/lib/*.so* $(LIBDIR)/
	$(CC) $^ $(CFLAGS) $(LIBS) $(LDFLAGS) -lm $(LDLIBS) -o $(DEBUG_DIR)/$@
	cp $(DEBUG_DIR)/$@ .
	$(STRIP) $@

clean:
	rm -rf $(PROGS) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp* $(LIBDIR) manifest.json $(DEBUG_DIR)
