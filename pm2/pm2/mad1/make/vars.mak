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

MAD1 :=  yes
COMMON_MAKEFILES +=  $(MAD_MAKE)/vars.mak
COMMON_MAKEFILES +=  $(MAD_MAKE)/rules.mak

# Inclusion des options specifiques au protocole reseau
include $(MAD_MAKE)/custom/options.mak

COMMON_MAKEFILES	+=	$(MAD_MAKE)/custom/options.mak

MAD_SRC		:=	$(MAD1_ROOT)/source
MAD_NET_SRC	:=	$(MAD_SRC)/$(NET_INTERF)
MAD_INC		:=	$(MAD1_ROOT)/include

MAD_OPT :=
MAD_GEN :=

ifndef MAD_OPTIONS
MAD_OPTIONS :=  default
endif

MAD_OPT_FILES :=  \
   $(foreach OPTION,$(MAD_OPTIONS),$(MAD_MAKE)/options/$(OPTION).mak)
COMMON_MAKEFILES +=  $(MAD_OPT_FILES)
include $(MAD_OPT_FILES)

COMMON_OPTIONS += MAD1 $(MAD_OPTIONS)

MAD_LIB_A  :=  $(GEN_LIB)/libmad.a
MAD_LIB_SO :=  $(GEN_LIB)/libmad.so

ifndef MAD_LINK
MAD_LINK :=  $(COMMON_LINK)
endif

ifeq ($(MAD_LINK),dyn)
MAD_LIB :=  $(MAD_LIB_SO)
else 
MAD_LIB :=  $(MAD_LIB_A)
endif

COMMON_CFLAGS  +=  -DMAD1 $(MAD_OPT) -I$(MAD_INC) $(NET_CFLAGS)
COMMON_LDFLAGS +=  $(NET_LFLAGS) 
COMMON_LLIBS   +=  $(MAD_LIB) $(NET_LLIBS)


