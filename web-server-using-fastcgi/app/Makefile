PROG1	= fastcgi_example
OBJS1	= $(PROG1).c

PROGS	= $(PROG1)

PKGS = fcgi
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS) )
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

# Link the built library
LDFLAGS = -L./lib -Wl,--no-as-needed,-rpath,'$$ORIGIN/lib'
CFLAGS += -I/opt/build/uriparser/build/include
LDLIBS += -luriparser

CFLAGS += -W -Wformat=2 -Wpointer-arith -Wbad-function-cast -Winline -Wdisabled-optimization -Wfloat-equal -Wall -Werror

all:	$(PROGS)

$(PROG1): $(OBJS1)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $^ -o $@

clean:
	rm -f $(PROGS) *.o *.eap* pa* *_LICENSE.txt
