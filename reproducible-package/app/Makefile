PROG1	= $(shell jq -r '.acapPackageConf.setup.appName' manifest.json)
OBJS1	= $(PROG1).c
DEBUG_DIR = debug

PROGS	= $(PROG1)

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

# Set to create a reproducible package
DATE_FMT = +%Y-%m-%d
ifdef SOURCE_DATE_EPOCH
    BUILD_DATE ?= $(shell \
        date -u -d "@$(SOURCE_DATE_EPOCH)" "$(DATE_FMT)" 2>/dev/null || \
				date -u -r "$(SOURCE_DATE_EPOCH)" "$(DATE_FMT)" 2>/dev/null || \
				date -u "$(DATE_FMT)")
else
    BUILD_DATE ?= $(shell date "$(DATE_FMT)")
endif

all:	$(PROGS)

$(PROG1): $(OBJS1)
	# Additional prints for visibility
	@printf "SOURCE_DATE_EPOCH: $(SOURCE_DATE_EPOCH)\n"
	@printf "BUILD_DATE: $(BUILD_DATE)\n"
	install -d $(DEBUG_DIR)
	$(CC) $^ $(CFLAGS) $(LIBS) $(LDFLAGS) -lm $(LDLIBS) -o $(DEBUG_DIR)/$@
	cp $(DEBUG_DIR)/$@ .
	$(STRIP) $@

clean:
	rm -rf $(PROGS) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp* *.conf* $(DEBUG_DIR)
