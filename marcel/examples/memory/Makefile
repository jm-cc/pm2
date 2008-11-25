

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

export FLAVOR_DEFAULT:=marcel-mami

ifndef FLAVOR
  ifdef PM2_FLAVOR
    export FLAVOR:=$(PM2_FLAVOR)
  else # PM2_FLAVOR
    export FLAVOR:=$(FLAVOR_DEFAULT)
  endif # PM2_FLAVOR
endif # FLAVOR

FLAVOR_WITH_FORTRAN :=	$(shell $(PM2_ROOT)/bin/pm2-flavor get --flavor=$(FLAVOR) | grep marcel=fortran)
ifeq (,$(FLAVOR_WITH_FORTRAN))
APPS_EXCLUDE += $(basename $(wildcard *.f90))
endif

membind-ldflags := -lrt -lnuma
memteams-ldflags := -lrt -lnuma
memteams-bubbles-ldflags := -lrt -lnuma
check_migration-ldflags := -lnuma
crossing_access-ldflags := -lnuma

mamif: CC=gfortran

include $(PM2_ROOT)/make/apps.mak

