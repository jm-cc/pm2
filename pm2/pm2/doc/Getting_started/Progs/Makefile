
CFLAGS		:= $(shell pm2-config --cflags)
LDFLAGS		:= $(shell pm2-config --libs)
CC		:= $(shell pm2-config --cc)

.PHONY: clean indent

CFILES		:= $(wildcard *.c)
OFILES		:= $(CFILES:%.c=%.o)
EXEFILES	:= $(CFILES:%.c=%)
CPPFILES	:= $(CFILES:%.c=%.C)
ASFILES		:= $(CFILES:%.c=%.s)

all: $(EXEFILES)

clean:
	-rm $(EXEFILES) $(OFILES) $(CPPFILES) *~ 
	-./cleanlogs

$(EXEFILES): %: %.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

$(CPPFILES): %.C: %.c
	@$(CC) -E $(CFLAGS) $< > $@

$(ASFILES): %.s: %.c
	@$(CC) -S $(CFLAGS) $<

indent:
	indent -i2 $(CFILES)
