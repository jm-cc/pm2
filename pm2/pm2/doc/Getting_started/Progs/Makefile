ifndef PM2_FLAVOR
$(error PM2_FLAVOR is not defined!)
endif

CFLAGS		:= $(shell pm2-config --cflags)
LDFLAGS		:= $(shell pm2-config --libs)
CC		:= $(shell pm2-config --cc)

ifeq ($(OSTYPE),linux-gnu)
CFLAGS		+= -DMAXHOSTNAMELEN=64
endif

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

$(EXEFILES): %: %.c Makefile
	@echo "Compiling $< for flavor '$(PM2_FLAVOR)'..."
	@$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

$(CPPFILES): %.C: %.c
	@$(CC) -E $(CFLAGS) $< > $@

$(ASFILES): %.s: %.c
	@$(CC) -S $(CFLAGS) $<

indent:
	indent --indent-level2 --line-length65 $(CFILES)
