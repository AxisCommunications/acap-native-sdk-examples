PROG1	= vapix_example
OBJS1	= $(PROG1).c
PROGS	= $(PROG1)
LIBDIR = lib

PKGS = gio-2.0
CURL_INSTALL_DIR=/opt/build/curl/install
JANSSON_INSTALL_DIR=/opt/build/jansson/install

# Build the application with the libraries previously built using the -l
# option and point out the library directory under the application directory
# with -L option.
# Setting rpath is required for the installed application to search the
# libraries in the application directory and not in AXIS OS standard path.
LDLIBS += -lcurl -ljansson
LDFLAGS = -L$(LIBDIR) -Wl,--no-as-needed,-rpath,'$$ORIGIN/$(LIBDIR)'

# curl header files need to get pointed out
CFLAGS += -I$(CURL_INSTALL_DIR)/include/curl
CFLAGS += -I$(JANSSON_INSTALL_DIR)/include

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

# Set warning flags
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
	[ -d "${LIBDIR}" ] || { \
		mkdir -p ${LIBDIR} ; \
		cp ${CURL_INSTALL_DIR}/lib/libcurl.so* ${LIBDIR}/ ; \
		cp ${JANSSON_INSTALL_DIR}/lib/libjansson.so* ${LIBDIR}/ ; \
	}

	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS)  $^ -o $@

clean:
	rm -rf $(PROGS) $(LIBDIR) *.o *.eap* *_LICENSE.txt package.conf* param.conf tmp*
