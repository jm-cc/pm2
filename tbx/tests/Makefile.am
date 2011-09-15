#
# PM2: Parallel Multithreaded Machine
# Copyright (C) 2010 "the PM2 team" (see AUTHORS file)
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
#


AM_CPPFLAGS = $(GLOBAL_AM_CPPFLAGS) $(PKG_AM_CPPFLAGS) -I$(abs_top_srcdir)/include
AM_CFLAGS   = $(GLOBAL_AM_CFLAGS)
AM_LDFLAGS  = $(GLOBAL_AM_LDFLAGS) $(PKG_AM_LDFLAGS)
LDADD       = $(GLOBAL_AM_LDADD) $(abs_top_builddir)/source/libtbx.la


check_PROGRAMS = 	tbx_malloc tbx_slist tbx_program_argument tbx_checksum tbx_snprintf \
			tbx_topology tbx_string_management
TESTS = $(check_PROGRAMS)
