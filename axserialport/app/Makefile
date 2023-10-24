PROGS = axserialport
SRCS  = axserialport.c
OBJS  = $(SRCS:.c=.o)

PKGS = glib-2.0 axserialport

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

CFLAGS += -Wall -O2

all: $(PROGS)

$(PROGS): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LIBS) $(LDLIBS) -lgthread-2.0 -o $@

clean:
	rm -f $(PROGS) *.o *.a core
