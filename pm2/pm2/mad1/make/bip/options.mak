
#
# Generic Network Interface, BIP (Myrinet) implementation
#

NET_INTERF	=	bip
NET_INIT	=	-DNETINTERF_INIT=mad_bip_netinterf_init

NET_CFLAGS	=	-I/usr/local/bip/include
NET_LFLAGS	=	-L/usr/local/bip/lib
NET_LLIBS	=	-lbip

