PROGS = axserialport
SRCS  = axserialport.c
OBJS  = $(SRCS:.c=.o)

PKGS = glib-2.0 axserialport

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

CFLAGS += -Wall -g -O2 -Werror

all: $(PROGS)

$(PROGS): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm -f $(PROGS) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp*
