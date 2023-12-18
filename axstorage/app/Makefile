PROGS     = axstorage
SRCS      = axstorage.c
OBJS      = $(SRCS:.c=.o)

PKGS = glib-2.0 gio-2.0 axstorage

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

CFLAGS   += -Wall -g -O2 -Werror

all: $(PROGS)

$(PROGS): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LIBS) $(LDLIBS) -o $@

clean:
	rm -f $(PROGS) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp*
