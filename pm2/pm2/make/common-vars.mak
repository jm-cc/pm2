#
#                      PM2 HIGH-PERF/ISOMALLOC
#           High Performance Parallel Multithreaded Machine
#                           version 3.0
#
#     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
#       Christian Perez, Jean-Francois Mehaut, Raymond Namyst
#
#            Laboratoire de l'Informatique du Parallelisme
#                        UMR 5668 CNRS-INRIA
#                 Ecole Normale Superieure de Lyon
#
#                      External Contributors:
#                 Yves Denneulin (LMC - Grenoble),
#                 Benoit Planquelle (LIFL - Lille)
#
#                    1998 All Rights Reserved
#
#
#                             NOTICE
#
# Permission to use, copy, modify, and distribute this software and
# its documentation for any purpose and without fee is hereby granted
# provided that the above copyright notice appear in all copies and
# that both the copyright notice and this permission notice appear in
# supporting documentation.
#
# Neither the institutions (Ecole Normale Superieure de Lyon,
# Laboratoire de L'informatique du Parallelisme, Universite des
# Sciences et Technologies de Lille, Laboratoire d'Informatique
# Fondamentale de Lille), nor the Authors make any representations
# about the suitability of this software for any purpose. This
# software is provided ``as is'' without express or implied warranty.
#

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
PM2_CONFIG := $(PM2_ROOT)/bin/pm2-config
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
DO_NOT_GENERATE_MAK_FILES += _distclean_ _distcleanlibs_ _distcleanflavors_
DO_NOT_GENERATE_MAK_FILES += _distcleandoc_ _distcleanall_
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

