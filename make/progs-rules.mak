# -*- mode: makefile;-*-

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

include $(PM2_SRCROOT)/make/objs-rules.mak

ifneq ($(PROGRAM),$(PROGNAME))
$(PROGNAME): $(PROGRAM)
endif

prog: $(PROGRAM)

link: prog
libs: $(MOD_OBJECTS)

all: $(PROGRAM)

.PHONY: $(PROGRAM) $(PROGNAME) prog

$(PROGRAM): $(PRG_PRG)

$(MOD_DEPENDS): $(COMMON_DEPS) $(MOD_GEN_C_SOURCES) $(MOD_GEN_C_INC)

prog: $(MOD_PRG)

$(MOD_PRG): $(MOD_OBJECTS) $(MOD_STAMP_FILES)
	$(COMMON_LINK)
	$(COMMON_MAIN) $(CC) $(MOD_OBJECTS) $(LDFLAGS) -o $(PRG_PRG)

