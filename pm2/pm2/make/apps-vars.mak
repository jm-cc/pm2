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

ifndef FLAVOR_DEFAULT
FLAVOR_DEFAULT := default
endif

include $(PM2_ROOT)/make/common-vars.mak

ifdef OLD_MAKEFILE
CONFIG_MODULES=$(shell $(PM2_CONFIG) --modules)
else
-include $(PM2_MAK_DIR)/apps-libs.mak
endif

all:

SRC_DIR := $(CURDIR)

ifneq ($(MAKECMDGOALS),config)

ifdef OLD_MAKEFILE
APP_STAMP := $(shell $(PM2_CONFIG) --stampdir)
APP_EXT := $(shell $(PM2_CONFIG) --ext)
APP_LOADER := $(shell $(PM2_CONFIG) --loader)

CC := $(shell $(PM2_CONFIG) --cc $(LIBRARY))

APP_CFLAGS += $(shell $(PM2_CONFIG) --cflags)
APP_LDFLAGS += $(shell $(PM2_CONFIG) --libs)
CFLAGS += $(APP_CFLAGS)
LDFLAGS += $(APP_LDFLAGS)
APP_LLIBS += $(shell $(PM2_CONFIG) --libs-only-l)
PM2_MODULES += $(shell $(PM2_CONFIG) --modules)
APP_STAMP_FILES := $(shell $(PM2_CONFIG) --stampfile)
APP_STAMP_FILES += $(shell $(PM2_CONFIG) --stampfile all)
THIS_DIR := $(shell basename `pwd`)
ifeq ($(THIS_DIR),examples)
APP_BUILDDIR := $(shell $(PM2_CONFIG) --builddir)/examples
else
APP_BUILDDIR := $(shell $(PM2_CONFIG) --builddir)/examples/$(THIS_DIR)
endif

APP_BIN := $(APP_BUILDDIR)/bin
APP_OBJ := $(APP_BUILDDIR)/obj
APP_ASM := $(APP_BUILDDIR)/asm
APP_DEP := $(APP_BUILDDIR)/dep
else
-include $(PM2_MAK_DIR)/apps-config.mak
endif # OLD_MAKEFILE
endif

# Sources, objets et dependances
SOURCES :=  $(wildcard $(SRC_DIR)/*.c)
OBJECTS :=  $(patsubst %.c,%$(APP_EXT).o,$(subst $(SRC_DIR),$(APP_OBJ),$(SOURCES)))
DEPENDS :=  $(patsubst %.c,%$(APP_EXT).d,$(subst $(SRC_DIR),$(APP_DEP),$(SOURCES)))
APPS = $(foreach PROG,$(PROGS),$(APP_BIN)/$(PROG)$(APP_EXT))

# Convertisseurs utiles
DEP_TO_OBJ =  $(APP_OBJ)/$(patsubst %.d,%.o,$(notdir $@))

COMMON_DEPS += $(APP_STAMP_FILES) $(MAKEFILE_FILE)

$(PM2_MAK_DIR)/apps-config.mak: $(APP_STAMP_FLAVOR)
	@$(PM2_CONFIG) --gen_mak apps

$(PM2_MAK_DIR)/apps-libs.mak:  $(APP_STAMP_FLAVOR)
	@mkdir -p `dirname $@`
	@echo "CONFIG_MODULES= " `$(PM2_CONFIG) --modules` > $@
