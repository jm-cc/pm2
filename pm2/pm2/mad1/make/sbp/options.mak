

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
# Generic Network Interface, SBP (Fast Ethernet) implementation
#

NET_INTERF	=	sbp
NET_INIT	=	-DNETINTERF_INIT=mad_sbp_netinterf_init -DCANT_SEND_TO_SELF

NET_CFLAGS	=	
NET_LFLAGS	=	
NET_LLIBS	=	-lded

