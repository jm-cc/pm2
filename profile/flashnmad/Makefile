CFLAGS += -g  -I${MINGDIR}/include/ -I${FXTDIR}/include/  -Wall -O0
CFLAGS += -I./FlashEngine/ -I./Parsing -I./NMadState

LDFLAGS += -lm  -L${MINGDIR}/lib/ -L${FXTDIR}/lib/ -lfxt -lming -ldl

LDFLAGS += ./Parsing/log_preprocess.o ./Parsing/log_process.o
LDFLAGS += ./NMadState/NMadStateInit.o ./NMadState/NMadState.o
LDFLAGS += ./FlashEngine/Interface.o ./FlashEngine/FlashEngineInit.o ./FlashEngine/Anim.o ./FlashEngine/DrawShape.o

CC=gcc

.PHONY: FlashEngine FlashEngine NMadState Parsing

all: FlashNMad

FlashNMad.o: FlashEngine Parsing NMadState

FlashNMad: FlashNMad.o

FlashEngine:
	$(MAKE) -C FlashEngine

NMadState:
	$(MAKE) -C NMadState

Parsing:
	$(MAKE) -C Parsing


project: projet.o

clean:
	rm -f *.o
	rm -f projet projet.swf FlashNMad
	make -C FlashEngine clean
	make -C NMadState clean
	make -C Parsing clean
