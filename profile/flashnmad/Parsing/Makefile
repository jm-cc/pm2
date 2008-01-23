CC=gcc

LDFLAGS+=-L$(FXTDIR)/lib -lfxt -ldl
CFLAGS+= -static -I../NMadState/ -I../Parsing/ -I../FlashEngine/ -I$(FXTDIR)/include -I$(MINGDIR)/include
CFLAGS+= -g -O0

all: log_preprocess.o log_process.o

fxtreader: fxtreader.o nmad_fxt.h

clean:
	rm -f *.o fxtreader
