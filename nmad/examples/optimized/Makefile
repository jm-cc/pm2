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

export FLAVOR_DEFAULT:=mad4

MOD_SRC	:= $(PM2_ROOT)/nmad/examples/optimized

include $(PM2_ROOT)/make/apps.mak

$(MOD_GEN_BIN)/ping-optimized$(MOD_EXT): $(patsubst %.o, %$(MOD_EXT).o, \
			$(if $(ping-optimized-objs), $(ping-optimized-objs), ping-optimized.o)) indexed-optimized.o vector-optimized.o
$(MOD_GEN_BIN)/ping-optimized$(MOD_EXT): LDFLAGS += $(ping-optimized-ldflags) -lm
$(MOD_GEN_BIN)/ping-optimized$(MOD_EXT): $(MOD_STAMP_FILES) | flavor

$(MOD_GEN_OBJ)/ping-optimized$(MOD_EXT).o: CFLAGS += $(ping-optimized-cflags)
$(MOD_GEN_OBJ)/ping-optimized$(MOD_EXT).o: $(MOD_STAMP_FILES) | flavor

