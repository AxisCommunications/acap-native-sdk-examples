PROG1	= customlib_example
OBJS1	= $(PROG1).c
PROGS	= $(PROG1)

LDLIBS += -Llib
CFLAGS += -Icustom_build
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

SHLIB_DIR = ./lib
LDFLAGS = -L$(SHLIB_DIR) -Wl,-rpath,'$$ORIGIN/lib'
SHLIBS += -lcustom

all:	$(PROGS)

$(PROG1): $(OBJS1)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS)  $^ $(LIBS) $(LDLIBS) $(SHLIBS) -o $@

clean:
	rm -f $(PROGS) *.o *.eap *_LICENSE.txt package.conf* param.conf
