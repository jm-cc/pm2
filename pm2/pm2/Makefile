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

MASTER_MODULE =  PM2

ifndef PM2_ROOT
$(error The PM2_ROOT variable is not set)
endif

include $(PM2_ROOT)/make/master_vars.mak

ifeq ($(LIGHT),yes)
COMMON_DEFAULT_TARGET :=  marcel_default mad_default
else
COMMON_DEFAULT_TARGET :=  pm2_default
endif

# Ce makefile
COMMON_MAKEFILES +=  $(MAD2_ROOT)/Makefile

# Inclusion des variables
ifneq ($(LIGHT),yes)
include $(PM2_MAKE)/vars.mak
include $(DSM_MAKE)/vars.mak
endif
include $(MARCEL_MAKE)/vars.mak
include $(MAD_MAKE)/vars.mak
include $(NTBX_MAKE)/vars.mak
include $(TBX_MAKE)/vars.mak

ifeq ($(LIGHT),yes)
COMMON_OPTIONS += LIGHT $(LIGHT)
endif

# Regles principales
include $(PM2_ROOT)/make/master_rules.mak

# Inclusion des regles de compilation
ifneq ($(LIGHT),yes)
include $(PM2_MAKE)/rules.mak
include $(DSM_MAKE)/rules.mak
endif
include $(MARCEL_MAKE)/rules.mak
include $(MAD_MAKE)/rules.mak
include $(NTBX_MAKE)/rules.mak
include $(TBX_MAKE)/rules.mak


##############################################################################
