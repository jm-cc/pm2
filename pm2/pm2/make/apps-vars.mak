

# PM2: Parallel Multithreaded Machine
# Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

ifndef FLAVOR_DEFAULT
FLAVOR_DEFAULT := default
endif

include $(PM2_ROOT)/make/common-vars.mak

ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
-include $(PM2_MAK_DIR)/apps-libs.mak
endif

all:

SRC_DIR := $(CURDIR)

ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
-include $(PM2_MAK_DIR)/apps-config.mak
endif

THIS_DIR := $(shell basename `pwd`)
ifeq ($(THIS_DIR),examples)
APP_BUILDDIR := $(APP_BUILDD)/examples
else
APP_BUILDDIR := $(APP_BUILDD)/examples/$(THIS_DIR)
endif

APP_BIN := $(APP_BUILDDIR)/bin
APP_OBJ := $(APP_BUILDDIR)/obj
APP_ASM := $(APP_BUILDDIR)/asm
APP_DEP := $(APP_BUILDDIR)/dep

# Sources, objets et dependances
SOURCES :=  $(wildcard $(SRC_DIR)/*.c)
OBJECTS :=  $(patsubst %.c,%$(APP_EXT).o,$(subst $(SRC_DIR),$(APP_OBJ),$(SOURCES)))
DEPENDS :=  $(patsubst %.c,%$(APP_EXT).d,$(subst $(SRC_DIR),$(APP_DEP),$(SOURCES)))
APPS = $(foreach PROG,$(PROGS),$(APP_BIN)/$(PROG)$(APP_EXT))

# Convertisseurs utiles
DEP_TO_OBJ =  $(APP_OBJ)/$(patsubst %.d,%.o,$(notdir $@))

COMMON_DEPS += $(APP_STAMP_FLAVOR) $(APP_STAMP_FILES) $(MAKEFILE_FILE)

