

# PM2: Parallel Multithreaded Machine
# Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

ifndef PM2_HOME
PM2_HOME	:=	$(HOME)
endif
PM2_ARCH	:=	$(shell $(PM2_ROOT)/bin/pm2_arch)
PM2_SYS		:=	$(shell $(PM2_ROOT)/bin/pm2_sys)

BIN_DIR		:=	$(PM2_HOME)/.pm2/bin
TMP_DIR		:=	/tmp/$(USER)_supertrace_$(PM2_ARCH)_$(PM2_SYS)

PROGRAM		:=	$(BIN_DIR)/supertrace_$(PM2_ARCH)_$(PM2_SYS)

ifeq (,$(findstring _$(MAKECMDGOALS)_,"_clean_ _cleanall_"))

DUMMY_BUILD	:=	$(shell if [ ! -d $(BIN_DIR) ]; then mkdir -p $(BIN_DIR); fi)
DUMMY_BUILD	:=	$(shell if [ ! -d $(TMP_DIR) ]; then mkdir -p $(TMP_DIR); fi)

OPTIONS		:=	-g

CC		:=	gcc
CFLAGS		:=	-O6 -Wall $(OPTIONS)
LDFLAGS		:=	 -lfl

ifneq (,$(findstring $(PM2_SYS),"WIN98_SYS_WINNT_SYS_WIN2K_SYS"))
CFLAGS		+=	-fnative-struct
LDFLAGS		+=	-e _mainCRTStartup
endif

SOURCES		:=	$(wildcard *.c)
GEN_SOURCES	:=	

OBJECTS		:=	$(patsubst %.c,$(TMP_DIR)/%.o,$(SOURCES))
EXTRA_OBJECTS	:=	$(subst .c,.o,$(GEN_SOURCES))
ALL_OBJECTS	:=	$(strip $(OBJECTS) $(EXTRA_OBJECTS))

DEPENDS		:=	$(subst .o,.d,$(OBJECTS))
EXTRA_DEPENDS	:=	$(subst .o,.d,$(EXTRA_OBJECTS))

DEP_TO_OBJ	=	$(subst .d,.o,$@)
LEX             =       flex

default: $(PROGRAM)

ifneq ($(wildcard $(DEPENDS)),)
include $(wildcard $(DEPENDS))
endif

ifneq ($(wildcard $(EXTRA_DEPENDS)),)
include $(wildcard $(EXTRA_DEPENDS))
endif

$(ALL_OBJECTS): %.o: %.d Makefile

# Program
ifdef NO_REBUILD
ifneq ($(wildcard $(PROGRAM)),)
$(PROGRAM):
else
$(PROGRAM): $(ALL_OBJECTS)
endif
else
$(PROGRAM): $(ALL_OBJECTS)
endif
	$(CC) -o $@ $^ $(LDFLAGS)

# Objects
$(OBJECTS): $(TMP_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(EXTRA_OBJECTS): $(TMP_DIR)/%.o: $(TMP_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

# Depends
$(DEPENDS): $(TMP_DIR)/%.d: %.c Makefile
	$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

$(EXTRA_DEPENDS): $(TMP_DIR)/%.d: $(TMP_DIR)/%.c Makefile
	$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

# Generated sources
$(TMP_DIR)/parser.c: parser.l
	$(LEX) -t $< > $@

endif # clean, cleanall

.PHONY: clean cleanall
clean:
	@echo "Cleaning supertrace..."
	@rm -rf $(PROGRAM) $(TMP_DIR)

cleanall:
	@echo "Cleaning supertrace..."
	@rm -rf $(BIN_DIR)/supertrace_* /tmp/$(USER)_supertrace_*
