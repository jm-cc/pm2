
#
# Generic Network Interface, implementation on VIA
#

NET_INTERF	=	via
NET_INIT	=	-DMAD_VIA -DNETINTERF_INIT=mad_via_netinterf_init \
			-D_REENTRANT -DCANT_SEND_TO_SELF

NET_CFLAGS	=	-DVIA_DEV_NAME=\"/dev/via_eth0\"
NET_LFLAGS	=	
NET_LLIBS	=	-lvipl -lpthread

