

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
# Generic Network Interface, PVM implementation
#

NET_INTERF	=	pvm
NET_INIT	=	-DNETINTERF_INIT=mad_pvm_netinterf_init

NET_CFLAGS	=	-I$(PVM_ROOT)/include
NET_LFLAGS	=	-L$(PVM_ROOT)/lib/$(PVM_ARCH)
NET_LLIBS	=	-lpvm3

