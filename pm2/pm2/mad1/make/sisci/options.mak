
#
# Generic Network Interface, SISCI implementation
#

NET_INTERF	=	sisci
NET_INIT	=	-DNETINTERF_INIT=mad_sisci_netinterf_init -DCANT_SEND_TO_SELF
NET_CFLAGS	=	-I$(HOME)/DIS/src/SISCI/api/ -I$(HOME)/DIS/src/SISCI/src/
NET_LFLAGS	=	
NET_LLIBS	=	$(HOME)//DIS/src/SISCI/api/libsisci.a


