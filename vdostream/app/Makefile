PROG1	= vdoencodeclient
OBJS1	= $(PROG1).c
PROGS	= $(PROG1)

PKGS = gio-2.0 gio-unix-2.0 vdostream

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

CFLAGS += -Wall -g -O2 -Werror

all: $(PROGS)

$(PROG1): $(OBJS1)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm -f $(PROGS) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp*
