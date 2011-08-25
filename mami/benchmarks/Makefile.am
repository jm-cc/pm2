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

noinst_PROGRAMS	=	mami_bench
BENCH_LIST	=	$(noinst_PROGRAMS)

if MAMI_USE_MARCEL
MAMI_EXEC	= $(MAMI_MARCEL_PREFIX)/bin/marcel-exec
else
MAMI_EXEC	=
endif

bench: $(BENCH_LIST)
	mkdir -p launchers
	chmod +x ./helpers/mami_bench_launcher
	for f in $(BENCH_LIST) ; do \
		(cd launchers && ln -sf ../helpers/mami_bench_launcher $$f) && $(MAMI_EXEC) launchers/$$f ; \
	done
