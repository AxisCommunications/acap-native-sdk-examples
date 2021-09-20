PROG1	= curl_example
OBJS1	= $(PROG1).c
PROGS	= $(PROG1)

LDLIBS += -Llib
CFLAGS += -I/opt/build/curl/build/include/curl
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

# Pre-prcessing proxy setting
ifneq ($(strip $(CURL_PROXY)),)
CFLAGS += -DCURL_PROXY="\"$(CURL_PROXY)\""
endif

SHLIB_DIR = ./lib
LDFLAGS = -L$(SHLIB_DIR) -Wl,--no-as-needed,-rpath,'$$ORIGIN/lib'
SHLIBS += -lm -lcurl -lssl

all:	$(PROGS)

$(PROG1): $(OBJS1)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SHLIBS) $(LDLIBS)  $^ $(LIBS) $(LDLIBS) -o $@

clean:
	rm -f $(PROGS) *.o *.eap *_LICENSE.txt package.conf* param.conf
