

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

LIBRARY=marcel
#LIBNAME=$(LIBRARY)

LIB_SO_MAJ=0

#$(LIBRARY)_CC=gcc
#$(LIBRARY)_AS=gcc
include $(PM2_ROOT)/make/libs-vars.mak

ifeq ($(LIBPTHREAD),true)
LIBNAME=pthread
LIB_LIB_SO_OPTIONS=-L$(LIB_GEN_LIB) -ltbx
$(LIB_LIB_SO): $(LIB_LIB_SO).0 libpthread.map

endif

include $(PM2_ROOT)/make/libs-rules.mak
