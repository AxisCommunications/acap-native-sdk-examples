PROG1	= axoverlay
OBJS1	= axoverlay.c

PROGS	= $(PROG1)

PKGS = gio-2.0 glib-2.0 cairo
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))
LDFLAGS  += -s -laxoverlay

CFLAGS-y  += -W -Wformat=2 -Wpointer-arith -Wbad-function-cast -Wstrict-prototypes -Wmissing-prototypes -Winline -Wdisabled-optimization -Wfloat-equal -Wall -Werror

all:	$(PROGS)

$(PROG1): $(OBJS1)
	$(CC) $^ $(CFLAGS) $(LIBS) $(LDFLAGS) -lm $(LDLIBS) -o $@
	$(STRIP) $@

clean:
	rm -f $(PROGS) *.o core *.eap
