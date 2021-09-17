PROG1	= vdoencodeclient
OBJS1	= $(PROG1).c
PROGS	= $(PROG1)

PKGS = gio-2.0 vdostream gio-unix-2.0

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

CFLAGS += -Wall

all:	$(PROGS)

$(PROG1): $(OBJS1)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) $(LDLIBS) -o $@

clean:
	rm -f $(PROGS) *.o *.eap
