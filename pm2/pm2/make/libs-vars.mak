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

# if someone do 'make' in an module directory, ...
default: no_goal

# Variables communes
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/common-vars.mak

# Nom de la librairie
#---------------------------------------------------------------------
# Note dans quel cas est-ce necessaire ?
# Tous sauf mad1 et mad2
ifndef LIBNAME
LIBNAME := $(LIBRARY)
endif

# Repertoire source
#---------------------------------------------------------------------
LIB_SRC := source

# Inclusion du cache de configuration de la librairie
#---------------------------------------------------------------------
ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
-include $(PM2_MAK_DIR)/$(LIBRARY)-config.mak
endif

# Commandes de compilation et d'assemblage
#---------------------------------------------------------------------
# Note: y'a-t-il un controle de collision concernant ces variables ?
# Note: pas de LD := ...   Est-ce normal ?
ifneq ($($(LIBRARY)_CC),)
CC := $($(LIBRARY)_CC)
endif

ifneq ($($(LIBRARY)_AS),)
AS := $($(LIBRARY)_AS)
endif

# Repertoires destination
#---------------------------------------------------------------------
LIB_REP_TO_BUILD := \
	$(LIB_GEN_STAMP)\
	$(LIB_GEN_DIR)\
	$(LIB_GEN_INC)\
	$(LIB_GEN_SRC)\
	$(LIB_GEN_CPP)\
	$(LIB_GEN_DEP)\
	$(LIB_GEN_ASM)\
	$(LIB_GEN_OBJ)\
	$(LIB_GEN_TMP)\
	$(LIB_GEN_LIB)

# Noms des fichiers bibliothèques cibles
#---------------------------------------------------------------------
LIB_LIB_A=$(LIB_GEN_LIB)/lib$(LIBNAME)$(LIB_EXT).a
LIB_LIB_SO=$(LIB_GEN_LIB)/lib$(LIBNAME)$(LIB_EXT).so
ifeq ($(BUILD_STATIC_LIBS),true)
LIB_LIB += $(LIB_LIB_A)
endif
ifeq ($(BUILD_DYNAMIC_LIBS),true)
LIB_LIB += $(LIB_LIB_SO)
endif

# Sources
#---------------------------------------------------------------------
LIB_C_SOURCES :=  $(wildcard $(LIB_SRC)/*.c)
LIB_S_SOURCES :=  $(wildcard $(LIB_SRC)/*.S)

# Bases : fichiers sans extension ni repertoire
#---------------------------------------------------------------------
LIB_C_BASE := $(foreach I, $(LIB_C_SOURCES), $(notdir $(basename $I)))
LIB_S_BASE := $(foreach I, $(LIB_S_SOURCES), $(notdir $(basename $I)))
LIB_BASE   := $(LIB_C_BASE) $(LIB_S_BASE)

# Objets
#---------------------------------------------------------------------
LIB_C_OBJECTS := $(foreach I, $(LIB_C_BASE), $(LIB_GEN_OBJ)/$I$(LIB_EXT).o)
LIB_S_OBJECTS := $(foreach I, $(LIB_S_BASE), $(LIB_GEN_OBJ)/$I$(LIB_EXT).o)
LIB_OBJECTS   := $(LIB_C_OBJECTS) $(LIB_S_OBJECTS)

# PICS - librairies dynamiques
#---------------------------------------------------------------------
LIB_C_PICS :=  $(foreach I, $(LIB_C_BASE), $(LIB_GEN_OBJ)/$I$(LIB_EXT).pic)
LIB_S_PICS :=  $(foreach I, $(LIB_S_BASE), $(LIB_GEN_OBJ)/$I$(LIB_EXT).pic)
LIB_PICS   :=  $(LIB_C_PICS) $(LIB_S_PICS)

# Preprocs
#---------------------------------------------------------------------
LIB_C_PREPROC := $(foreach I, $(LIB_C_BASE), $(LIB_GEN_CPP)/$I$(LIB_EXT).i)
LIB_S_PREPROC := $(foreach I, $(LIB_S_BASE), $(LIB_GEN_CPP)/$I$(LIB_EXT).si)
LIB_PREPROC   := $(LIB_C_PREPROC) $(LIB_S_PREPROC)

# FUT entries
#---------------------------------------------------------------------
# LIB_S_FUT : pas de sens
LIB_C_FUT := $(foreach I, $(LIB_C_BASE), $(LIB_GEN_CPP)/$I$(LIB_EXT).fut)
LIB_FUT   := $(LIB_C_FUT)

# Dependances
#---------------------------------------------------------------------
LIB_C_DEPENDS := $(foreach I, $(LIB_C_BASE), $(LIB_GEN_DEP)/$I$(LIB_EXT).d)
LIB_S_DEPENDS := $(foreach I, $(LIB_S_BASE), $(LIB_GEN_DEP)/$I$(LIB_EXT).d)
LIB_DEPENDS   := $(strip $(LIB_C_DEPENDS) $(LIB_S_DEPENDS))

# "Convertisseurs" utiles
#---------------------------------------------------------------------
LIB_DEP_TO_OBJ = $(LIB_GEN_OBJ)/$(patsubst %.d,%.o,$(notdir $@))
LIB_OBJ_TO_S   = $(LIB_GEN_ASM)/$(patsubst %.o,%.s,$(notdir $@))
LIB_PIC_TO_S   = $(LIB_GEN_ASM)/$(patsubst %.pic,%.s,$(notdir $@))

# Contribution aux dependances communes :
# - stamp
# - makefile
#---------------------------------------------------------------------
COMMON_DEPS += $(LIB_STAMP_FLAVOR) $(MAKEFILE_FILE) 

######################################################################
