

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

include $(PM2_ROOT)/make/common-rules.mak

ifdef PROG_RECURSIF
$(PROGRAM):

ifneq ($(PROGRAM),$(PROGNAME))
$(PROGNAME): $(PROGRAM)
endif

prog: $(PROGRAM)

all: $(PROGRAM)

.PHONY: $(PROGRAM) $(PROGNAME) prog

ifdef SHOW_FLAVOR
$(PROGRAM): showflavor
endif

ifdef SHOW_FLAGS
$(PROGRAM): showflags
endif

$(PROGRAM): $(PRG_PRG)

.PHONY: showflags
showflags:
	@echo $(CFLAGS)

showflavor:
	@echo Compiling for flavor: $(FLAVOR)

ifeq (,$(findstring _$(MAKECMDGOALS)_,_clean_ $(DO_NOT_GENERATE_MAK_FILES)))
# Target subdirectories
DUMMY_BUILD :=  $(foreach REP, $(PRG_REP_TO_BUILD), $(shell mkdir -p $(REP)))
ifeq ($(wildcard $(PRG_DEPENDS)),$(PRG_DEPENDS))
include $(PRG_DEPENDS)
endif
endif

$(PRG_DEPENDS): $(COMMON_DEPS) $(PRG_GEN_C_SOURCES) $(PRG_GEN_C_INC)
$(PRG_OBJECTS): $(PRG_GEN_OBJ)/%.o: $(PRG_GEN_DEP)/%.d $(COMMON_DEPS)

$(PRG_PRG): $(PRG_OBJECTS)
	@ echo "Linking program"
	$(PRG_PREFIX) $(CC) $(PRG_OBJECTS) $(LDFLAGS) -o $(PRG_PRG)

# C
$(PRG_C_OBJECTS): $(PRG_GEN_OBJ)/%$(PRG_EXT).o: $(PRG_SRC)/%.c
	$(PRG_PREFIX) $(CC) $(CFLAGS) -c $< -o $@

$(PRG_C_DEPENDS): $(PRG_GEN_DEP)/%$(PRG_EXT).d: $(PRG_SRC)/%.c
	$(PRG_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) -w $< \
		| sed '\''s/.*:/$(subst /,\/,$(PRG_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

# Assembleur
$(PRG_S_OBJECTS): $(PRG_GEN_OBJ)/%$(PRG_EXT).o: $(PRG_SRC)/%.S
	$(COMMON_HIDE) $(CC) -E -P $(CFLAGS) $< > $(PRG_OBJ_TO_S)
	$(PRG_PREFIX) $(AS) $(CFLAGS) -c $(PRG_OBJ_TO_S) -o $@

$(PRG_S_DEPENDS): $(PRG_GEN_DEP)/%$(PRG_EXT).d: $(PRG_SRC)/%.S
	$(PRG_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(PRG_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

# Compilation des fichiers sources generes
$(PRG_GEN_C_OBJECTS): $(PRG_GEN_OBJ)/%$(PRG_EXT).o: $(PRG_GEN_SRC)/%.c
	$(PRG_PREFIX) $(CC) $(CFLAGS) -c $< -o $@

$(PRG_GEN_C_DEPENDS): $(PRG_GEN_DEP)/%$(PRG_EXT).d: $(PRG_GEN_SRC)/%.c
	$(PRG_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(PRG_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

# Lex

$(PRG_GEN_C_L_SOURCES): $(PRG_GEN_SRC)/%$(PRG_EXT).c: $(PRG_SRC)/%.l
	$(COMMON_HIDE) $(LEX) $<; mv lex.yy.c $@

# Yacc
$(PRG_GEN_C_Y_SOURCES): $(PRG_GEN_SRC)/%$(PRG_EXT).c: $(PRG_SRC)/%.y
	$(COMMON_HIDE) $(YACC) $<; mv y.tab.c $@; mv y.tab.h $(PRG_GEN_C_TO_H)

$(PRG_GEN_C_Y_INC): $(PRG_GEN_INC)/%$(PRG_EXT).h: $(PRG_SRC)/%.y
	$(COMMON_HIDE) $(YACC) $<; mv y.tab.h $@; mv y.tab.h $(PRG_GEN_H_TO_C)

else # !PROG_RECURSIF: premier appel
MAKE_LIBS = +set -e ; for modules in $(CONFIG_MODULES); do \
                if [ $$modules != $(PROGRAM) ] ; then \
		    $(MAKE) -C $(PM2_ROOT)/modules/$$modules ; \
                fi ;\
	    done 

.PHONY: $(PROGRAM)
$(PROGRAM): # prgclean
	@ echo "Making libs"
	$(COMMON_HIDE) $(MAKE_LIBS)
	@ echo "Making program"
	$(COMMON_HIDE) $(MAKE) PROG_RECURSIF=true $@
	@ echo "Program ready"



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

endif # !PROG_RECURSIF


$(PM2_MAK_DIR)/progs-config.mak: $(PRG_STAMP_FLAVOR)
	$(COMMON_HIDE) $(PM2_GEN_MAK) progs

ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
$(PM2_MAK_DIR)/progs-libs.mak: $(PRG_STAMP_FLAVOR)
	@mkdir -p `dirname $@`
	@echo "CONFIG_MODULES= " `$(PM2_CONFIG) --modules` > $@
endif
