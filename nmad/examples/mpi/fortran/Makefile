# NewMadeleine
# Copyright (C) 2006 (see AUTHORS file)
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

ifeq ($(MPIF77),)
MPIF77	:=	../mpif77 -Wall
endif

ifeq ($(MPIF90),)
MPIF90	:=	../mpif90 -Wall
endif

PROGS		:=	fpi basics pi3f90

modules 	:=	$(shell pm2-config --modules)
libs		:=	$(shell for i in ${modules} ; do pm2-config --stampfile $$i ; done)

all: $(PROGS)

%: %.f
	$(MPIF77) $< -o $@ ${${@}_LIBS}

fpi: fpi.f ${libs}
basics: basics.f ${libs}

%: %.f90
	$(MPIF90) $< -o $@ ${${@}_LIBS}

pi3f90: pi3f90.f90 ${libs}

clean:
	rm -f *.o $(PROGS)

lib:
	make -C $(PM2_ROOT)/nmad FLAVOR=nmad-mpi

clean-lib:
	make -C $(PM2_ROOT)/nmad FLAVOR=nmad-mpi clean
