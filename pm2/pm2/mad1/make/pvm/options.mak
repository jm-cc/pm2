
#
# Generic Network Interface, PVM implementation
#

NET_INTERF	=	pvm
NET_INIT	=	-DNETINTERF_INIT=mad_pvm_netinterf_init

NET_CFLAGS	=	-I$(PVM_ROOT)/include
NET_LFLAGS	=	-L$(PVM_ROOT)/lib/$(PVM_ARCH)
NET_LLIBS	=	-lpvm3

