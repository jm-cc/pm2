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

ifndef PM2_ROOT
$(error The PM2_ROOT variable is not set)
endif
PM2_MAKE :=  $(PM2_ROOT)/make

ifndef MARCEL_ROOT
MARCEL_ROOT :=  $(PM2_ROOT)/marcel
endif
MARCEL_MAKE :=  $(MARCEL_ROOT)/make

ifndef DSM_ROOT
DSM_ROOT :=  $(PM2_ROOT)/dsm
endif
DSM_MAKE :=  $(DSM_ROOT)/make

ifndef MAD2_ROOT
MAD2_ROOT :=  $(PM2_ROOT)/mad2
endif
MAD_MAKE  :=  $(MAD2_ROOT)/make

ifndef TBX_ROOT
TBX_ROOT :=  $(PM2_ROOT)/toolbox
endif
TBX_MAKE :=  $(TBX_ROOT)/make

ifndef NTBX_ROOT
NTBX_ROOT :=  $(PM2_ROOT)/toolbox/net
endif
NTBX_MAKE :=  $(NTBX_ROOT)/make

MASTER_MODULE := $(strip $(MASTER_MODULE))
MASTER_ROOT   := $($(MASTER_MODULE)_ROOT)

GEN := build

GEN_ROOT         :=  $(MASTER_ROOT)/$(GEN)
COMMON_ARCH      :=  $(shell $(PM2_ROOT)/bin/pm2_arch)
COMMON_SYS       :=  $(shell $(PM2_ROOT)/bin/pm2_sys)
COMMON_MAKEFILES :=  $(PM2_ROOT)/make/master_vars.mak
COMMON_CC        :=  gcc
COMMON_AS        :=  gcc
COMMON_CFLAGS    := -D$(COMMON_SYS) -D$(COMMON_ARCH)
COMMON_LDFLAGS   :=
COMMON_ASFLAGS   := -D$(COMMON_SYS) -D$(COMMON_ARCH)
COMMON_LLIBS     :=
COMMON_OPTIONS   :=
COMMON_LINK      := static

ifndef CLASS
CLASS :=  default
endif

ifndef NAME
NAME :=  $(CLASS)
endif

GEN_DIR :=  $(GEN_ROOT)/$(NAME)
GEN_BIN :=  $(GEN_DIR)/bin
GEN_OBJ :=  $(GEN_DIR)/obj
GEN_DEP :=  $(GEN_DIR)/dep
GEN_SRC :=  $(GEN_DIR)/src
GEN_INC :=  $(GEN_DIR)/inc
GEN_ASM :=  $(GEN_DIR)/asm
ifdef INSTALL
GEN_LIB :=  $(INSTALL)/lib
else
GEN_LIB :=  $(GEN_DIR)/lib
endif 

ifneq ($(MAKECMDGOALS),distclean)
DUMMY_BUILD :=  $(shell mkdir -p $(GEN_ASM))
DUMMY_BUILD :=  $(shell mkdir -p $(GEN_INC))
DUMMY_BUILD :=  $(shell mkdir -p $(GEN_SRC))
DUMMY_BUILD :=  $(shell mkdir -p $(GEN_DEP))
DUMMY_BUILD :=  $(shell mkdir -p $(GEN_OBJ))
DUMMY_BUILD :=  $(shell mkdir -p $(GEN_LIB))
DUMMY_BUILD :=  $(shell mkdir -p $(GEN_BIN))
endif

ifdef SRC_DIR
APP_DIR     :=  $(SRC_DIR)/$(GEN)/$(NAME)
APP_BIN     :=  $(APP_DIR)/bin
APP_OBJ     :=  $(APP_DIR)/obj
APP_DEP     :=  $(APP_DIR)/dep
ifdef INSTALL
APP_BIN :=  $(INSTALL)/bin
else
APP_BIN :=  $(APP_DIR)/bin
endif 
DUMMY_BUILD :=  $(shell mkdir -p $(APP_DEP))
DUMMY_BUILD :=  $(shell mkdir -p $(APP_OBJ))
DUMMY_BUILD :=  $(shell mkdir -p $(APP_BIN))
endif

ifdef INSTALL
COMMON_OPTIONS += INSTALL $(subst /,_,$(INSTALL))
else
COMMON_OPTIONS += INSTALL default
endif

ifdef $(PM2_CLASS_PATH)
include $(PM2_CLASS_PATH)/$(CLASS).mak
COMMON_MAKEFILES +=  $(PM2_CLASS_PATH)/$(CLASS).mak
else
include $(MASTER_ROOT)/make/classes/$(CLASS).mak
COMMON_MAKEFILES +=  $(MASTER_ROOT)/make/classes/$(CLASS).mak
endif
COMMON_CFLAGS    +=  $(CLASS_CFLAGS)
COMMON_LDFLAGS   +=  $(CLASS_LDFLAGS)

ifdef LINK
COMMON_LINK :=  $(LINK)
endif

ifdef VERB
MAK_VERB :=  $(VERB)
endif

ifeq ($(MAK_VERB),verbose)
COMMON_PREFIX :=#
COMMON_HIDE   :=#	
else
ifeq ($(MAK_VERB),normal)
COMMON_PREFIX :=#
COMMON_HIDE    =  @
else
ifeq ($(MAK_VERB),quiet)
COMMON_PREFIX  =  @ echo "building " $(@F) ;
COMMON_HIDE   :=  @
else  # silent
COMMON_PREFIX  =  @
COMMON_HIDE   :=  @
endif
endif
endif

##############################################################################
