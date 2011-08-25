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


ACLOCAL_AMFLAGS = -I ./m4
SUBDIRS = source tests


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pkgconfig/tbx.pc


include_HEADERS=include/tbx.h include/tbx_debug.h include/tbx_htable.h include/tbx_log.h 	\
	include/tbx_slist_inline.h include/tbx_topology.h include/tbx_string.h			\
	include/tbx_compiler.h include/tbx_fast_list.h include/tbx_interface.h 			\
	include/tbx_macros.h include/tbx_pointers.h include/tbx_snprintf.h include/tbx_types.h 	\
	include/tbx_darray.h include/tbx_fortran_argument.h include/tbx_list.h 			\
	include/tbx_malloc.h include/tbx_program_argument.h include/tbx_checksum.h 		\
	include/tbx_darray_inline.h include/tbx_hooks.h include/tbx_list_inline.h 		\
	include/tbx_malloc_inline.h include/tbx_slist.h include/tbx_timing.h


EXTRA_DIST = autogen.sh
