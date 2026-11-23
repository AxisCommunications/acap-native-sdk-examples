PROG1	= $(shell jq -r '.acapPackageConf.setup.appName' manifest.json)
PROGS	= $(PROG1).c trustreport_fcgi.c trustreport_templates.c certset.c

PKGS = fcgi glib-2.0 libcrypto libsystemd
CFLAGS += $(shell pkg-config --cflags $(PKGS))
LDLIBS += $(shell pkg-config --libs $(PKGS))

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

all:	$(PROG1)

$(PROG1): $(PROGS)
	mkdir -p debug
	$(CC) $^ $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o debug/$@
	cp debug/$@ .
	$(STRIP) $@

.PHONY: clean
clean:
	rm -f *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp*
	rm -rf debug
