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

all:

ifndef FLAVOR_DEFAULT
FLAVOR_DEFAULT := default
endif

# Pseudo module pour les applications/exemples
#---------------------------------------------------------------------
MODULE ?= $(strip $(if $(filter-out examples, $(notdir $(CURDIR))), \
			examples-$(notdir $(CURDIR)), \
			examples))
GEN_SUBDIR ?= $(strip $(if $(filter-out examples, $(notdir $(CURDIR))), \
			examples/$(notdir $(CURDIR)), \
			examples))

# R�pertoire(s) contenant les sources
#---------------------------------------------------------------------
MOD_SRC ?= .

# Variables communes pour produire du code
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/objs-vars.mak

# R�pertoire(s) contenant les sources
#---------------------------------------------------------------------
APPS_EXCLUDE += $(foreach file, $(MOD_BASE) $(APPS), \
			$(basename $($(file)-objs)))
APPS += $(foreach file, $(MOD_BASE), \
			$(if $($(file)-objs), $(file)))
APPS_LIST=$(if $(filter-out undefined, $(origin PROGS)), \
		$(PROGS), \
		$(sort $(APPS) $(filter-out $(APPS_EXCLUDE), $(MOD_BASE))))

export APP_DIR=$(CURDIR)

# Type de cache � g�n�rer
#---------------------------------------------------------------------
PM2_GEN_MAK_OPTIONS += --type apps --subdir $(GEN_SUBDIR)


#COMMON_DEPS += $(APP_STAMP_FLAVOR) $(APP_STAMP_FILES) $(MAKEFILE_FILE)

