PROG1	= reproducible_package
OBJS1	= $(PROG1).c

PROGS	= $(PROG1)

CFLAGS-y  = -W -Wformat=2 -Wpointer-arith -Wbad-function-cast -Wstrict-prototypes -Wmissing-prototypes -Winline -Wdisabled-optimization -Wfloat-equal -Wall -Werror

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
	$(CC) $^ $(CFLAGS) $(LIBS) -o $@
	$(STRIP) $@

clean:
	rm -f $(PROGS) *.o core *.conf* *.eap* *_LICENSE.txt
