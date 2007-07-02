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

ifeq ($(MPICC),)
MPICC	:=	../mpicc -g -Wall
endif

#MPICC	:=	/opt/mpich-mx/bin/mpicc

modules 	:=	$(shell pm2-config --modules)
libs		:=	$(shell for i in ${modules} ; do pm2-config --stampfile $$i ; done)

PROGS		:=	mpi_pastix

all: $(PROGS)

%: %.o
	$(MPICC) $< -o $@ ${${@}_LIBS}

%.o: %.c %.h
	$(MPICC) -c $< -o $@

%.o: %.c
	$(MPICC) -c $< -o $@

mpi_pastix: mpi_pastix.o ${libs}

clean:
	rm -f *.o $(PROGS)

lib:
	make -C $(PM2_ROOT)/nmad FLAVOR=nmad-mpi

clean-lib:
	make -C $(PM2_ROOT)/nmad FLAVOR=nmad-mpi clean
