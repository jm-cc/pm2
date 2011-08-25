#
# PM2: Parallel Multithreaded Machine
# Copyright (C) 2010, 2011 "the PM2 team" (see AUTHORS file)
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


ACLOCAL_AMFLAGS = -I m4
SUBDIRS = include source tests benchmarks


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pkgconfig/marcel.pc

EXTRA_DIST = 	$(srcdir)/scripts/linker/arch-i386/libpthread.map \
		$(srcdir)/scripts/linker/arch-ia64/libpthread.map \
		$(srcdir)/scripts/linker/arch-x86_64/libpthread.map \
		$(srcdir)/autogen.sh \
		$(gdbinit_files)

bin_SCRIPTS = scripts/marcel-exec
pkgdata_DATA = $(gdbinit_files)


bench:
	$(MAKE) -C benchmarks bench
