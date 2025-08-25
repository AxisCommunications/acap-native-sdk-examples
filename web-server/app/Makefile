PROG1 = web_server_rev_proxy
OBJS1 = $(PROG1).c
PROGS = $(PROG1)
LIBDIR = lib

PKGS = glib-2.0 civetweb

LDLIBS += -lcivetweb
CFLAGS += -I/opt/build/civetweb/include

LDFLAGS += -L$(LIBDIR) -Wl,--no-as-needed,-rpath,'$$ORIGIN/lib'

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
	$(CC) $^ $(CFLAGS) $(LIBS) $(LDFLAGS) -lm $(LDLIBS) -o $@
	$(STRIP) $@

clean:
	rm -rf $(PROGS) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp* $(LIBDIR)
