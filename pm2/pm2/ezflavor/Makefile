
ifndef PM2_HOME
PM2_HOME	:=	$(HOME)
endif

BIN_DIR		:=	$(PM2_HOME)/.pm2/bin

PROGRAM		:=	$(BIN_DIR)/ezflavor_`pm2_arch`_`pm2_sys`

OPTIONS		:=	-DUSE_TRACE -g

SYS		:=	$(shell pm2_sys)

CC		:=	gcc
CFLAGS		:=	-O6 -Wall $(shell gtk-config --cflags) $(OPTIONS)
LDFLAGS		:=	$(shell gtk-config --libs) -lfl

ifneq (,$(findstring $(SYS),"WIN98_SYS_WINNT_SYS_WIN2K_SYS"))
CFLAGS		+=	-fnative-struct
LDFLAGS		+=	-e _mainCRTStartup
endif

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
	@mkdir -p $(BIN_DIR)
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

