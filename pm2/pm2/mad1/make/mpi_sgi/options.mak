
#
# Generic Network Interface, MPI-SGI  implementation
#

NET_INTERF	=	mpi_sgi
NET_INIT	=	-DNETINTERF_INIT=mad_mpi_netinterf_init

NET_CFLAGS	=	
NET_LFLAGS	=	

NET_LLIBS	=	-lmpi

