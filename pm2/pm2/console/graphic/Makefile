

.PHONY: default all
default: all

SRC_DIR	:=	.
BIN_DIR = ../bin/$(PM2_ARCH_SYS)

include ../../Makefile

PM2_APP_CFLAGS	=	-I/usr/X11R6/include -I$$OPENWINHOME/include
PM2_APP_LDFLAGS	=	-L/usr/X11R6/lib -L$$OPENWINHOME/lib
PM2_APP_LLIBS	=	-lxview -lolgx -lX11

all:	$(BIN_DIR)/xpm2
