
#
# Generic Network Interface, MPI for SP2  implementation
# Change the base of the poe environment for your configuration
# This is for AIX 4.2.1

NET_INTERF      =       mpi_sp2
NET_INIT	=	-DNETINTERF_INIT=mad_mpi_netinterf_init

NET_CFLAGS      =       -I/usr/lpp/ppe.poe/include '-DMPI_Init(a,b)=poe_remote_main();MPI_Init(a,b)'
NET_LFLAGS      =       -L/usr/lpp/ppe.poe/lib -L/usr/lpp/ppe.poe/lib/ip
NET_LLIBS       =       -lmpi

