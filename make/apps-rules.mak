# -*- mode: makefile;-*-

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

include $(PM2_SRCROOT)/make/objs-rules.mak

.PHONY: all examples
all: $(APPS_LIST)
	$(COMMON_HIDE) echo $(APPS_LIST) sucessfully generated

examples: all

.PHONY: flavor
$(MOD_STAMP_FILES): flavor
flavor:
	$(COMMON_HIDE) echo ">>> Generating libraries..."
	$(COMMON_MAIN) $(MAKE) -C $(PM2_OBJROOT)
	$(COMMON_HIDE) echo "<<< Generating libraries: done"

$(MOD_GEN_BIN)/%: $(MOD_STAMP_FILES)
	$(COMMON_LINK)
	$(COMMON_MAIN) $(CC) $(EARLY_LDFLAGS) $(filter %.o, $^) $(LDFLAGS) -o $@

.PHONY: $(APPS_LIST)
$(APPS_LIST): %: flavor $(MOD_GEN_BIN)/%

# Generation des fichiers 'fut'
#.PHONY: fut
#fut:
#	$(COMMON_BUILD)
#	$(COMMON_MAIN) $(MAKE) -C . APP_RECURSIF=true $(APP_CPP)/$(TARGET).i $(APP_CPP)/$(TARGET).fut

#$(APP_CPP)/%$(LIB_EXT).i: $(SRC_DIR)/%.c
#	$(COMMON_BUILD)
#	$(COMMON_MAIN) $(CC) $(CPPFLAGS) -E -DPREPROC $(CFLAGS) $< > $@

#$(APP_CPP)/%.fut: $(APP_CPP)/%.i
#	$(COMMON_BUILD)
#	$(COMMON_MAIN) cp /dev/null $@
#	$(COMMON_HIDE) gcc -c -O0 $< -o /tmp/foo-$$$$.o && \
#	nm /tmp/foo-$$$$.o | fgrep this_is_the_ | sed -e 's/^.*this_is_the_//' >> $@ && \
#	rm -f /tmp/foo-$$$$.o
#	$(COMMON_HIDE) touch $(LIB_GEN_STAMP)/fut_stamp

#else # APP_RECURSIF


# Inclusion du cache de configuration spécific des programmes
#---------------------------------------------------------------------
MOD_LINKED_OBJECTS=$(APPS_LIST)

# Dépendance faible ( '|' ) sur flavor juste pour s'assurer qu'elle est
# bien construite avant (surtout avec make -j), en particulier pour les
# headers. 
# Il y a déjà d'autres dépendances pour forcer la reconstruction du .o et/ou du
# programme si les bibliothèques de la flavors ont été modifiées
#
# Pb:
# Si $(MOD_STAMP_FILES) est modifié pendant la recompilation de flavor,
# make ne s'en apperçoit pas (avec -j) et ne reconstruit pas la cible.
# Solutions possibles
# - double run pour que make regarde à nouveau les dépendances
# - dépendances complètes pour la flavor (trop complexe a priori)
define PROGRAM_template
 $(MOD_GEN_BIN)/$(1): $$(patsubst %.o, %.o, \
			$$(if $$($(1)-objs), $$($(1)-objs), $(1).o))
 $(MOD_GEN_BIN)/$(1): LDFLAGS += $$($(1)-ldflags)
 $(MOD_GEN_BIN)/$(1): $(MOD_STAMP_FILES) | flavor
endef
define OBJECT_template
 $(MOD_GEN_OBJ)/$(1).o: CFLAGS += $$($(1)-cflags)
 $(MOD_GEN_OBJ)/$(1).o: CPPFLAGS += $$($(1)-cppflags)
 $(MOD_GEN_OBJ)/$(1).o: CXXFLAGS += $$($(1)-cxxflags)
 $(MOD_GEN_OBJ)/$(1).o: $(MOD_STAMP_FILES) | flavor
endef

GF	:= $(shell $(PM2_CONFIG) --fc)
GFLAGS	:= $(shell $(PM2_CONFIG) --cflags)

$(MOD_GEN_OBJ)/%.o: %.f90
	$(GF) -c $(GFLAGS) $< -o $@

#$(foreach prog, $(APPS_LIST),$(warning $(call PROGRAM_template,$(prog))))
$(foreach prog, $(APPS_LIST),$(eval    $(call PROGRAM_template,$(prog))))
#$(foreach obj,  $(MOD_BASE), $(warning $(call  OBJECT_template,$(obj))))
$(foreach obj,  $(MOD_BASE), $(eval    $(call  OBJECT_template,$(obj))))

