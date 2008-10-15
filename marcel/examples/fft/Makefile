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


fft-objs := fft.o marcel_np.o
MACROS = c.m4.marcel

.SUFFIXES:
.SUFFIXES:	.c .cm4 .h .hm4

.hm4.h:
	m4 ${MACROS} $*.hm4 > $*.h

.cm4.c:
	m4 $(MACROS) $*.cm4 > $*.c

PROGS = fft

include $(PM2_ROOT)/make/apps.mak

fft.o: fft.c
