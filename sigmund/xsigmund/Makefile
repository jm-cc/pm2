

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

export FLAVOR_DEFAULT:=sigmund

ifneq (,$(findstring $(PM2_SYS),"WIN98_SYS_WINNT_SYS_WIN2K_SYS"))
CFLAGS		+=	-fnative-struct
LDFLAGS		+=	-e _mainCRTStartup
endif

#CFLAGS += $(shell gtk-config --cflags)
#LDFLAGS += $(shell gtk-config --libs)
CFLAGS += -DGTK2 $(shell pkg-config --cflags gtk+-2.0)
LDFLAGS += $(shell pkg-config --libs gtk+-2.0)

include $(PM2_ROOT)/make/progs.mak
