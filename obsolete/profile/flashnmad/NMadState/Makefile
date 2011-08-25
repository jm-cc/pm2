CC=gcc
CFLAGS += -Wall -I../FlashEngine/ -I../Parsing/ -I../NMadState/ -I$(MINGDIR)/include
CFLAGS += -g -O0
LDFLAGS += ../FlashEngine/Interface.o  ../FlashEngine/Anim.o ../FlashEngine/DrawShape.o -lming -ldl

.PHONY: clean FlashEngine

all: NMadState.o NMadStateInit.o 

FlashEngine:
	$(MAKE) -C ../FlashEngine/

clean:
	rm -f *.o
	rm -f NMadState
