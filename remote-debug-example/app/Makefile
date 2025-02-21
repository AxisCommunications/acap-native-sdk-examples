PROG1 = remote_debug
OBJS1 = $(PROG1).c
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

all:  $(PROGS)

$(PROG1): $(OBJS1)
	install -d $(DEBUG_DIR)
	$(CC) $^ $(CFLAGS) $(CFLAGS_DEBUG) -o $(DEBUG_DIR)/$@
	cp $(DEBUG_DIR)/$@ .
	$(STRIP) $@

clean:
	rm -rf $(PROGS) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp* $(DEBUG_DIR) core
