

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

#
# Generic Network Interface, MPI for SP2  implementation
# Change the base of the poe environment for your configuration
# This is for AIX 4.2.1

NET_INTERF      =       mpi_sp2
NET_INIT	=	-DNETINTERF_INIT=mad_mpi_netinterf_init

NET_CFLAGS      =       -I/usr/lpp/ppe.poe/include '-DMPI_Init(a,b)=poe_remote_main();MPI_Init(a,b)'
NET_LFLAGS      =       -L/usr/lpp/ppe.poe/lib -L/usr/lpp/ppe.poe/lib/ip
NET_LLIBS       =       -lmpi

