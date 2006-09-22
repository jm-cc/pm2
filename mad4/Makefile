

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
MOD_SRC := $(PM2_ROOT)/nmad/core/src
include $(PM2_ROOT)/make/libs-vars.mak

# Inclusion du cache specifique a madeleine
#---------------------------------------------------------------------
ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
-include $(PM2_MAK_DIR)/mad4-specific.mak
endif

# Gestion du scheduler : prise en compte de fichiers sources supplémentaires
MOD_SRC += $(foreach SCHED, $(MAD4_SCHEDULERS), $(PM2_ROOT)/nmad/schedulers/$(SCHED)/src)

# Gestion des drivers : prise en compte des fichiers sources supplémentaires
#---------------------------------------------------------------------
MOD_SRC += $(foreach PROT, $(MAD4_PROTOCOLS), $(PM2_ROOT)/nmad/drivers/$(PROT)/src)

# Regles communes aux librairies
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/libs-rules.mak

# Construction du cache specifique a Madeleine IV
#---------------------------------------------------------------------
$(PM2_MAK_DIR)/mad4-specific.mak: $(MOD_STAMP_FLAVOR)
	$(COMMON_HIDE)_PM2_OUTPUT=`$(PM2_CONFIG) --protocols $(MODULE)`;\
	if [ -n "$$_PM2_OUTPUT" ]; then mkdir -p `dirname $@`; \
	echo "MAD4_PROTOCOLS := $$_PM2_OUTPUT" > $@ ;\
	fi;
	$(COMMON_HIDE)_PM2_OUTPUT=`$(PM2_CONFIG) --schedulers $(MODULE)`;\
	echo "MAD4_SCHEDULERS := $$_PM2_OUTPUT" >> $@ ;