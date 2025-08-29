PROG1	= customlib_example
OBJS1	= $(PROG1).c
PROGS	= $(PROG1)
DEBUG_DIR = debug

LDLIBS += -Llib
CFLAGS += -Icustom_build
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

SHLIB_DIR = ./lib
LDFLAGS = -L$(SHLIB_DIR) -Wl,-rpath,'$$ORIGIN/lib'
SHLIBS += -lcustom

all:	$(PROGS)

$(PROG1): $(OBJS1)
	install -d $(DEBUG_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS)  $^ $(LIBS) $(LDLIBS) $(SHLIBS) -o $(DEBUG_DIR)/$@
	cp $(DEBUG_DIR)/$@ .
	$(STRIP) $@

clean:
	rm -rf $(PROGS) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp* $(DEBUG_DIR)
