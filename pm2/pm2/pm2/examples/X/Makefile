
export FLAVOR_DEFAULT:=pm2

CFLAGS += -I/usr/X11R6/include -I$$OPENWINHOME/include
LDFLAGS += -L/usr/X11R6/lib -L$$OPENWINHOME/lib -lxview -lolgx -lX11

PROGS := polygon

include $(PM2_ROOT)/make/apps.mak

$(APP_BIN)/polygon$(APP_EXT): $(APP_OBJ)/regul$(APP_EXT).o $(APP_OBJ)/custom$(APP_EXT).o

