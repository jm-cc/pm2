

# PM2: Parallel Multithreaded Machine
# Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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
ifeq ($(wildcard $(DEPENDS)),$(DEPENDS))
include $(DEPENDS)
endif

examples: $(APPS)
.PHONY: examples

include $(PM2_ROOT)/make/common-rules.mak

all: examples

$(DEPENDS): $(COMMON_DEPS)
$(OBJECTS): $(APP_OBJ)/%.o: $(APP_DEP)/%.d $(COMMON_DEPS)

$(APP_OBJ)/%$(APP_EXT).o: $(SRC_DIR)/%.c
	$(COMMON_PREFIX) $(CC) $(CFLAGS) -c $< -o $@

$(DEPENDS): $(APP_DEP)/%$(APP_EXT).d: $(SRC_DIR)/%.c
	$(COMMON_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


$(APP_BIN)/%: $(APP_OBJ)/%.o
	$(COMMON_PREFIX) $(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

%: $(APP_BIN)/% ;

else 

MAKE_LIBS = $(MAKE) -C $(PM2_ROOT) libs

examples:
.PHONY: examples $(APPS)

include $(PM2_ROOT)/make/common-rules.mak

all: examples

examples:
	$(COMMON_HIDE) $(MAKE_LIBS)
	$(COMMON_HIDE) $(MAKE) APP_RECURSIF=true $@

# Regles de nettoyage
#---------------------------------------------------------------------
.PHONY: clean cleanall refresh refreshall sos
clean cleanall refresh refreshall sos:
	$(COMMON_HIDE) make -s -C $(PM2_ROOT) $@


$(PROGS):
	@$(MAKE_LIBS)
	@$(MAKE) APP_RECURSIF=true $@$(APP_EXT)


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

ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
$(PM2_MAK_DIR)/apps-libs.mak:  $(APP_STAMP_FLAVOR)
	$(COMMON_HIDE) mkdir -p `dirname $@`
	$(COMMON_HIDE) echo "CONFIG_MODULES= " `$(PM2_CONFIG) --modules` > $@
endif
