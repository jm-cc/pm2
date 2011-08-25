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

SUBDIRS =

if MAMI_USE_MARCEL
TESTS_ENVIRONMENT 	= $(MAMI_MARCEL_PREFIX)/bin/marcel-exec
endif

TESTS = $(check_PROGRAMS)

EXTRA_DIST =				\
	helper.h

check_PROGRAMS	=

if MAMI_USE_MARCEL
if HAVE_FORTRAN_COMPILER
attach_mamif_SOURCES	=	attach_mamif.f90
attach_migratef_SOURCES	=	attach_migratef.f90
check_PROGRAMS +=			\
	attach_mamif			\
	attach_migratef
endif
endif

check_PROGRAMS	+=			\
	attach_and_migrate		\
	attach				\
	attach_distribute		\
	attach_firsttouch		\
	attach_firsttouch_migrate	\
	attach_malloc			\
	attach_multiple			\
	attach_nexttouch		\
	attach_register			\
	attach_split2			\
	attach_split			\
	attach_stack			\
	attach_stats

check_PROGRAMS +=	\
	bigmalloc2			\
	bigmalloc			\
	check_location			\
	check_migration			\
	cpuset				\
	distribute2			\
	distribute3			\
	distribute			\
	firsttouch2			\
	firsttouch_migrate2		\
	fragment			\
	hugepages			\
	localvsrandom2			\
	localvsrandom			\
	malloc				\
	memory				\
	memstats			\
	migrate_and_access		\
	migration_and_access_cost	\
	migration			\
	migration_error			\
	mlocate				\
	next_touch2			\
	next_touch			\
	next_touch_error		\
	next_touch_kernel		\
	next_touch_multiple		\
	register			\
	select_node			\
	unclean

localvsrandom_LDADD 		=	-lpthread
localvsrandom2_LDADD 		=	-lpthread
memory_LDADD 			=	-lpthread
migrate_and_access_LDADD	=	-lpthread
migration_LDADD			=	-lpthread
next_touch2_LDADD 		=	-lpthread
next_touch_LDADD 		=	-lpthread
next_touch_kernel_LDADD 	=	-lpthread
next_touch_multiple_LDADD 	=	-lpthread
