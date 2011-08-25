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

AC_INIT(PthreadMicroBench,
	0.1,
	http://runtime.bordeaux.inria.fr/marcel/,
	pthreadmicro)
AC_PREREQ(2.59)

AC_PREFIX_DEFAULT(/opt/pthreadmicro)
AC_CONFIG_SRCDIR(./configure.ac)
AC_CONFIG_AUX_DIR(./build-aux)
AC_CONFIG_MACRO_DIR([m4])

# Preserve order options in help
m4_ifdef([AC_PRESERVE_HELP_ORDER], [AC_PRESERVE_HELP_ORDER])

# Make the build output less verbose
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Activate Automake support
AM_INIT_AUTOMAKE([foreign])

# Check for usual build tools
AC_LANG([C])
AC_PROG_CPP
AC_PROG_CC
AC_PROG_LN_S
LT_INIT
AC_PROG_INSTALL

### automake-1.9 does not substitute these two by default
AC_SUBST([abs_top_srcdir])
AC_SUBST([abs_top_builddir])

# Automake outputs
AC_CONFIG_FILES(Makefile)
AC_OUTPUT
