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


AM_CPPFLAGS = 	-I$(abs_top_srcdir)/bubblelib/include $(GLOBAL_AM_CPPFLAGS) $(INTERNAL_CPPFLAGS)\
		$(TBX_CFLAGS) $(FXT_CFLAGS) $(LIBMING_CFLAGS)
AM_CFLAGS   = $(GLOBAL_AM_CFLAGS)
AM_LDFLAGS  = $(GLOBAL_AM_LDFLAGS) $(TBX_LIBS) $(FXT_LIBS) $(LIBMING_LIBS)
LDADD       = $(GLOBAL_AM_LDADD)


include_HEADERS = include/bubblelib_anim.h include/bubblelib_fxt.h include/bubblelib_output.h

lib_LTLIBRARIES = libbubble.la
libbubble_la_SOURCES = source/bubblelib_anim.c source/bubblelib_fxt.c source/bubblelib_ming.c