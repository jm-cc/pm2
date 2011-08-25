# MaMI --- NUMA Memory Interface
#
# Copyright (C) 2008, 2009, 2010, 2011  Centre National de la Recherche Scientifique
#
# MaMI is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or (at
# your option) any later version.
#
# MaMI is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# See the GNU Lesser General Public License in COPYING.LGPL for more details.

AM_CPPFLAGS  = -DDEBUG -I $(abs_top_srcdir)/include -I $(abs_top_builddir)/include $(GLOBAL_AM_CPPFLAGS) $(INTERNAL_AM_CPPFLAGS)
AM_CFLAGS    = $(GLOBAL_AM_CFLAGS)
AM_LDFLAGS   = $(GLOBAL_AM_LDFLAGS)

LIBS = $(GLOBAL_AM_LDADD) $(top_builddir)/source/libmami.la @LIBS@

bin_PROGRAMS =

SUBDIRS =

if MAMI_USE_MARCEL
TESTS_ENVIRONMENT 	= $(MAMI_MARCEL_PREFIX)/bin/marcel-exec
endif

TESTS = $(check_PROGRAMS)

EXTRA_DIST =				\
	marcel_stream.h

#	saxpy
#	sgemm

check_PROGRAMS =

#	crossing_access
#	membind
#	memteams

if MAMI_USE_MARCEL
check_PROGRAMS +=		\
	mami_jacobi		\
	mami_memteams_bubbles	\
	mami_stream
endif


bin_PROGRAMS	+=	$(check_PROGRAMS)
