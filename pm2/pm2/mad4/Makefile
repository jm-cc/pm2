

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

# LIBNAME -> nom de la librairie
#---------------------------------------------------------------------
LIBNAME=mad

# Inclusion des makefiles generiques
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/libs-vars.mak

# Inclusion du cache specifique a madeleine
#---------------------------------------------------------------------
ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
-include $(PM2_MAK_DIR)/mad3-specific.mak
endif


# Gestion du reste : prise en compte des fichiers sources supplémentaires
#---------------------------------------------------------------------
MOD_C_SOURCES += $(wildcard $(MOD_SRC)/core/*.c)

vpath %.c $(MOD_SRC)/core

# Gestion des drivers : prise en compte des fichiers sources supplémentaires
#---------------------------------------------------------------------
MOD_SRC2=$(MOD_SRC)/network
MOD_C_SOURCES += $(foreach PROT, $(MAD3_PROTOCOLS), $(MOD_SRC2)/mad_$(PROT).c)

# Le vpath supplémentaire
#---------------------------------------------------------------------
vpath %.c $(MOD_SRC2)


# Gestion des factory : prise en compte des fichiers sources supplémentaires
#---------------------------------------------------------------------
MOD_C_SOURCES += $(wildcard $(MOD_SRC)/mad3/*.c)

vpath %.c $(MOD_SRC)/mad3

# Gestion des services : prise en compte des fichiers sources supplémentaires
#---------------------------------------------------------------------
MOD_C_SOURCES += $(wildcard $(MOD_SRC)/services/*.c)

vpath %.c $(MOD_SRC)/services



# Regles communes aux librairies
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/libs-rules.mak

# Construction du cache specifique a Madeleine III
#---------------------------------------------------------------------
$(PM2_MAK_DIR)/mad3-specific.mak: $(MOD_STAMP_FLAVOR)
	$(COMMON_HIDE)_PM2_OUTPUT=`$(PM2_CONFIG) --protocols $(MODULE)`;\
	if [ -n "$$_PM2_OUTPUT" ]; then mkdir -p `dirname $@`; \
	echo "MAD3_PROTOCOLS := $$_PM2_OUTPUT" > $@ ;\
	fi;

