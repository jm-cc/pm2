ifndef PM2_FLAVOR
$(error PM2_FLAVOR is not yet defined!)
endif

CFLAGS		:= $(shell pm2-config --cflags)
LDFLAGS		:= $(shell pm2-config --libs)
CC		:= $(shell pm2-config --cc)

.PHONY: all clean

CFILES		:= $(wildcard *.c)
OFILES		:= $(CFILES:%.c=%.o)
EXEFILES	:= $(CFILES:%.c=%)
CPPFILES	:= $(CFILES:%.c=%.C)
ASFILES		:= $(CFILES:%.c=%.s)

all: $(EXEFILES)

clean:
	-rm $(EXEFILES) $(OFILES) $(CPPFILES) *~ 
	-./cleanlogs

$(EXEFILES): %: %.c Makefile
	@echo "Compiling $< for flavor '$(PM2_FLAVOR)'..."
	@$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

$(CPPFILES): %.C: %.c
	@$(CC) -E $(CFLAGS) $< > $@

$(ASFILES): %.s: %.c
	@$(CC) -S $(CFLAGS) $<

indent: $(CFILES)
	indent --indent-level2 --line-length65 $?
	@touch indent