

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

# LIBRARY -> nom du module
#---------------------------------------------------------------------
LIBRARY=mad2

# LIBNAME -> nom de la librairie
#---------------------------------------------------------------------
LIBNAME=mad

# Variables communes (nécessaire pour avoir $(PM2_MAK_DIR))
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/common-vars.mak

# Inclusion du cache specifique a madeleine
#---------------------------------------------------------------------
ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
-include $(PM2_MAK_DIR)/mad2-specific.mak
endif

# Gestion des drivers : prise en compte des fichiers sources supplémentaires
# Attention : utilisation de '=' (et pas ':=') car $(LIB_SRC) n'est
# pas encore fixé
#---------------------------------------------------------------------
LIB_SRC2=$(LIB_SRC)/connection_interfaces
LIB_C_SOURCES += $(foreach PROT, $(MAD2_PROTOCOLS), $(LIB_SRC2)/mad_$(PROT).c)

# Inclusion des makefiles generiques
# Regles communes aux librairies
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/libs.mak

# Le vpath supplémentaire maintenant que $(LIB_SRC) (et donc $(LIB_SRC2))
# a la bonne valeur
#---------------------------------------------------------------------
vpath %.c $(LIB_SRC2)

# Inclusion du makefile de dependance des drivers
#---------------------------------------------------------------------
ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
ifneq ($(wildcard $(LIB_C_DEPENDS_ADDED)),)
-include $(LIB_C_DEPENDS_ADDED)
endif
endif

# Construction du cache specifique a Madeleine II
#---------------------------------------------------------------------
$(PM2_MAK_DIR)/mad2-specific.mak: $(LIB_STAMP_FLAVOR)
	@_PM2_OUTPUT=`$(PM2_CONFIG) --protocols $(LIBRARY)`;\
	if [ -n "$$_PM2_OUTPUT" ]; then mkdir -p `dirname $@`; \
	echo "MAD2_PROTOCOLS := $$_PM2_OUTPUT" > $@ ;\
	fi;

