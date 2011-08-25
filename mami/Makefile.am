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

ACLOCAL_AMFLAGS = -I ./m4
SUBDIRS = source tests tools examples benchmarks

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pkgconfig/mami.pc

include_HEADERS	=
include_HEADERS	+=	include/mami_debug.h
include_HEADERS	+=	include/mami_list.h
include_HEADERS	+=	include/mami.h
include_HEADERS	+=	include/mami_marcel_private.h
include_HEADERS	+=	include/mami_thread.h
include_HEADERS	+=	include/mami_helper.h
include_HEADERS	+=	include/mami_fortran.h
include_HEADERS	+=	include/mami_marcel.h
include_HEADERS	+=	include/mami_private.h
include_HEADERS	+=	include/mami_valgrind.h


EXTRA_DIST = autogen.sh

bench: ; $(MAKE) -C benchmarks bench
