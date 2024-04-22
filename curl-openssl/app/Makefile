PROG1	= curl_openssl
OBJS1	= $(PROG1).c
PROGS	= $(PROG1)

PKGS = openssl libcurl
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

# Set warning flags
CFLAGS += -Wall \
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

# Pre-process debug setting if it's set in the Dockerfile
ifneq ($(strip $(APP_DEBUG)),)
CFLAGS += -DAPP_DEBUG="\"$(APP_DEBUG)\""
endif

# Pre-process proxy setting if it's set in the Dockerfile
ifneq ($(strip $(APP_PROXY)),)
CFLAGS += -DAPP_PROXY="\"$(APP_PROXY)\""
endif

all:	$(PROGS)

$(PROG1): $(OBJS1)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm -f $(PROGS) *.o *.eap *_LICENSE.txt package.conf* param.conf cacert.pem
