

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
# Generic Network Interface, MPI-BIP (Myrinet) implementation
#

NET_INTERF	=	mpi_bip
NET_INIT	=	-DNETINTERF_INIT=mad_mpi_netinterf_init

NET_CFLAGS	=	-I/usr/local/bip/include/mpi -I/usr/local/bip/include
NET_LFLAGS	=	-L/usr/local/bip/lib/mpi_gnu -L/usr/local/bip/lib
NET_LLIBS	=	-lmpi -lbip

