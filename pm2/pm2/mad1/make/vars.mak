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

# Deux macros déclarées d'intérêt public :
EMPTY		:=
SPACE		:=	$(EMPTY) $(EMPTY)

# L'utilisation de _MAD_OPTIONS (en lieu et place de MAD_OPTIONS)
# force l'adjonction d'un suffixe aux fichiers créés (binaires,
# objets, bibliothèques)
ifdef _MAD_OPTIONS
MAD_OPTIONS		:=	$(_MAD_OPTIONS)
MAD_USE_EXTENSION	:=	yes
else
MAD_USE_EXTENSION	:=	no
endif

# Inclusion des options
MAD_OPT_FILES		:=	$(MAD1_ROOT)/make/options.mak \
	$(foreach OPTION,$(MAD_OPTIONS),$(MAD1_ROOT)/make/options-$(OPTION).mak)

include $(MAD_OPT_FILES)

COMMON_MAKEFILES	+=	$(MAD_OPT_FILES)

# Calcul du suffixe
ifdef MAD_OPTIONS
MAD_EXT		:=	-mad_$(subst $(SPACE),_,$(sort $(MAD_OPTIONS)))
else
MAD_EXT		:=	-mad_none
endif

# Si on étend les noms de fichiers, il faut mettre a jour COMMON_EXT,
# sinon il faut mettre a jour COMMON_EXTRA_DEP...
ifeq ($(MAD_USE_EXTENSION),yes)
COMMON_EXT		:=	$(COMMON_EXT)$(MAD_EXT)
else
COMMON_EXTRA_DEP	:=	$(COMMON_EXTRA_DEP)$(MAD_EXT)
MAD_EXTRA_DEP_FILE	:=	$(MAD1_ROOT)/.opt$(COMMON_EXTRA_DEP)
COMMON_MAKEFILES	+=	$(MAD_EXTRA_DEP_FILE)
endif

# Inclusion des options specifiques au protocole reseau
include $(MAD1_ROOT)/make/custom/options.mak

COMMON_MAKEFILES	+=	$(MAD1_ROOT)/make/custom/options.mak

# Inclusion des options specifiques a l'architecture
include $(MAD1_ROOT)/make/archdep/$(shell basename $(PM2_SYS) _SYS).inc

MAD_GEN_OPT	:=	$(MAD_GEN_1) $(MAD_GEN_2) $(MAD_GEN_3) \
			$(MAD_GEN_4) $(MAD_GEN_5) $(MAD_GEN_6)

MAD_OPT		:=	$(strip $(MAD_OPT_0) $(MAD_OPT_1) $(MAD_OPT_2) \
			$(MAD_OPT_3) $(MAD_OPT_4) $(MAD_OPT_5) \
			$(MAD_OPT_6) $(MAD_OPT_7) $(MAD_OPT_8))

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
MAD_LIB_A	=	$(MAD_LIBD)/libmad$(COMMON_EXT).a
MAD_LIB_SO	=	$(MAD_LIBD)/libmad$(COMMON_EXT).so

ifeq ($(MAD_LINK),dyn)
MAD_LIB		=	$(MAD_LIB_SO)
else 
MAD_LIB		=	$(MAD_LIB_A)
endif


COMMON_LIBS	+=	$(MAD_LIB)

COMMON_LDFLAGS	+=	$(NET_LFLAGS) $(NET_LLIBS) $(ARCHDLIB)

MAD_USER_MAK		=	$(MAD1_ROOT)/make/user$(COMMON_EXT).mak


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

