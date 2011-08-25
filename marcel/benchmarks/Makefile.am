#
# PM2
# Copyright (C) Université Bordeaux 1, CNRS 2008-2009 (see AUTHORS file)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# See the GNU Lesser General Public License in COPYING.LGPL for more details.
#

AM_CPPFLAGS  = -DDEBUG -imacros $(abs_top_builddir)/config.h -I $(abs_top_srcdir)/include -I $(abs_top_builddir)/include $(GLOBAL_AM_CPPFLAGS) $(PKG_AM_CPPFLAGS) $(TBX_CFLAGS)
AM_CFLAGS    = $(GLOBAL_AM_CFLAGS)
AM_LDFLAGS   = $(GLOBAL_AM_LDFLAGS) $(TBX_LIBS)  -no-install
if IS_LIBPTHREAD
  LDADD = $(abs_top_builddir)/source/libpthread.la $(GLOBAL_AM_LDADD)
else
if IS_LIBPMARCEL
  LDADD = $(abs_top_builddir)/source/libpmarcel.la $(GLOBAL_AM_LDADD)
else
  LDADD = $(abs_top_builddir)/source/libmarcel.la  $(GLOBAL_AM_LDADD)
endif
endif

noinst_HEADERS	=	tsp-job.h tsp-types.h tsp.h
check_PROGRAMS	=	cond contsw create createtasks createseeds pingpong sumtime tsp
#  TODO: fft
BENCH_LIST	=	$(check_PROGRAMS) semaphore

bench: $(check_PROGRAMS)
	mkdir -p launchers
	chmod +x ./helpers/marcel_bench_launcher
	for f in $(BENCH_LIST) ; do \
		(cd launchers && ln -sf ../helpers/marcel_bench_launcher $$f) && LD_BIND_NOW=y launchers/$$f ; \
	done

## tsp test is cut in multiple files
tsp_SOURCES   = tsp.c tsp-main.c tsp-job.c
tsp_LDADD     = $(LDADD) -lm
