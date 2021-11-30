DEPS = liblarod
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config \
            --cflags $(DEPS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config \
            --libs $(DEPS))
CFLAGS += -Wall -Wextra -Werror

# This example is based on larod version 1
CFLAGS += -DLAROD_API_VERSION_1

.PHONY: all
all: larod_simple_app 

.PHONY: clean
clean:
	-mv package.conf.orig package.conf
	rm -f larod_simple_app *.o *.eap *_LICENSE.txt
	rm -rf model
