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

PM2	:=	yes

COMMON_MAKEFILES	+=	$(PM2_ROOT)/make/vars.mak \
				$(PM2_ROOT)/make/rules.mak

ifndef PM2_ARCH_SYS
PM2_ARCH	:=	$(shell $(PM2_ROOT)/bin/pm2_arch)
PM2_SYS		:=	$(shell $(PM2_ROOT)/bin/pm2_sys)
PM2_ARCH_SYS	:=	$(shell basename $(PM2_SYS) _SYS)/$(shell basename $(PM2_ARCH) _ARCH)
endif

COMMON_CFLAGS	+=	-D$(PM2_ARCH) -D$(PM2_SYS) -DPM2=\"$(PM2_ARCH_SYM)\"

PM2_ARCH_SYM	:=	$(subst /,_,$(PM2_ARCH_SYS))

ifndef MARCEL_ROOT
MARCEL_ROOT	:=	$(PM2_ROOT)/marcel
endif

ifndef MAD1_ROOT
MAD1_ROOT	:=	$(PM2_ROOT)/mad1
endif

ifndef MAD2_ROOT
MAD2_ROOT	:=	$(PM2_ROOT)/mad2
endif

ifndef DSM_ROOT
DSM_ROOT	:=	$(PM2_ROOT)/dsm
endif

ifdef ALL_OPTIONS
PM2_OPTIONS	:=	$(ALL_OPTIONS)
MARCEL_OPTIONS	:=	$(ALL_OPTIONS)
MAD_OPTIONS	:=	$(ALL_OPTIONS)
DSM_OPTIONS	:=	$(ALL_OPTIONS)
endif

# Si PM2_OPTIONS n'est pas specifie en parametre de Make, alors on
# prend la version specifiee dans le fichier d'options...
ifndef PM2_OPTIONS
PM2_USE_OPT_FILE	:=	no
PM2_OPT_FILE		:=	$(PM2_ROOT)/make/options.mak
else
PM2_USE_OPT_FILE	:=	yes
PM2_OPT_FILE		:=	$(PM2_ROOT)/make/options-$(PM2_OPTIONS).mak
endif

COMMON_MAKEFILES	+=	$(PM2_OPT_FILE)

# Verification de l'existence du fichier d'options
ifneq ($(PM2_OPT_FILE),$(wildcard $(PM2_OPT_FILE)))
$(error $(PM2_OPT_FILE): No such file)
else
# Inclusion du fichier de parametrage de PM2
include $(PM2_OPT_FILE)
endif

# Petite entorse a l'utilisation du '+=' ...
ifeq ($(PM2_USE_OPT_FILE),yes)
COMMON_EXT		:=	$(strip $(COMMON_EXT)$(PM2_EXT))
endif

# Les options "generiques" (-g, etc.) sont dans PM2_GEN_OPT, et les
# autres sont dans PM2_OPT...
ifeq ($(COMMON_GEN_CFLAGS),)
COMMON_GEN_CFLAGS	:=	$(strip $(PM2_GEN_OPT))
endif
COMMON_CFLAGS		+=	$(PM2_OPT) -I$(PM2_INC)

# Repertoires principaux de PM2
PM2_SRC		:=	$(PM2_ROOT)/source
PM2_DEP		:=	$(PM2_SRC)/depend
PM2_OBJ		:=	$(PM2_SRC)/obj/$(PM2_ARCH_SYS)
PM2_INC		:=	$(PM2_ROOT)/include
PM2_LIBD	:=	$(PM2_ROOT)/lib/$(PM2_ARCH_SYS)

# Creation des repertoires necessaires "a la demande"
ifneq ($(MAKECMDGOALS),distclean) # avoid auto-building of directories
ifneq ($(wildcard $(PM2_DEP)),$(PM2_DEP))
DUMMY_BUILD	:=	$(shell mkdir -p $(PM2_DEP))
endif

ifneq ($(wildcard $(PM2_OBJ)),$(PM2_OBJ))
DUMMY_BUILD	:=	$(shell mkdir -p $(PM2_OBJ))
endif

ifneq ($(wildcard $(PM2_LIBD)),$(PM2_LIBD))
DUMMY_BUILD	:=	$(shell mkdir -p $(PM2_LIBD))
endif
endif

# Obligatoire et inamovible ;-)
PM2_CC		:=	gcc
PM2_AS		:=	$(PM2_CC)

# CFLAGS et LDFLAGS: attention a ne pas utiliser ':=' !
PM2_CFLAGS	=	$(COMMON_GEN_CFLAGS) $(COMMON_CFLAGS)
PM2_KCFLAGS	=	$(PM2_CFLAGS) -DPM2_KERNEL
PM2_LDFLAGS	=	$(COMMON_LDFLAGS)
PM2_ASFLAGS	:=	-D$(PM2_ARCH) -D$(PM2_SYS)

# Bibliotheques
PM2_LIB		=	$(PM2_LIBD)/libpm2$(COMMON_EXT).a
COMMON_LIBS	+=	$(PM2_LIB)


COMMON_DEFAULT_TARGET	+=	pm2_default
COMMON_CLEAN_TARGET	+=	pm2clean
COMMON_DISTCLEAN_TARGET	+=	pm2distclean

# Choix madeleine I/II
include $(PM2_ROOT)/make/mad.mak
COMMON_MAKEFILES	+=	$(PM2_ROOT)/make/mad.mak

# Marcel
include $(MARCEL_ROOT)/make/vars.mak

# Madeleine
ifeq ($(MAD2),yes)
$(error Not yet implemented)
else
include $(MAD1_ROOT)/make/vars.mak
endif

# DSM/PM2
ifeq ($(PM2_USE_DSM),yes)
include $(DSM_ROOT)/make/vars.mak
endif


######################## Applications PM2 ########################

ifdef SRC_DIR

ifeq ($(SRC_DIR),.)
SRC_DIR		:=	./
endif

COMMON_MAKEFILES	+=	$(SRC_DIR)/Makefile

# Repertoires applicatifs
DEP_DIR	=	$(SRC_DIR)/depend
OBJ_DIR	=	$(SRC_DIR)/obj/$(PM2_ARCH_SYS)
ifndef BIN_DIR
BIN_DIR	=	$(SRC_DIR)/bin/$(PM2_ARCH_SYS)
endif

# Creation des repertoires necessaires "a la demande"
ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(wildcard $(DEP_DIR)),$(DEP_DIR))
DUMMY_BUILD	:=	$(shell mkdir -p $(DEP_DIR))
endif

ifneq ($(wildcard $(OBJ_DIR)),$(OBJ_DIR))
DUMMY_BUILD	:=	$(shell mkdir -p $(OBJ_DIR))
endif

ifneq ($(wildcard $(BIN_DIR)),$(BIN_DIR))
DUMMY_BUILD	:=	$(shell mkdir -p $(BIN_DIR))
endif
endif


endif

