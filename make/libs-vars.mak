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

# Variables communes pour produire du code
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/objs-vars.mak

# Type de cache à générer
#---------------------------------------------------------------------
PM2_GEN_MAK_OPTIONS += --type lib

# Nom de la librairie
#---------------------------------------------------------------------
LIBRARY ?= $(MODULE)
LIBNAME ?= $(LIBRARY)

# Noms des fichiers bibliothèques cibles
#---------------------------------------------------------------------
LIB_SO_MAJ?=0

LIB_LIB_A=$(MOD_GEN_LIB)/lib$(LIBNAME)$(LIB_EXT).a
LIB_LIB_SO=$(MOD_GEN_LIB)/lib$(LIBNAME)$(LIB_EXT).so
LIB_LIB_SO_MAJ=$(LIB_LIB_SO)$(addprefix .,$(LIB_SO_MAJ))
LIB_LIB_SO_MAJ_MIN=$(LIB_LIB_SO_MAJ)$(addprefix .,$(LIB_SO_MIN))

ifeq ($(BUILD_STATIC_LIBS),yes)
LIB_LIB += buildstatic
endif
ifeq ($(BUILD_DYNAMIC_LIBS),yes)
LIB_LIB += builddynamic
LIB_SO_MAP = $(wildcard scripts/arch-$(MOD_ARCH)/lib$(LIBNAME).map)
ifeq ($(LINK_OTHER_LIBS),yes)
LINK_LIB += linkdynamic
endif
endif


