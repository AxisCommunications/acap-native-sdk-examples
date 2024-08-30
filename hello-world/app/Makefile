PROG1	= hello_world
OBJS1	= $(PROG1).c

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

all:	$(PROGS)

$(PROG1): $(OBJS1)
	$(CC) $^ $(CFLAGS) $(LIBS) -o $@
	$(STRIP) $@

clean:
	rm -f $(PROGS) *.o core *.eap
