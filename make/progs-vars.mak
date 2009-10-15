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

# Variables communes pour produire du code
#---------------------------------------------------------------------
include $(PM2_SRCROOT)/make/objs-vars.mak

# Type de cache à générer
#---------------------------------------------------------------------
PM2_GEN_MAK_OPTIONS += --type prog

# Nom du programme
#---------------------------------------------------------------------
PROGRAM ?= $(MODULE)
PROGNAME ?= $(PROGRAM)

MOD_PRG := $(MOD_GEN_BIN)/$(PROGNAME)

PRG_PRG := $(MOD_PRG)



