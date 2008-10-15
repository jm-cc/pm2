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


ifndef FLAVOR
  ifdef PM2_FLAVOR
    export FLAVOR:=$(PM2_FLAVOR)
  else # PM2_FLAVOR
    export FLAVOR:=$(FLAVOR_DEFAULT)
  endif # PM2_FLAVOR
endif # FLAVOR

.SUFFIXES:
.SUFFIXES:	.c .cm4 .h .hm4

CFLAGS		:= $(shell pm2-config --flavor=${FLAVOR} --cflags)
LDFLAGS		:= $(shell pm2-config --flavor=${FLAVOR} --libs)
CC		:= $(shell pm2-config --flavor=${FLAVOR} --cc)
BUILD		:= $(shell pm2-config --flavor=${FLAVOR} --builddir)

OFILES		:= fft.o marcel_np.o

TARGET		:= ${BUILD}/examples/fft/bin/fft

fft: ${OFILES}
	-mkdir -p ${BUILD}/examples/fft/bin/
	${CC} ${CFLAGS} ${OFILES} ${LDFLAGS} -lm -o ${TARGET}

clean:
	-rm ${TARGET} ${OFILES}

MACROS = c.m4.marcel

%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

%h: %.hm4
	m4 ${MACROS} $*.hm4 > $*.h

%.c: %.cm4
	m4 $(MACROS) $*.cm4 > $*.c

