PROGS     = axparameter
SRCS      = axparameter.c
OBJS      = $(SRCS:.c=.o)

PKGS = glib-2.0 gio-2.0 axparameter

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

CFLAGS += -O2 \
          -Werror \
          -Wall \
          -Wextra \
          -Wbad-function-cast \
          -Wdisabled-optimization \
          -Wfloat-equal \
          -Wformat=2 \
          -Winline \
          -Wmissing-prototypes \
          -Wpointer-arith \
          -Wstrict-prototypes

all: $(PROGS)

$(PROGS): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LIBS) $(LDLIBS) -o $@

clean:
	rm -f $(PROGS) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp*
