
#
# Generic Network Interface, MPI-LAM  implementation
#

NET_INTERF	=	mpi_lam
NET_INIT	=	-DNETINTERF_INIT=mad_mpi_netinterf_init

NET_CFLAGS	=	-I$(HOME)/lam/h
NET_LFLAGS	=	-L$(HOME)/lam/lib

NET_LLIBS	=	-lmpi -largs -ltrillium -ltstdio -lt

