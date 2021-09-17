PROG = subscribe_to_event

PKGS = glib-2.0 axevent
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

CFLAGS   += -W \
	    -Wformat=2 \
	    -Wpointer-arith \
	    -Wbad-function-cast \
	    -Wstrict-prototypes \
	    -Wmissing-prototypes \
	    -Winline \
	    -Wdisabled-optimization \
	    -Wfloat-equal \
	    -Wall \
	    -Werror

all:	$(PROG)

clean:
	rm -f $(PROG) *.o *.eap *_LICENSE.txt
