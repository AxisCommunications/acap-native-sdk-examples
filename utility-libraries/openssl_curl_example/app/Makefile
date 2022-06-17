PROG1	= openssl_curl_example
OBJS1	= $(PROG1).c
PROGS	= $(PROG1)

# Build the application with the libraries previously built using the -l
# option and point out the library directory under the application directory
# with -L option.
# Setting rpath is required for the installed application to search the
# libraries in the application directory and not in AXIS OS standard path.
LDLIBS += -lssl -lcrypto -lcurl
LDFLAGS = -Llib -Wl,--no-as-needed,-rpath,'$$ORIGIN/lib'

# Since OpenSSL build files are installed to the SDK library path, those header
# files are found from the SDK environment variables, but curl header files
# need to get pointed out
CFLAGS += -I/opt/build/curl/install/include/curl

# Set warning flags
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

# Pre-process debug setting if it's set in the Dockerfile
ifneq ($(strip $(APP_DEBUG)),)
CFLAGS += -DAPP_DEBUG="\"$(APP_DEBUG)\""
endif

# Pre-process proxy setting if it's set in the Dockerfile
ifneq ($(strip $(APP_PROXY)),)
CFLAGS += -DAPP_PROXY="\"$(APP_PROXY)\""
endif

all:	$(PROGS)

$(PROG1): $(OBJS1)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS)  $^ $(LIBS) -o $@

clean:
	rm -f $(PROGS) *.o *.eap *_LICENSE.txt package.conf* param.conf
