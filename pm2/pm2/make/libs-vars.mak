

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

# if someone do 'make' in an module directory, ...
default: no_goal

# Variables communes
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/common-vars.mak

# Nom de la librairie
#---------------------------------------------------------------------
# Note dans quel cas est-ce necessaire ?
# Tous sauf mad1 et mad2
ifndef LIBNAME
LIBNAME := $(LIBRARY)
endif

# Repertoire source
#---------------------------------------------------------------------
LIB_SRC := source

# Inclusion du cache de configuration de la librairie
#---------------------------------------------------------------------
ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
-include $(PM2_MAK_DIR)/$(LIBRARY)-config.mak
endif

# Commandes de compilation et d'assemblage
#---------------------------------------------------------------------
# Note: y'a-t-il un controle de collision concernant ces variables ?
# Note: pas de LD := ...   Est-ce normal ?
ifneq ($($(LIBRARY)_CC),)
CC := $($(LIBRARY)_CC)
endif

ifneq ($($(LIBRARY)_AS),)
AS := $($(LIBRARY)_AS)
endif

# Repertoires destination
#---------------------------------------------------------------------
LIB_REP_TO_BUILD := \
	$(LIB_GEN_STAMP)\
	$(LIB_GEN_DIR)\
	$(LIB_GEN_INC)\
	$(LIB_GEN_SRC)\
	$(LIB_GEN_CPP)\
	$(LIB_GEN_DEP)\
	$(LIB_GEN_ASM)\
	$(LIB_GEN_OBJ)\
	$(LIB_GEN_TMP)\
	$(LIB_GEN_LIB)

# Noms des fichiers biblioth�ques cibles
#---------------------------------------------------------------------
LIB_LIB_A=$(LIB_GEN_LIB)/lib$(LIBNAME)$(LIB_EXT).a
LIB_LIB_SO=$(LIB_GEN_LIB)/lib$(LIBNAME)$(LIB_EXT).so
ifeq ($(BUILD_STATIC_LIBS),true)
LIB_LIB += $(LIB_LIB_A)
endif
ifeq ($(BUILD_DYNAMIC_LIBS),true)
LIB_LIB += $(LIB_LIB_SO)
endif

# Sources
#---------------------------------------------------------------------
LIB_C_SOURCES :=  $(wildcard $(LIB_SRC)/*.c)
LIB_S_SOURCES :=  $(wildcard $(LIB_SRC)/*.S)

# Bases : fichiers sans extension ni repertoire
#---------------------------------------------------------------------
LIB_C_BASE := $(foreach I, $(LIB_C_SOURCES), $(notdir $(basename $I)))
LIB_S_BASE := $(foreach I, $(LIB_S_SOURCES), $(notdir $(basename $I)))
LIB_BASE   := $(LIB_C_BASE) $(LIB_S_BASE)

# Objets
#---------------------------------------------------------------------
LIB_C_OBJECTS := $(foreach I, $(LIB_C_BASE), $(LIB_GEN_OBJ)/$I$(LIB_EXT).o)
LIB_S_OBJECTS := $(foreach I, $(LIB_S_BASE), $(LIB_GEN_OBJ)/$I$(LIB_EXT).o)
LIB_OBJECTS   := $(LIB_C_OBJECTS) $(LIB_S_OBJECTS)

# PICS - librairies dynamiques
#---------------------------------------------------------------------
LIB_C_PICS :=  $(foreach I, $(LIB_C_BASE), $(LIB_GEN_OBJ)/$I$(LIB_EXT).pic)
LIB_S_PICS :=  $(foreach I, $(LIB_S_BASE), $(LIB_GEN_OBJ)/$I$(LIB_EXT).pic)
LIB_PICS   :=  $(LIB_C_PICS) $(LIB_S_PICS)

# Preprocs
#---------------------------------------------------------------------
LIB_C_PREPROC := $(foreach I, $(LIB_C_BASE), $(LIB_GEN_CPP)/$I$(LIB_EXT).i)
LIB_S_PREPROC := $(foreach I, $(LIB_S_BASE), $(LIB_GEN_CPP)/$I$(LIB_EXT).si)
LIB_PREPROC   := $(LIB_C_PREPROC) $(LIB_S_PREPROC)

# FUT entries
#---------------------------------------------------------------------
# LIB_S_FUT : pas de sens
LIB_C_FUT := $(foreach I, $(LIB_C_BASE), $(LIB_GEN_CPP)/$I$(LIB_EXT).fut)
LIB_FUT   := $(LIB_C_FUT)

# Dependances
#---------------------------------------------------------------------
LIB_C_DEPENDS := $(foreach I, $(LIB_C_BASE), $(LIB_GEN_DEP)/$I$(LIB_EXT).d)
LIB_S_DEPENDS := $(foreach I, $(LIB_S_BASE), $(LIB_GEN_DEP)/$I$(LIB_EXT).d)
LIB_DEPENDS   := $(strip $(LIB_C_DEPENDS) $(LIB_S_DEPENDS))

# "Convertisseurs" utiles
#---------------------------------------------------------------------
LIB_DEP_TO_OBJ = $(LIB_GEN_OBJ)/$(patsubst %.d,%.o,$(notdir $@))
LIB_OBJ_TO_S   = $(LIB_GEN_ASM)/$(patsubst %.o,%.s,$(notdir $@))
LIB_PIC_TO_S   = $(LIB_GEN_ASM)/$(patsubst %.pic,%.s,$(notdir $@))

# Contribution aux dependances communes :
# - stamp
# - makefile
#---------------------------------------------------------------------
COMMON_DEPS += $(LIB_STAMP_FLAVOR) $(MAKEFILE_FILE) 

######################################################################
