

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

ifdef APP_RECURSIF

# Target subdirectories
DUMMY_BUILD :=  $(shell mkdir -p $(APP_BIN))
DUMMY_BUILD :=  $(shell mkdir -p $(APP_OBJ))
DUMMY_BUILD :=  $(shell mkdir -p $(APP_ASM))
DUMMY_BUILD :=  $(shell mkdir -p $(APP_DEP))
DUMMY_BUILD :=  $(shell mkdir -p $(APP_CPP))

ifeq ($(wildcard $(DEPENDS)),$(DEPENDS))
include $(DEPENDS)
endif

include $(PM2_ROOT)/make/common-rules.mak

$(DEPENDS): $(COMMON_DEPS)
$(OBJECTS): $(APP_OBJ)/%.o: $(APP_DEP)/%.d $(COMMON_DEPS)

$(APP_OBJ)/%$(APP_EXT).o: $(SRC_DIR)/%.c
	$(COMMON_PREFIX) $(CC) $(CFLAGS) -c $< -o $@

$(DEPENDS): $(APP_DEP)/%$(APP_EXT).d: $(SRC_DIR)/%.c
	$(COMMON_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) -w $< \
		| sed '\''s/.*:/$(subst /,\/,$(DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


$(APP_BIN)/%: $(APP_OBJ)/%.o
	$(COMMON_PREFIX) $(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(PROGS): %: $(APP_BIN)/% ;

# Generation des fichiers 'fut'
.PHONY: fut
fut:
	$(COMMON_PREFIX) $(MAKE) -C . APP_RECURSIF=true $(APP_CPP)/$(TARGET).i $(APP_CPP)/$(TARGET).fut

$(APP_CPP)/%$(LIB_EXT).i: $(SRC_DIR)/%.c
	$(COMMON_PREFIX) $(CC) -E -P -DPREPROC $(CFLAGS) $< > $@

$(APP_CPP)/%.fut: $(APP_CPP)/%.i
	$(COMMON_PREFIX) cp /dev/null $@
	$(COMMON_HIDE) gcc -c -O0 $< -o /tmp/foo-$(USER).o
	$(COMMON_HIDE) nm /tmp/foo-$(USER).o | fgrep this_is_the_ | sed -e 's/^.*this_is_the_//' >> $@
	$(COMMON_HIDE) rm -f /tmp/foo-$(USER).o
	$(COMMON_HIDE) touch $(LIB_GEN_STAMP)/fut_stamp

else # APP_RECURSIF

MAKE_LIBS = +$(MAKE) -C $(PM2_ROOT) libs

.PHONY: all
all: $(PROGS)

include $(PM2_ROOT)/make/common-rules.mak

$(PROGS):
	@$(MAKE_LIBS) APP_TARGET="$@" APP_DIR=$(CURDIR)
	@$(MAKE) APP_RECURSIF=true $@$(APP_EXT)


# Regles de nettoyage
#---------------------------------------------------------------------
.PHONY: clean cleanall refresh refreshall sos
clean cleanall refresh refreshall sos:
	$(COMMON_HIDE) make -s -C $(PM2_ROOT) $@

.PHONY: help bannerhelpapps targethelpapps
help: globalhelp

bannerhelp: bannerhelpapps

bannerhelpapps:
	@echo "This is PM2 Makefile for examples"

targethelp: targethelpapps

PROGSLIST:=$(foreach PROG,$(PROGS),$(PROG))
targethelpapps:
	@echo "  all|examples: build the examples"
	@echo "  help: this help"
	@echo "  clean: clean examples source tree for current flavor"
#	@echo "  distclean: clean examples source tree for all flavors"
	@echo
	@echo "Examples to build:"
	@echo "  $(PROGSLIST)"

endif

$(PM2_MAK_DIR)/apps-config.mak: $(APP_STAMP_FLAVOR)
	$(COMMON_HIDE) $(PM2_GEN_MAK) apps

