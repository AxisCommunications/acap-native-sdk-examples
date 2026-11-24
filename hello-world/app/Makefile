PROG1 = $(shell jq -r '.acapPackageConf.setup.appName' manifest.json)
OBJS1 = $(PROG1).o

PROGS = $(PROG1)
DEBUG_DIR = debug

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

.PHONY: all
all: $(PROGS)

$(PROG1): $(OBJS1)
	install -d $(DEBUG_DIR)
	$(CC) $^ $(CFLAGS) $(LIBS) $(LDFLAGS) -lm $(LDLIBS) -o $(DEBUG_DIR)/$@
	cp $(DEBUG_DIR)/$@ .
	$(STRIP) $@

.PHONY: clean
clean:
	rm -rf $(PROGS) *.o *.eap* *_LICENSE.txt tmp* $(DEBUG_DIR)
