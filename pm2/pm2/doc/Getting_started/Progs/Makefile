
FLAVOR		:= $(shell echo $$PM2_FLAVOR")
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
	@echo "Compiling $< for flavor '$(FLAVOR)'..."
	@$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

$(CPPFILES): %.C: %.c
	@$(CC) -E $(CFLAGS) $< > $@

$(ASFILES): %.s: %.c
	@$(CC) -S $(CFLAGS) $<

indent:
	indent --indent-level2 --line-length65 $(CFILES)
