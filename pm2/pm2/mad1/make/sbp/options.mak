
#
# Generic Network Interface, SBP (Fast Ethernet) implementation
#

NET_INTERF	=	sbp
NET_INIT	=	-DNETINTERF_INIT=mad_sbp_netinterf_init -DCANT_SEND_TO_SELF

NET_CFLAGS	=	
NET_LFLAGS	=	
NET_LLIBS	=	-lded

