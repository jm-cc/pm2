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

# if someone do 'make' in a module directory, ...
default: no_goal

# Variables communes
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/common-vars.mak

# Repertoire source
#---------------------------------------------------------------------
MOD_SRC ?= source

# Inclusion du cache de configuration du module
#---------------------------------------------------------------------
DO_NOT_GENERATE_MAK_FILES+=__ _default_ _no_goal_
ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
-include $(PM2_MAK_DIR)/$(MODULE)-config.mak
endif

# Commandes de compilation et d'assemblage
#---------------------------------------------------------------------
# Note: y'a-t-il un controle de collision concernant ces variables ?
# Note: pas de LD := ...   Est-ce normal ?
ifneq ($($(MODULE)_CC),)
$(warning $(MODULE)_CC used. Please, report this behaviour)
CC := $($(MODULE)_CC)
endif

ifneq ($($(MODULE)_AS),)
$(warning $(MODULE)_AS used. Please, report this behaviour)
AS := $($(MODULE)_AS)
endif

ifneq ($($(MODULE)_LD),)
$(warning $(MODULE)_LD used. Please, report this behaviour)
LD := $($(MODULE)_LD)
endif

# Repertoires destination
#---------------------------------------------------------------------
MOD_REP_TO_BUILD := \
	$(MOD_GEN_STAMP)\
	$(MOD_GEN_DIR)\
	$(MOD_GEN_INC)\
	$(MOD_GEN_SRC)\
	$(MOD_GEN_CPP)\
	$(MOD_GEN_DEP)\
	$(MOD_GEN_ASM)\
	$(MOD_GEN_OBJ)\
	$(MOD_GEN_TMP)\
	$(MOD_GEN_LIB)\
	$(MOD_GEN_BIN)

# Sources
#---------------------------------------------------------------------
MOD_C_SOURCES ?= $(foreach rep, $(MOD_SRC), $(wildcard $(rep)/*.c))
MOD_S_SOURCES ?= $(foreach rep, $(MOD_SRC), $(wildcard $(rep)/*.S))
MOD_L_SOURCES ?= $(foreach rep, $(MOD_SRC), $(wildcard $(rep)/*.l))
MOD_Y_SOURCES ?= $(foreach rep, $(MOD_SRC), $(wildcard $(rep)/*.y))
MOD_HSPLITS_DIRS ?= $(wildcard h_splits)
MOD_HSPLITS_SOURCES ?= $(shell find $(MOD_HSPLITS_DIRS) \( -type d -name SCCS -prune \) -o -name *.h -printf "%P ")

# Bases : fichiers sans extension ni repertoire
#---------------------------------------------------------------------
MOD_L_BASE = $(foreach I, $(MOD_L_SOURCES), $(notdir $(basename $I)))
MOD_Y_BASE = $(foreach I, $(MOD_Y_SOURCES), $(notdir $(basename $I)))

# Generated sources
#---------------------------------------------------------------------
MOD_GEN_C_L_SOURCES = $(foreach I, $(MOD_L_BASE), $(MOD_GEN_SRC)/$I$(MOD_EXT).c)
MOD_GEN_C_Y_SOURCES = $(foreach I, $(MOD_Y_BASE), $(MOD_GEN_SRC)/$I$(MOD_EXT).c)
MOD_GEN_C_SOURCES   = $(MOD_GEN_C_L_SOURCES) $(MOD_GEN_C_Y_SOURCES)

# Generated includes
#---------------------------------------------------------------------
MOD_GEN_C_Y_INC = $(foreach I, $(MOD_Y_BASE), $(MOD_GEN_INC)/$I$(MOD_EXT).h)
MOD_GEN_C_INC   = $(MOD_GEN_C_Y_INC)

# Bases : fichiers sans extension ni repertoire (suite)
#---------------------------------------------------------------------
MOD_C_BASE = $(foreach I, $(MOD_GEN_C_SOURCES) $(MOD_C_SOURCES), \
			$(notdir $(basename $I)))
MOD_S_BASE = $(foreach I, $(MOD_S_SOURCES), $(notdir $(basename $I)))
MOD_BASE   = $(MOD_C_BASE) $(MOD_S_BASE)
# Les sources n'ont d�j� plus que l'�ventuel sous r�pertoire
MOD_HSPLITS_BASE = $(foreach I, $(MOD_HSPLITS_SOURCES), $(basename $I))

# Objets
#---------------------------------------------------------------------
MOD_C_OBJECTS = $(foreach I, $(MOD_C_BASE), $(MOD_GEN_OBJ)/$I$(MOD_EXT).o)
MOD_S_OBJECTS = $(foreach I, $(MOD_S_BASE), $(MOD_GEN_OBJ)/$I$(MOD_EXT).o)
MOD_OBJECTS   = $(MOD_C_OBJECTS) $(MOD_S_OBJECTS)

# PICS - librairies dynamiques
#---------------------------------------------------------------------
MOD_C_PICS =  $(foreach I, $(MOD_C_BASE), $(MOD_GEN_OBJ)/$I$(MOD_EXT).pic)
MOD_S_PICS =  $(foreach I, $(MOD_S_BASE), $(MOD_GEN_OBJ)/$I$(MOD_EXT).pic)
MOD_PICS   =  $(MOD_C_PICS) $(MOD_S_PICS)

# Preprocs
#---------------------------------------------------------------------
MOD_C_PREPROC = $(foreach I, $(MOD_C_BASE), $(MOD_GEN_CPP)/$I$(MOD_EXT).i)
MOD_S_PREPROC = $(foreach I, $(MOD_S_BASE), $(MOD_GEN_CPP)/$I$(MOD_EXT).si)
MOD_PREPROC   = $(MOD_C_PREPROC) $(MOD_S_PREPROC)

# FUT entries
#---------------------------------------------------------------------
# MOD_S_FUT : pas de sens
MOD_C_FUT = $(foreach I, $(MOD_C_BASE), $(MOD_GEN_CPP)/$I$(MOD_EXT).fut)
MOD_FUT   = $(MOD_C_FUT)

# H splited files
#---------------------------------------------------------------------
MOD_HSPLITS_PART_GEN=$(foreach f, $(MOD_HSPLITS_BASE), \
	$(MOD_GEN_INC)/$(f)_$(strip $(1)).h)
MOD_HSPLITS_DEFINES_GEN:=$(call MOD_HSPLITS_PART_GEN, defines)
MOD_HSPLITS_TYPES_GEN:=$(call MOD_HSPLITS_PART_GEN, types)
MOD_HSPLITS_DECLS_GEN:=$(call MOD_HSPLITS_PART_GEN, decls)
MOD_HSPLITS_INLINES_GEN:=$(call MOD_HSPLITS_PART_GEN, inlines)
MOD_HSPLITS_MAIN_GEN:=$(addprefix $(MOD_GEN_INC)/, $(MOD_HSPLITS_SOURCES))

MOD_HSPLITS_GEN:= $(foreach part, DEFINES TYPES DECLS INLINES MAIN EXTRA, \
			$(MOD_HSPLITS_$(part)_GEN))

# Dependances
#---------------------------------------------------------------------
MOD_C_DEPENDS   = $(strip $(patsubst %,$(MOD_GEN_DEP)/%.d, \
			$(notdir $(MOD_C_OBJECTS) $(MOD_C_PICS))))
MOD_S_DEPENDS   = $(strip $(patsubst %,$(MOD_GEN_DEP)/%.d, \
			$(notdir $(MOD_S_OBJECTS) $(MOD_S_PICS))))
MOD_I_DEPENDS   = $(strip $(patsubst %,$(MOD_GEN_DEP)/%.d, \
			$(notdir $(MOD_C_PREPROC))))
MOD_SI_DEPENDS   = $(strip $(patsubst %,$(MOD_GEN_DEP)/%.d, \
			$(notdir $(MOD_S_PREPROC))))
MOD_DEPENDS   = $(strip $(patsubst %,$(MOD_GEN_DEP)/%.d, \
			$(notdir $(MOD_OBJECTS) $(MOD_PICS) $(MOD_PREPROC))))
# Flags de compilation pour avoir les d�pendances au cours de la compilation
ifneq ($(DEP_ON_FLY),false)
CPPFLAGS += -MMD -MP -MF $(MOD_GEN_DEP)/$(notdir $@).d
#	-MT '$(MOD_GEN_DEP)/$(notdir $@).d'
endif
CC_DEPFLAGS = -MM

# "Convertisseurs" utiles
#---------------------------------------------------------------------
MOD_OBJ_TO_S   = $(MOD_GEN_ASM)/$(patsubst %.o,%.s,$(filter %.o,$(notdir $@)))
MOD_PIC_TO_S   = $(MOD_GEN_ASM)/$(patsubst %.pic,%.s,$(filter %.pic,$(notdir $@)))
MOD_GEN_C_TO_H = $(MOD_GEN_INC)/$(patsubst %.c,%.h,$(filter %.c,$(notdir $@)))
MOD_GEN_H_TO_C = $(MOD_GEN_SRC)/$(patsubst %.h,%.c,$(filter %.h,$(notdir $@)))


# Contribution aux dependances communes :
# - stamp
# - makefile
#---------------------------------------------------------------------
COMMON_DEPS += $(MOD_STAMP_FLAVOR) $(MAKEFILE_FILE) $(PM2_MAK_DIR)/$(MODULE)-config.mak

######################################################################
