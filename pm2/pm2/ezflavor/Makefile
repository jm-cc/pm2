
PROGRAM		:=	$(PM2_ROOT)/bin/ezflavor

OPTIONS		:=	-DUSE_TRACE -g

CC		:=	gcc
CFLAGS		:=	-O6 -Wall $(shell gtk-config --cflags) $(OPTIONS)
LDFLAGS		:=	$(shell gtk-config --libs) -lfl

SOURCES		:=	$(wildcard *.c)

ifeq ($(wildcard parser.c),)
SOURCES		+=	parser.c
endif

OBJECTS		:=	$(subst .c,.o,$(SOURCES))
DEPENDS		:=	$(subst .c,.d,$(SOURCES))

DEP_TO_OBJ	=	$(subst .d,.o,$@)
LEX             =       flex

default: $(PROGRAM)

ifneq ($(wildcard $(DEPENDS)),)
include $(wildcard $(DEPENDS))
endif

$(OBJECTS): %.o: %.d Makefile

$(PROGRAM): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) -c $<

$(DEPENDS): %.d: %.c Makefile
	$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

parser.c: parser.l

.PHONY: clean
clean:
	rm -rf $(OBJECTS) $(DEPENDS) $(PROGRAM) parser.c

