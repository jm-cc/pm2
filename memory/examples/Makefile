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

export FLAVOR_DEFAULT:=mami

ifndef FLAVOR
  ifdef PM2_FLAVOR
    export FLAVOR:=$(PM2_FLAVOR)
  else # PM2_FLAVOR
    export FLAVOR:=$(FLAVOR_DEFAULT)
  endif # PM2_FLAVOR
endif # FLAVOR

FLAVOR_WITH_FORTRAN :=	$(shell $(PM2_ROOT)/bin/pm2-flavor get --flavor=$(FLAVOR) | grep memory=fortran)
ifeq (,$(FLAVOR_WITH_FORTRAN))
APPS_EXCLUDE += $(basename $(wildcard *.f90))
endif

all: $(APPS_LIST)
config: .config/blas_$(FLAVOR) .config/numa_$(FLAVOR)

# Include the file that tells whether BLAS is available.
-include .config/blas_$(FLAVOR)

# Determine whether BLAS is available.
.config/blas_$(FLAVOR):
	@mkdir -p "$(dir $@)"
	@echo "#include <cblas.h>" > ,,conftest.c
	@echo "int main (int argc, char *argv[]) { int x=CblasRowMajor; return 0; }" >> ,,conftest.c
	@if $(CC) $(CFLAGS) -o ,,conftest ,,conftest.c $(LDFLAGS) -lblas 2>/dev/null;	\
	then echo "HAVE_BLAS = yes" > "$@" ;								\
	else echo "HAVE_BLAS = no" > "$@" ; fi
	@rm -f ,,conftest{,.c}

ifeq (no,$(HAVE_BLAS))
APPS_EXCLUDE += sgemm
APPS_EXCLUDE += saxpy
endif
sgemm-ldflags := -lblas
saxpy-ldflags := -lblas

# Include the file that tells whether NUMA is available.
-include .config/numa_$(FLAVOR)

# Determine whether NUMA is available.
.config/numa_$(FLAVOR):
	@mkdir -p "$(dir $@)"
	@echo "#include <numa.h>" > ,,conftest.c
	@echo "int main (int argc, char *argv[]) { return 0; }" >> ,,conftest.c
	@if $(CC) $(CFLAGS) -o ,,conftest ,,conftest.c $(LDFLAGS) -lnuma 2>/dev/null;	\
	then echo "HAVE_NUMA = yes" > "$@" ;								\
	else echo "HAVE_NUMA = no" > "$@" ; fi
	@rm -f ,,conftest{,.c}

ifeq (no,$(HAVE_NUMA))
APPS_EXCLUDE += check_migration
APPS_EXCLUDE += crossing_access
APPS_EXCLUDE += membind
APPS_EXCLUDE += memteams
APPS_EXCLUDE += memteams-bubbles
APPS_EXCLUDE += distribute3
endif
membind-ldflags := -lrt -lnuma
memteams-ldflags := -lrt -lnuma
memteams-bubbles-ldflags := -lrt -lnuma
check_migration-ldflags := -lnuma
crossing_access-ldflags := -lnuma
distribute3-ldflags := -lnuma

mamif: CC=gfortran

include $(PM2_ROOT)/make/apps.mak
