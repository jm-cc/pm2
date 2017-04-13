# sample makefile for PIOMan examples


EXAMPLES := piom_bench ltask_mini test_cond

CC      = gcc
LD      = $(CC)
CFLAGS  = -Wall -O4 -DNDEBUG -fPIC $(shell pkg-config --cflags tbx pioman)
LDFLAGS =
LIBS    = $(shell pkg-config --libs tbx pioman)

TARGET_BIN = $(EXAMPLES)

all: $(TARGET_BIN)

%.pic.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET_BIN): %: %.pic.o
	$(LD) $(LDFLAGS) -o $@ $+ $(LIBS)

clean:
	-/bin/rm $(TARGET_BIN) *.pic.o

