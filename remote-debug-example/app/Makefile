PROG1 = $(shell jq -r '.acapPackageConf.setup.appName' manifest.json)
OBJS1 = $(PROG1).o
DEBUG_DIR = debug

PROGS = $(PROG1)

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

CFLAGS_DEBUG = -g3 -O0
CFLAGS += $(CFLAGS_DEBUG)

.PHONY: all
all:  $(PROGS)

$(PROG1): $(OBJS1)
	install -d $(DEBUG_DIR)
	$(CC) $^ $(CFLAGS) $(LIBS) $(LDFLAGS) -o $(DEBUG_DIR)/$@
	cp $(DEBUG_DIR)/$@ .
	$(STRIP) $@

.PHONY: clean
clean:
	rm -rf $(PROGS) *.o *.eap* *_LICENSE.txt tmp* $(DEBUG_DIR) core
