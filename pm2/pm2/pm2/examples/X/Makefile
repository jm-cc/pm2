

# PM2: Parallel Multithreaded Machine
# Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

export FLAVOR_DEFAULT:=pm2

CFLAGS += -I/usr/X11R6/include -I$$OPENWINHOME/include
LDFLAGS += -L/usr/X11R6/lib -L$$OPENWINHOME/lib -lxview -lolgx -lX11

PROGS := polygon

include $(PM2_ROOT)/make/apps.mak

$(APP_BIN)/polygon$(APP_EXT): $(APP_OBJ)/regul$(APP_EXT).o $(APP_OBJ)/custom$(APP_EXT).o

