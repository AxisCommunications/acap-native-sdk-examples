PROG1	= web_server_rev_proxy
OBJS1	= $(PROG1).c
PROGS	= $(PROG1)
MONKEY_DIR = /opt/build/monkey/build
APP_DIR = /usr/local/packages/$(PROG1)
LIBDIR = lib
HTMLDIR = html

PKGS = glib-2.0 monkey
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

CFLAGS += -I$(MONKEY_DIR)/include
LDFLAGS += -L./$(LIBDIR) -Wl,-rpath,'$$ORIGIN/$(LIBDIR)'

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
	cp -r $(MONKEY_DIR)/$(LIBDIR) .
	cp -r $(MONKEY_DIR)/$(HTMLDIR) .
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm -rf $(PROGS) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp* $(LIBDIR) $(HTMLDIR)
