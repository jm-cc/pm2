
#
# Generic Network Interface, SCI implementation
#

NET_INTERF	=	sci

NET_LFLAGS	=	
NET_LLIBS	=	

# Accepted defines for SCI are DMA, HEADER and DEBUG
NET_CFLAGS	=	-DDEBUG -DHEADER -I/sci/driver/basesn2.0/drv/src
