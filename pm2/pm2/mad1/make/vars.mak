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

MAD1	:=	yes

COMMON_MAKEFILES	+=	$(MAD1_ROOT)/make/vars.mak \
				$(MAD1_ROOT)/make/rules.mak

ifndef PM2_ARCH_SYS
PM2_ARCH	:=	$(shell $(PM2_ROOT)/bin/pm2_arch)
PM2_SYS		:=	$(shell $(PM2_ROOT)/bin/pm2_sys)
PM2_ARCH_SYS	:=	$(shell basename $(PM2_SYS) _SYS)/$(shell basename $(PM2_ARCH) _ARCH)
COMMON_CFLAGS	+=	-D$(PM2_SYS) -D$(PM2_ARCH)
endif

# Si MAD_OPTIONS n'est pas specifie en parametre de Make, alors on
# prend la version specifiee dans le fichier d'options...
ifndef MAD_OPTIONS
MAD_USE_OPT_FILE	:=	no
MAD_OPT_FILE		:=	$(MAD1_ROOT)/make/options.mak
else
MAD_USE_OPT_FILE	:=	yes
MAD_OPT_FILE		:=	$(MAD1_ROOT)/make/options-$(MAD_OPTIONS).mak
endif

COMMON_MAKEFILES	+=	$(MAD_OPT_FILE)

# Verification de l'existence du fichier d'options
ifneq ($(MAD_OPT_FILE),$(wildcard $(MAD_OPT_FILE)))
$(error $(MAD_OPT_FILE): No such file)
else
# Inclusion du fichier de parametrage de Marcel
include $(MAD_OPT_FILE)
endif

# Inclusion des options specifiques au protocole reseau
include $(MAD1_ROOT)/make/custom/options.mak

COMMON_MAKEFILES	+=	$(MAD1_ROOT)/make/custom/options.mak

# Inclusion des options specifiques a l'architecture
include $(MAD1_ROOT)/make/archdep/$(shell basename $(PM2_SYS) _SYS).inc

# Petite entorse a l'utilisation du '+=' ...
ifeq ($(MAD_USE_OPT_FILE),yes)
COMMON_EXT		:=	$(strip $(COMMON_EXT)$(MAD_EXT))
endif

# TODO: C'est un peu obsolete ! Il faudrait utiliser un moyen plus
# generique pour detecter si les "flags" de compilation ont change
# depuis la derniere fois !
ifdef PM2
COMMON_MAKEFILES	+=	$(MAD1_ROOT)/.mad_pm2
else
COMMON_MAKEFILES	+=	$(MAD1_ROOT)/.mad_standalone
endif

# Les options "generiques" (-g, etc.) sont dans MAD_GEN_OPT, et les
# autres sont dans MAD_OPT...
ifeq ($(COMMON_GEN_CFLAGS),)
COMMON_GEN_CFLAGS	:=	$(strip $(MAD_GEN_OPT))
endif

COMMON_CFLAGS		+=	-DMAD1 $(MAD_OPT) -I$(MAD_INC)

# Repertoires principaux de Madeleine
MAD_SRC		:=	$(MAD1_ROOT)/source
MAD_DEP		:=	$(MAD_SRC)/depend
MAD_OBJ		:=	$(MAD_SRC)/obj/$(PM2_ARCH_SYS)
MAD_INC		:=	$(MAD1_ROOT)/include
MAD_LIBD	:=	$(MAD1_ROOT)/lib/$(PM2_ARCH_SYS)

# Creation des repertoires necessaires "a la demande"
ifneq ($(MAKECMDGOALS),distclean) # avoid auto-building of directories
ifneq ($(wildcard $(MAD_DEP)),$(MAD_DEP))
DUMMY_BUILD	:=	$(shell mkdir -p $(MAD_DEP))
endif

ifneq ($(wildcard $(MAD_OBJ)),$(MAD_OBJ))
DUMMY_BUILD	:=	$(shell mkdir -p $(MAD_OBJ))
endif

ifneq ($(wildcard $(MAD_LIBD)),$(MAD_LIBD))
DUMMY_BUILD	:=	$(shell mkdir -p $(MAD_LIBD))
endif
endif

# Obligatoire et inamovible ;-)
MAD_CC		:=	gcc

# CFLAGS et LDFLAGS: attention a ne pas utiliser ':=' !
MAD_CFLAGS	=	$(COMMON_GEN_CFLAGS) $(COMMON_CFLAGS)
MAD_KCFLAGS	=	$(MAD_CFLAGS) -DMAD_KERNEL \
			-DNET_ARCH=\"$(NET_INTERF)\" $(NET_INIT) \
			$(NET_CFLAGS)
MAD_LDFLAGS	=	$(COMMON_LDFLAGS)

# Bibliotheques
MAD_LIB		:=	$(MAD_LIBD)/libmad$(COMMON_EXT).a
COMMON_LIBS	+=	$(MAD_LIB)

COMMON_LDFLAGS	+=	$(NET_LFLAGS) $(NET_LLIBS) $(ARCHDLIB)


COMMON_DEFAULT_TARGET	+=	mad_default
COMMON_CLEAN_TARGET	+=	madclean
COMMON_DISTCLEAN_TARGET	+=	maddistclean

######################## Applications Madeleine ########################

ifdef MAD_EX_DIR

ifeq ($(MAD_EX_DIR),.)
SRC_DIR		:=	./
else
SRC_DIR		:=	$(MAD_EX_DIR)
endif

COMMON_MAKEFILES	+=	$(SRC_DIR)/Makefile

# Repertoires applicatifs
DEP_DIR		:=	$(SRC_DIR)/depend
OBJ_DIR		:=	$(SRC_DIR)/obj/$(PM2_ARCH_SYS)
BIN_DIR		:=	$(SRC_DIR)/bin/$(PM2_ARCH_SYS)

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

