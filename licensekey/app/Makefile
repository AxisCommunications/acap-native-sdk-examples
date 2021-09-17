PROGS     = licensekey_handler
SRCS      = licensekey_handler.c
OBJS      = $(SRCS:.c=.o)
PKGS      = glib-2.0

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

CFLAGS   += -Wall -g -O2

LDLIBS   += -Wl,-Bstatic,-llicensekey_stat,-Bdynamic,-llicensekey -ldl

all: $(PROGS)

$(PROGS): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm -f $(PROGS) *.o
