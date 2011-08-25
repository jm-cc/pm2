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


AM_CPPFLAGS = -I$(abs_top_srcdir)/bubblelib/include $(GLOBAL_AM_CPPFLAGS) $(INTERNAL_CPPFLAGS) $(TBX_CFLAGS) $(FXT_CFLAGS)
AM_CFLAGS   = $(GLOBAL_AM_CFLAGS)
AM_LDFLAGS  = $(GLOBAL_AM_LDFLAGS) $(TBX_LIBS) $(FXT_LIBS)
LDADD       = $(GLOBAL_AM_LDADD) $(abs_top_builddir)/bubblelib/libbubble.la


bin_PROGRAMS = bubbles
bubbles_SOURCES = bubbles.c