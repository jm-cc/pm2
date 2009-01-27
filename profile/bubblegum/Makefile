

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

FLAVOR_DEFAULT:=marcel-bubblegum
CFLAGS += -I${MOD_GEN_INC}
CFLAGS += -I$(shell pm2-config --flavor=$(FLAVOR) --includedir bubblelib)

.PHONY: default
include $(PM2_ROOT)/make/progs.mak

$(MOD_GEN_INC)/fut_entries.h: $(MOD_GEN_STAMP)/fut_stamp ${PM2_ROOT}/bin/pm2-build-fut-table
	$(COMMON_BUILD)
	$(COMMON_MAIN) ${PM2_ROOT}/bin/pm2-build-fut-table -f $(FLAVOR) -m bubbles

$(MOD_GEN_OBJ)/bubbles.o: $(MOD_GEN_INC)/fut_entries.h

$(MOD_GEN_STAMP)/fut_stamp: $(MOD_STAMP_FLAVOR)
	$(COMMON_BUILD)
	$(COMMON_MAIN) touch $@

