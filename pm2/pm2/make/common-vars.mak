

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

# Préférences globales
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/config.mak

# FLAVOR -> flavor utilisee pour la compilation
#---------------------------------------------------------------------
ifndef FLAVOR
  ifdef PM2_FLAVOR
    export FLAVOR:=$(PM2_FLAVOR)
  else # PM2_FLAVOR
    ifdef FLAVOR_DEFAULT
      export FLAVOR:=$(FLAVOR_DEFAULT)
    else # FLAVOR_DEFAULT
      export FLAVOR:=default
    endif # FLAVOR_DEFAULT
  endif # PM2_FLAVOR
endif # FLAVOR

# MAK_VERB -> niveau d'affichage des messages
#---------------------------------------------------------------------
ifeq ($(MAK_VERB),verbose)
COMMON_PREFIX  =#
COMMON_HIDE   :=#
COMMON_CLEAN  :=#	
else
ifeq ($(MAK_VERB),normal)
COMMON_PREFIX  =#
COMMON_HIDE   :=  @
COMMON_CLEAN  :=#	
else
ifeq ($(MAK_VERB),quiet)
COMMON_PREFIX  =  @ echo "building " $(@F) ;
COMMON_HIDE   :=  @
COMMON_CLEAN  :=#	
else  # silent
COMMON_PREFIX  =  @
COMMON_HIDE   :=  @
COMMON_CLEAN  :=  @
endif
endif
endif

# Prefixes -> utilite ?
#---------------------------------------------------------------------
LIB_PREFIX = $(COMMON_PREFIX)
PRG_PREFIX = $(COMMON_PREFIX)

# COMMON_DEPS -> dependances racines
# - config.mak contient les reglages du processus de contruction
#---------------------------------------------------------------------
COMMON_DEPS += $(PM2_ROOT)/make/config.mak

# PM2_CONFIG -> script d'information de configuration de PM2
#---------------------------------------------------------------------
ifndef PM2_CONFIG
PM2_CONFIG := PM2_ROOT=$(PM2_ROOT) $(PM2_ROOT)/bin/pm2-config
endif

# Utilite de cette commande ? 
# Dans quel cas, FLAVOR n'est pas definie ?
#---------------------------------------------------------------------
ifdef FLAVOR
override PM2_CONFIG := $(PM2_CONFIG) --flavor=$(FLAVOR)
endif

# PM2_GEN_MAK -> script de génération des makefiles "accélérateurs"
#---------------------------------------------------------------------
PM2_GEN_MAK := $(PM2_ROOT)/bin/pm2-gen-make.sh $(FLAVOR)

# GOALS pour lesquels on ne générera ni n'incluera les .mak
#---------------------------------------------------------------------
ifndef DO_NOT_GENERATE_MAK_FILES
DO_NOT_GENERATE_MAK_FILES := 
endif
DO_NOT_GENERATE_MAK_FILES += _sos_
DO_NOT_GENERATE_MAK_FILES += _checkflavor_
DO_NOT_GENERATE_MAK_FILES += _clean_ _cleanall_ _cleantools_ _cleantoolsall_
DO_NOT_GENERATE_MAK_FILES += _distclean_ _distcleanflavors_ _distcleandoc_
DO_NOT_GENERATE_MAK_FILES += _refresh_ _refreshall_
DO_NOT_GENERATE_MAK_FILES += _init_ _checkmake_ _cvsinit_ _flavorinit_
DO_NOT_GENERATE_MAK_FILES += _doc_
DO_NOT_GENERATE_MAK_FILES += _help_
DO_NOT_GENERATE_MAK_FILES += _config_ _textconfig_ _menuconfig_
DO_NOT_GENERATE_MAK_FILES += _xdialogconfig_ _xconfig_

# PM2_MAK_DIR -> repertoire destination des makefiles generes
#---------------------------------------------------------------------
# Set variable if not set
ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
ifeq ($(PM2_MAK_DIR),)
export PM2_MAK_DIR := $(shell $(PM2_CONFIG) --makdir)
endif
endif

