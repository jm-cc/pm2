

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

export FLAVOR_DEFAULT:=marcel

# Fichiers C à ne pas considérer comme but
#----------------------------------------------------------------------
APPS_EXCLUDE = $(basename $(wildcard *-posix.c))

FLAVOR_WITH_PMARCEL :=	$(shell $(PM2_ROOT)/bin/pm2-flavor get | grep pmarcel)
ifeq (,$(FLAVOR_WITH_PMARCEL))
APPS_EXCLUDE += $(basename $(wildcard barrier*.c))
endif

FLAVOR_WITH_NUMA :=	$(shell $(PM2_ROOT)/bin/pm2-flavor get | grep numa)
ifeq (,$(FLAVOR_WITH_NUMA))
APPS_EXCLUDE += $(basename $(wildcard bubble-*.c))
APPS_EXCLUDE += $(basename $(wildcard memomp-*.c))
APPS_EXCLUDE += $(basename $(wildcard krazu-bubbles.c))
endif

FLAVOR_WITH_TLS :=	$(shell $(PM2_ROOT)/bin/pm2-flavor get | grep tls)
ifeq (,$(FLAVOR_WITH_TLS))
APPS_EXCLUDE += $(basename $(wildcard tls*.c))
endif

# Programmes multi-objects et flags spécifics
#----------------------------------------------------------------------
eclone-objs := clone.o eclone.o
eclone-cflags := -O0
stackjump-cflags := -fno-omit-frame-pointer

tsp-objs := tsp-main.o tsp-job.o tsp.o
tsp-cflags := -DMT
tsp-main-cflags := -DMT
tsp-job-cflags := -DMT
tsp-ldflags := -lm

balance-ldflags := -lm

barrier-1-cflags := -I$(PM2_ROOT)/marcel/include/pthread

hello: CC=g++

# Applications dont le nom n'a pas de .c correspondant
#----------------------------------------------------------------------
APPS :=

#----------------------------------------------------------------------
# Les applications compilées sont :
# $(PROGS)
# Ou, si $(PROGS) n'est pas défini :
# (%.c -> %)
# - $(APPS_EXCLUDE)
# - (%.c -> % qui apparaissent dans un %-objs)
# + $(APPS)
# + (%.c -> % qui ont un %-objs)
#----------------------------------------------------------------------

include $(PM2_ROOT)/make/apps.mak

