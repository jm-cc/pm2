include $(MARCEL_ROOT)/make/common.mak

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


CFLAGS = $(MAR_CFLAGS) -I. 
LDFLAGS = $(MAR_LFLAGS) $(MAR_LLIB) -lm


TARGET = fft
OBJS = fft.o marcel_np.o 
MACROS = c.m4.marcel

x = *

$(TARGET): $(OBJS) $(MAR_LIB)
	gcc $(OBJS) $(CFLAGS) -o $(TARGET) $(LDFLAGS)

.SUFFIXES:
.SUFFIXES:	.o .c .C .h .H

.H.h:
	m4 ${MACROS} $*.H > $*.h

.C.c:
	m4 $(MACROS) $*.C > $*.c

.c.o:
	gcc -c $(CFLAGS) $*.c

.C.o:
	m4 $(MACROS) $*.C > $*.c
	gcc -c $(CFLAGS) $*.c

clean:
	rm -f $(OBJS) $(TARGET) fft.c *~ #*
