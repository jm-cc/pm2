CC=gcc

LDFLAGS+=$(shell pkg-config --libs fxt) -ldl
CFLAGS+= -static -I../NMadState/ -I../Parsing/ -I../FlashEngine/ $(shell pkg-config --cflags fxt) -I$(MINGDIR)/include
CFLAGS+= -g -O0

all: log_preprocess.o log_process.o

fxtreader: fxtreader.o nmad_fxt.h

clean:
	rm -f *.o fxtreader
