PROG     = hello_world
OBJS     = $(PROG).c
MAJORVER = 1
MINORVER = 0
PATCH    = 0
TARGET   = libcustom.so.$(MAJORVER).$(MINORVER).$(PATCH)
DEBUG_DIR = debug

CFLAGS += -I.
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
	  -Werror \
	  -fPIC

all: $(TARGET) custom_link

$(TARGET): $(OBJS)
	install -d $(DEBUG_DIR)
	$(CC) $(CFLAGS) -shared $^ -o $(DEBUG_DIR)/$@
	cp $(DEBUG_DIR)/$@ .
	$(STRIP) $@

custom_link:
	ln -sf libcustom.so.$(MAJORVER).$(MINORVER).$(PATCH) libcustom.so.$(MAJORVER)
	ln -sf libcustom.so.$(MAJORVER) libcustom.so

clean:
	rm -rf libcustom* $(DEBUG_DIR)
