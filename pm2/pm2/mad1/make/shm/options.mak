#!/bin/sh

#
# Generic Network Interface, IPC (Shared Memory) implementation
#

NET_INTERF	=	shm
NET_INIT	=	-DNETINTERF_INIT=mad_shm_netinterf_init

NET_CFLAGS	=	
NET_LFLAGS	=	
NET_LLIBS	=	-lposix4

