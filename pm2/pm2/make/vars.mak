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

ifdef _ALL_OPTIONS
_PM2_OPTIONS	:=	$(_ALL_OPTIONS)
_MARCEL_OPTIONS	:=	$(_ALL_OPTIONS)
_MAD_OPTIONS	:=	$(_ALL_OPTIONS)
_DSM_OPTIONS	:=	$(_ALL_OPTIONS)
endif

# Deux macros déclarées d'intérêt public :
EMPTY		:=
SPACE		:=	$(EMPTY) $(EMPTY)

# L'utilisation de _PM2_OPTIONS (en lieu et place de PM2_OPTIONS)
# force l'adjonction d'un suffixe aux fichiers créés (binaires,
# objets, bibliothèques)
ifdef _PM2_OPTIONS
PM2_OPTIONS		:=	$(_PM2_OPTIONS)
PM2_USE_EXTENSION	:=	yes
else
PM2_USE_EXTENSION	:=	no
endif

# Inclusion des options
PM2_OPT_FILES	:=	$(PM2_ROOT)/make/options.mak \
	$(foreach OPTION,$(PM2_OPTIONS),$(PM2_ROOT)/make/options-$(OPTION).mak)

include $(PM2_OPT_FILES)

COMMON_MAKEFILES	+=	$(PM2_OPT_FILES)

# Fichier de dépendance artificiel pour forcer la recompilation
# lorsque les options changent
ifdef PM2_OPTIONS
PM2_EXTRA_DEP_FILE	:=	$(PM2_ROOT)/.opt_$(subst $(SPACE),_,$(sort $(PM2_OPTIONS)))
else
PM2_EXTRA_DEP_FILE	:=	$(PM2_ROOT)/.opt
endif

# Si on étend les noms de fichiers, il faut calculer le suffixe, sinon
# il faut ajouter une dépendance supplémentaire...
ifeq ($(PM2_USE_EXTENSION),yes)
PM2_EXT		:=	-pm2_$(subst $(SPACE),_,$(sort $(PM2_OPTIONS)))
else
COMMON_MAKEFILES	+=	$(PM2_EXTRA_DEP_FILE)
endif

# Petite entorse a l'utilisation du '+=' ...
COMMON_EXT		:=	$(COMMON_EXT)$(PM2_EXT)

PM2_GEN_OPT	:=	$(PM2_GEN_1) $(PM2_GEN_2) $(PM2_GEN_3) \
			$(PM2_GEN_4) $(PM2_GEN_5) $(PM2_GEN_6)

PM2_OPT		:=	$(strip $(PM2_OPT_0) $(PM2_OPT_1) $(PM2_OPT_2) \
			$(PM2_OPT_3) $(PM2_OPT_4) $(PM2_OPT_5) \
			$(PM2_OPT_6) $(PM2_OPT_7) $(PM2_OPT_8))

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
PM2_LIB_A	=	$(PM2_LIBD)/libpm2$(COMMON_EXT).a
PM2_LIB_SO	=	$(PM2_LIBD)/libpm2$(COMMON_EXT).so

ifeq ($(PM2_LINK),dyn)
PM2_LIB		:=	$(PM2_LIB_SO)
else 
PM2_LIB		:=	$(PM2_LIB_A)
endif

COMMON_LIBS	:=	$(PM2_LIB)


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

