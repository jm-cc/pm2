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

PM2 :=  yes
COMMON_MAKEFILES +=  $(PM2_MAKE)/vars.mak
COMMON_MAKEFILES +=  $(PM2_MAKE)/rules.mak

PM2_SRC :=  $(PM2_ROOT)/source
PM2_INC :=  $(PM2_ROOT)/include
PM2_GEN_OBJ :=  $(GEN_OBJ)/pm2
PM2_GEN_ASM :=  $(GEN_ASM)/pm2
PM2_GEN_DEP :=  $(GEN_DEP)/pm2

PM2_OPT :=

ifndef PM2_OPTIONS
PM2_OPTIONS :=  default
endif

PM2_OPT_FILES :=  \
   $(foreach OPTION,$(PM2_OPTIONS),$(PM2_MAKE)/options/$(OPTION).mak)
COMMON_MAKEFILES +=  $(PM2_OPT_FILES)
include $(PM2_OPT_FILES)

COMMON_OPTIONS += PM2 $(PM2_OPTIONS)

PM2_LIB_A  :=  $(GEN_LIB)/libpm2.a
PM2_LIB_SO :=  $(GEN_LIB)/libpm2.so

ifndef PM2_LINK
PM2_LINK :=  $(COMMON_LINK)
endif

ifeq ($(PM2_LINK),dyn)
PM2_LIB :=  $(PM2_LIB_SO)
else 
PM2_LIB :=  $(PM2_LIB_A)
endif

COMMON_CFLAGS  +=  -DPM2 $(PM2_OPT) -I$(PM2_INC)
COMMON_LDFLAGS += 
COMMON_LLIBS   +=  $(PM2_LIB) 

