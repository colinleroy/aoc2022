PROGRAM=homectrl grphview
homectrl_SOURCES := main.c network.c \
	switches.c sensors.c climate.c \
	server_url.c \
	../lib/simple_serial.c \
	../lib/extended_conio.c \
	../lib/extended_string.c \
	../lib/surl.c

grphview_SOURCES := graph-viewer.c \
	../lib/surl.c \
	../lib/simple_serial.c

grphview_GCCSOURCES := ../lib/extended_conio.c ../lib/tgi_sdl.c ../lib/extended_string.c ../lib/tgi_fastline.c

DEPS := 

CC65_TARGET = apple2enh
CFLAGS  = -I ../lib 
homectrl_CLCFLAGS = -t $(CC65_TARGET) -Wl -D,__STACKSIZE__=0x0400 -DSERIAL_TO_LANGCARD
grphview_CLCFLAGS = -t $(CC65_TARGET) -Wl -D,__STACKSIZE__=0x0400 -DSERIAL_TO_LANGCARD -C ../../config/apple2enh-hgr.cfg
GCCCFLAGS = -g -O0 -Werror $(shell sdl-config --cflags)
GCCLDFLAGS = $(shell sdl-config --libs)
.PHONY: all clean

all: homectrl.bin homectrl.x86_64 grphview.bin grphview.x86_64

homectrl.bin: $(homectrl_SOURCES)
	cl65 $(CFLAGS) $(homectrl_CLCFLAGS) -m homectrl.map -o $@ $^

homectrl.x86_64: $(homectrl_SOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

grphview.bin: $(grphview_SOURCES)
	cl65 $(CFLAGS) $(grphview_CLCFLAGS) -m grphview.map -o $@ $^

grphview.x86_64: $(grphview_SOURCES) $(grphview_GCCSOURCES)
	gcc $(CFLAGS) $(GCCCFLAGS) $^ $(GCCLDFLAGS) -o $@

clean:
	rm -f homectrl.bin homectrl.x86_64 grphview.bin grphview.x86_64 *.o *.map

upload: all
	scp homectrl.bin grphview.bin diskstation.lan:/volume1/a2repo/
