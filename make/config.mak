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

# MAKEFILE_FILE -> nom par defaut des makefiles 
#---------------------------------------------------------------------
MAKEFILE_FILE := Makefile

# CC, AS, LD -> Commandes de construction
#---------------------------------------------------------------------
#CC := gcc #set with "pm2-config --cc"
AS := gcc # needed for some gcc specific flags
LD := gcc # needed for some gcc specific flags

LEX  :=  flex
YACC :=  bison -y -d --locations

# Controle du niveau d'affichage
#---------------------------------------------------------------------
ifdef VERB
MAK_VERB :=  $(VERB)
else
#MAK_VERB :=  verbose
#MAK_VERB :=  normal
MAK_VERB :=  quiet
#MAK_VERB :=  silent
endif

# Controle de la g�n�ration des d�pendances
#---------------------------------------------------------------------
# gcc-2.95 est incapable de g�n�rer les d�pendances � la vol�e :
# il faut d�commenter la ligne suivante
ifndef DEP_ON_FLY
ifneq (,$(shell $(CC) --version | grep '2\.95'))
DEP_ON_FLY:=false
endif
endif

