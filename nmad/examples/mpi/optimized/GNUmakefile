# NewMadeleine
# Copyright (C) 2006 (see AUTHORS file)
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

export FLAVOR_DEFAULT:=nmad

MOD_SRC	:= ../../../../nmad/examples/mpi/optimized
PROGS 	:= ping_optimized

include ../../../../make/apps.mak

$(MOD_GEN_BIN)/ping_optimized: $(patsubst %.o, %.o, \
			$(if $(ping-optimized-objs), $(ping-optimized-objs), ping_optimized.o)) indexed_optimized.o vector_optimized.o struct_optimized.o
$(MOD_GEN_BIN)/ping_optimized: LDFLAGS += $(ping-optimized-ldflags) -lm
$(MOD_GEN_BIN)/ping_optimized: $(MOD_STAMP_FILES) | flavor

$(MOD_GEN_OBJ)/ping_optimized.o: CFLAGS += $(ping-optimized-cflags)
$(MOD_GEN_OBJ)/ping_optimized.o: $(MOD_STAMP_FILES) | flavor

