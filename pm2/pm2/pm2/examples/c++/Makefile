
CC	:=	g++

CFLAGS	:=	$(shell pm2-config --cflags) $(OPTIONS)
LIBS	:=	$(shell pm2-config --libs)
DEP	:=	$(shell pm2-config --stampfile all)
BUILD	:=	$(shell pm2-config --builddir)
BIN	:=	$(BUILD)/examples/c++/bin

DUMMY := $(shell mkdir -p $(BIN))

PROGS := hello

$(BIN)/hello: $(DEP)

$(BIN)/%: %.o
	$(CC) -o $@ $< $(LIBS)

%.o: %.C
	$(CC) -c $< $(CFLAGS) -o $@
