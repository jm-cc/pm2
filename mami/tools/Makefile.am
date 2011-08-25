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

toolsbindir = $(prefix)/bin/tools/

toolsbin_PROGRAMS =				\
	mami_sampling_for_memory_access		\
	mami_sampling_for_memory_migration

toolsbin_SCRIPTS	=				\
	mami_analyzeSamplingForMemoryMigration.pl	\
	mami_runSamplingForMemoryAccess.sh 		\
	mami_runSamplingForMemoryMigration.sh

