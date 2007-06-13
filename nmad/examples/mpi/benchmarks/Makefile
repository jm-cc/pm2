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

modules :=	$(shell pm2-config --modules)
libs	:=	$(shell for i in ${modules} ; do pm2-config --stampfile $$i ; done)

#MPICC	:=	/opt/mpich-mx/bin/mpicc

PROGS		:=	pingpong mpi_para_ping index indexType_hetero_size mpi_multi_ping mpi_latency mpi_multi_ping_hetero vector struct
pingpong_LIBS	:=	-lm
vector_LIBS	:=	-lm

all: $(PROGS)

%: %.o ${libs}
	$(MPICC) $^ -o $@ ${${@}_LIBS}

%.o: %.c %.h
	$(MPICC) -c $< -o $@

%.o: %.c
	$(MPICC) -c $< -o $@

mpi_para_ping: mpi_para_ping.o
pingpong: pingpong.o structType.o vectorType.o indexType.o toolbox.o
index: index.o indexType.o toolbox.o
vector: vector.o vectorType.o toolbox.o
struct: struct.o structType.o toolbox.o

indexType_hetero_size: indexType_hetero_size.o
mpi_multi_ping: mpi_multi_ping.o
mpi_latency:mpi_latency.o
mpi_multi_ping_hetero: mpi_multi_ping_hetero.o

clean:
	rm -f *.o $(PROGS)

lib:
	make -C $(PM2_ROOT)/mad4 FLAVOR=mad4-mpi

clean-lib:
	make -C $(PM2_ROOT)/mad4 FLAVOR=mad4-mpi clean
