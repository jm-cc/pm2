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

ifndef FLAVOR
ifdef PM2_FLAVOR
export FLAVOR:=$(PM2_FLAVOR)
else
ifdef FLAVOR_DEFAULT
export FLAVOR:=$(FLAVOR_DEFAULT)
else
export FLAVOR:=default
endif
endif
endif

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

LIB_PREFIX = $(COMMON_PREFIX)
PRG_PREFIX = $(COMMON_PREFIX)

COMMON_DEPS += $(PM2_ROOT)/make/config.mak

ifndef PM2_CONFIG
PM2_CONFIG := $(PM2_ROOT)/bin/pm2-config
endif

ifdef FLAVOR
override PM2_CONFIG := $(PM2_CONFIG) --flavor=$(FLAVOR)
endif

