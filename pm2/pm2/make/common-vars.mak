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

# Pr�f�rences globales
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/config.mak

# MODULE -> module en cours de compilation
#---------------------------------------------------------------------
MODULE ?= $(notdir $(CURDIR))
#$(if $(filter-out examples, $(notdir $(CURDIR))), \
#		$(notdir $(CURDIR)), \
#		$(notdir $(shell dirname $(CURDIR))))

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
# Pr�r�glages standard

#SHOW_FLAGS=true
SHOW_FLAVOR=true

ifeq ($(MAK_VERB),verbose)
SHOW_TARGET   := true
SHOW_DEPEND   := true
SHOW_MAIN     := true
SHOW_HIDE     := true
SHOW_CLEAN    := true
SHOW_DIRECTORY:= true
else
ifeq ($(MAK_VERB),normal)
SHOW_TARGET   := true
SHOW_DEPEND   := false
SHOW_MAIN     := true
SHOW_HIDE     := false
SHOW_CLEAN    := true
SHOW_DIRECTORY:= false
else
ifeq ($(MAK_VERB),quiet)
SHOW_TARGET   := true
SHOW_DEPEND   := false
SHOW_MAIN     := false
SHOW_HIDE     := false
SHOW_CLEAN    := true
SHOW_DIRECTORY:= false
else
ifeq ($(MAK_VERB),depend)
SHOW_TARGET   := true
SHOW_DEPEND   := true
SHOW_MAIN     := false
SHOW_HIDE     := false
SHOW_CLEAN    := true
SHOW_DIRECTORY:= false
else  # silent
SHOW_TARGET   := false
SHOW_DEPEND   := false
SHOW_MAIN     := false
SHOW_HIDE     := false
SHOW_CLEAN    := false
SHOW_DIRECTORY:= false
endif
endif
endif
endif
# Constructions des messages en fonctions des variables SHOW_
ifeq ($(SHOW_TARGET),true)
ifeq ($(SHOW_DEPEND),true)
show-depend    = $(EMPTY) due to:$(foreach file,$?,\n      * $(file))
endif
show-target    = @ printf "%s $(@F)$(show-depend)\n" $1;:
else
show-target    = @:
endif
COMMON_MAKE    = $(call show-target, "  Generating Makefile")
COMMON_BUILD   = $(call show-target, "    building")
COMMON_LINK    = $(call show-target, "    linking")
ifeq ($(SHOW_MAIN),true)
COMMON_MAIN    = #
else
COMMON_MAIN    = @
endif
ifeq ($(SHOW_HIDE),true)
COMMON_HIDE    = #
else
COMMON_HIDE    = @
endif
ifeq ($(SHOW_CLEAN),true)
COMMON_CLEAN   = #
else
COMMON_CLEAN   = @
endif
ifneq ($(SHOW_DIRECTORY),true)
# Do not print directories enter/exit
MAKEFLAGS += --no-print-directory
endif

# COMMON_DEPS -> dependances racines
# - config.mak contient les reglages du processus de contruction
#---------------------------------------------------------------------
COMMON_DEPS += $(PM2_ROOT)/make/config.mak

# PM2_CONFIG -> script d'information de configuration de PM2
#---------------------------------------------------------------------
ifndef PM2_CONFIG
PM2_CONFIG := PM2_ROOT=$(PM2_ROOT) $(PM2_ROOT)/bin/pm2-config
endif
override PM2_CONFIG := $(PM2_CONFIG) --flavor=$(FLAVOR)

# PM2_GEN_MAK -> script de g�n�ration des makefiles "acc�l�rateurs"
#---------------------------------------------------------------------
PM2_GEN_MAK := $(PM2_ROOT)/bin/pm2-gen-make.sh --silent

# GOALS pour lesquels on ne g�n�rera ni n'incluera les .mak
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

