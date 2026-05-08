PROG1	= $(shell jq -r '.acapPackageConf.setup.appName' manifest.json)

PKGS = gio-2.0 glib-2.0 cairo vdostream axoverlay2
CFLAGS += $(shell pkg-config --cflags $(PKGS))
LDLIBS += $(shell pkg-config --libs $(PKGS)) -lm

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

all: axoverlay2

$(PROG1): $(PROG1).c
	mkdir -p debug
	$(CC) $^ $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o debug/$@
	cp debug/$@ .
	$(STRIP) $@

.PHONY: clean
clean:
	rm -rf $(PROG1) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp* debug
