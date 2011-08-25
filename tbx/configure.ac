
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

AC_INIT(ToolBoX,
	1.0.1,
	http://gforge.inria.fr/projects/pm2/,
	tbx)
AC_PREREQ(2.59)


AC_PREFIX_DEFAULT(/opt/tbx)
AC_CONFIG_SRCDIR(./configure.ac)
AC_CONFIG_AUX_DIR(./build-aux)
AC_CONFIG_MACRO_DIR(./m4)


# Preserve order options in help
m4_ifdef([AC_PRESERVE_HELP_ORDER], [AC_PRESERVE_HELP_ORDER])


# Make the build output less verbose
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])


# Get ARCH & SYS specifications
TARGET_SYS_ARCH


# Save user CFLAGS/LDFLAGS
PUSH_BUILDENV


# Activate Automake support
AM_INIT_AUTOMAKE([foreign])


# Check for usual build tools
AC_PROG_CPP
AC_PROG_CC
AC_PROG_FC
AC_PROG_LIBTOOL
CHECK__FORTRAN_COMPILER
CHECK__C_COMPILER
PKG_PROG_PKG_CONFIG
LT_INIT
AC_PROG_INSTALL


### automake-1.9 does not substitute these two by default
AC_SUBST([abs_top_srcdir])
AC_SUBST([abs_top_builddir])


# Check prototype and function availability ...
TBX__TIMING_METHOD


# Restore user CFLAGS/LDFLAGS
POP_BUILDENV


# Global build tool options
ADD_TO(PKG_AM_CPPFLAGS, -DTBX)
ADD_TO(GLOBAL_AM_CPPFLAGS, -D_GNU_SOURCE)
ADD_TO(GLOBAL_AM_LDFLAGS, $LDFLAGS)
ADD_TO(GLOBAL_AM_CFLAGS, $CFLAGS)


# Option declarations
OPTION_ENABLE(coverage, enables the coverage in this library, whether the library will be covered)
OPTION_ENABLE(profile, enables the profiling in this library, whether the library will be profiled)
OPTION_ENABLE(gcc_instrument, auto-profiles all functions of this module, whether library functions will be profiled)
OPTION_ENABLE(debug, builds the library in debug mode, whether the library will be built in debug mode)
OPTION_ENABLE(gprof, instruments the code for gprof, whether the code will be instrumented for gprof)
OPTION_DISABLE(inline, does not inline function code, whether inline optimisation will be enabled)
OPTION_ENABLE(optimize, enables compiler optimisations, whether the library will be optimised)
OPTION_ENABLE(strip, enables the linker strip operation, whether the library will be striped)
OPTION_DISABLE(fastalloc, disable fast allocator, whether to use fast allocator)


# Check dependencies
OPTION_CHECK_DEPEND(gcc_instrument, profile,)
OPTION_CHECK_DEPEND(coverage, debug,)
OPTION_CHECK_DEPEND(strip,, profile gcc_instrument debug inline coverage gprof)


# Option activations
OPTION_APPLY(coverage, [ ADD_TO(GLOBAL_AM_CFLAGS, -O0 --coverage) ADD_TO(GLOBAL_AM_LDFLAGS, --coverage) ])
OPTION_APPLY(profile, ADD_TO(PKG_AM_CPPFLAGS, -DDO_PROFILE))
OPTION_APPLY(gcc_instrument, [ ADD_TO(PKG_AM_CPPFLAGS, -DPM2_INSTRUMENT) ADD_TO(GLOBAL_AM_CFLAGS, -finstrument-functions) ])
OPTION_APPLY(debug, [ ADD_TO(PKG_AM_CPPFLAGS, -DPM2DEBUG -DTBX_DEBUG) 
		      ADD_TO(GLOBAL_AM_CFLAGS, -gdwarf-2 -g3 -fno-omit-frame-pointer) ])
OPTION_APPLY(gprof, [ ADD_TO(GLOBAL_AM_CFLAGS, -pg -fno-omit-frame-pointer) ADD_TO(GLOBAL_AM_LDFLAGS, [-Wl,-pg]) ])
OPTION_APPLY(inline, [ ADD_TO(PKG_AM_CPPFLAGS, -DPM2_NOINLINE) ADD_TO(GLOBAL_AM_CFLAGS, -fno-inline) ])
OPTION_APPLY(optimize, [ ADD_TO(PKG_AM_CPPFLAGS, -DPM2_OPT -DNDEBUG)
		       	 ADD_TO(GLOBAL_AM_CPPFLAGS, -DNDEBUG)
		       	 ADD_TO(GLOBAL_AM_CFLAGS, -O2)
			 ADD_TO(GLOBAL_AM_LDFLAGS, [-Wl,-O2]) ])
OPTION_APPLY(strip, [ ADD_TO(GLOBAL_AM_LDFLAGS, [-Wl,-s]) ])
OPTION_APPLY(fastalloc, [ ADD_TO(PKG_AM_CPPFLAGS, -DTBX_DISABLE_FASTALLOC) ])


# PM2 profile
CHECK_HWLOC_PKG


# Global build tool options
AC_SUBST(PKG_AM_CPPFLAGS)
AC_SUBST(PKG_AM_LDFLAGS)
AC_SUBST(GLOBAL_AM_CPPFLAGS)
AC_SUBST(GLOBAL_AM_CFLAGS)
AC_SUBST(GLOBAL_AM_LDFLAGS)
AC_SUBST(GLOBAL_AM_LDADD)


# tbx.pc => package dependency list
AC_SUBST(REQUIRED_PKG)


# Here is the list of Automake files
AC_CONFIG_FILES(Makefile
		source/Makefile
		tests/Makefile
		pkgconfig/tbx.pc)

AC_OUTPUT
