PROG1 = vdo_cl_filter_demo
OBJS1 = $(PROG1).c
PROGS = $(PROG1)

PKGS = gio-unix-2.0 glib-2.0 opencl vdostream

CFLAGS += -Wall
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

all: $(PROGS)

$(PROG1): $(OBJS1)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) $(LDLIBS)  -o $@

clean:
	rm -f $(PROGS) *.o *.eap
