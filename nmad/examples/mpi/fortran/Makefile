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
MPIF77	:=	../mpif77
endif

PROGS		:=	fpi basics

modules 	:=	$(shell pm2-config --modules)
libs		:=	$(shell for i in ${modules} ; do pm2-config --stampfile $$i ; done)

all: $(PROGS)

%: %.o
	$(MPIF77) $< -o $@ ${${@}_LIBS}

%.o: %.f %.h
	$(MPIF77) -c $< -o $@

%.o: %.f
	$(MPIF77) -c $< -o $@

fpi: fpi.o ${libs}
basics: basics.o ${libs}

clean:
	rm -f *.o $(PROGS)

lib:
	make -C $(PM2_ROOT)/mad4 FLAVOR=mad4-mpi

clean-lib:
	make -C $(PM2_ROOT)/mad4 FLAVOR=mad4-mpi clean
