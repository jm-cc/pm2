
#
# Generic Network Interface, MPI-BIP (Myrinet) implementation
#

NET_INTERF	=	mpi_bip
NET_INIT	=	-DNETINTERF_INIT=mad_mpi_netinterf_init

NET_CFLAGS	=	-I/usr/local/bip/include/mpi -I/usr/local/bip/include
NET_LFLAGS	=	-L/usr/local/bip/lib/mpi_gnu -L/usr/local/bip/lib
NET_LLIBS	=	-lmpi -lbip

