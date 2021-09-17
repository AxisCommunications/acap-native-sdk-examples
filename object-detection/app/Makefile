PROG1	= object_detection
OBJS1	= $(PROG1).c argparse.c imgconverter.c imgprovider.c imgutils.c
PROGS	= $(PROG1)

PKGS = gio-2.0 vdostream gio-unix-2.0 liblarod

CFLAGS  += -Iinclude

LDLIBS  += -lyuv -ljpeg
LDFLAGS += -L./lib -Wl,-rpath,'$$ORIGIN/lib'

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

CFLAGS += -Wall -DLAROD_API_VERSION_1

all:	$(PROGS) \
		model/converted_model.tflite \
		label/labels.txt

$(PROG1): $(OBJS1)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

model/converted_model.tflite:
	mkdir -p model
	cp converted_model_edgetpu.tflite model/converted_model.tflite

label/labels.txt:
	mkdir -p label
	curl -L -o label/labels.txt https://github.com/google-coral/test_data/raw/master/coco_labels.txt

clean:
	rm -f $(PROGS) *.o *.eap lib/* include/* model/* label/*
