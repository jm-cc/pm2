

# PM2: Parallel Multithreaded Machine
# Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

CC	:=	g++

CFLAGS	:=	$(shell pm2-config --cflags) $(OPTIONS)
LIBS	:=	$(shell pm2-config --libs)
DEP	:=	$(shell pm2-config --stampfile all)
BUILD	:=	$(shell pm2-config --builddir)
BIN	:=	$(BUILD)/examples/c++/bin

DUMMY := $(shell mkdir -p $(BIN))

PROGS := hello

$(BIN)/hello: $(DEP)

$(BIN)/%: %.o
	$(CC) -o $@ $< $(LIBS)

%.o: %.C
	$(CC) -c $< $(CFLAGS) -o $@
