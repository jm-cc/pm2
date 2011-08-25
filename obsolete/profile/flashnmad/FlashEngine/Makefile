$(CC)=gcc

CFLAGS += -Wall -I../Parsing -I../NMadState -I../FlashEngine/ -I${MINGDIR}/include/
CFLAGS += -g -O3
LDFLAGS+= -lm -ldl

all: Anim.o DrawShape.o Interface.o FlashEngineInit.o

clean:
	rm -f *.o
