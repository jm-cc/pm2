# sample makefile for PIOMan examples

PIOMAN_CFLAGS := $(shell pkg-config --cflags pioman)
PIOMAN_LIBS   := $(shell pkg-config --libs pioman)

EXAMPLES := piom_task

CC      = gcc
LD      = $(CC)
CFLAGS  = -Wall -O4 -fPIC $(PIOMAN_CFLAGS)
LDFLAGS =
LIBS    = $(PIOMAN_LIBS)

TARGET_BIN = $(EXAMPLES)

all: $(TARGET_BIN)

%.pic.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET_BIN): %: %.pic.o
	$(LD) $(LDFLAGS) -o $@ $+ $(LIBS)

clean:
	-/bin/rm $(TARGET_BIN) *.pic.o

