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

AC_INIT(PM2/Profile,
	1.0.0,
	http://gforge.inria.fr/projects/pm2/,
	profile)
AC_PREREQ(2.59)
LT_PREREQ([2.2.6])
PKG_PROG_PKG_CONFIG([0.18])


AC_PREFIX_DEFAULT(/opt/profile)
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
AM_INIT_AUTOMAKE([foreign color-tests])


# Check for usual build tools
AC_PROG_CPP
AC_PROG_CC
LT_INIT
AC_PROG_INSTALL
IS_GNU_BUILDCHAIN
IS_INTEL_BUILDCHAIN

### automake-1.9 does not substitute these two by default
AC_SUBST([abs_top_srcdir])
AC_SUBST([abs_top_builddir])


# Restore user CFLAGS/LDFLAGS
POP_BUILDENV


# Global build tool options
ADD_TO(GLOBAL_AM_CPPFLAGS, -DPROFILE)
ADD_TO(GLOBAL_AM_CFLAGS, -std=gnu99 -Wall -Wmissing-prototypes -fvisibility=hidden)
ADD_TO(GLOBAL_AM_LDFLAGS, $LDFLAGS)


# Option declarations
OPTION_ENABLE(coverage, enables the coverage in this library, whether the library will be covered)
OPTION_ENABLE(profile, enables the profiling in this library, whether the library will be profiled)
OPTION_ENABLE(gcc_instrument, auto-profiles all functions of this module, whether library functions will be profiled)
OPTION_ENABLE(debug, builds the library in debug mode, whether the library will be built in debug mode)
OPTION_ENABLE(gprof, instruments the code for gprof, whether the code will be instrumented for gprof)
OPTION_DISABLE(inline, does not inline function code, whether inline optimisation will be enabled)
OPTION_ENABLE(optimize, enables compiler optimisations, whether the library will be optimised)
OPTION_DISABLE(implicit_profiling, disables the profiling on LOG_IN / LOG_OUT, whether LOG_IN / LOG_OUT will not be profiled)
OPTION_ENABLE(autostart, starts profiling automatically, whether profiling will be started automatically)


# Check dependencies
OPTION_CHECK_DEPEND(profile,,)
OPTION_CHECK_DEPEND(gcc_instrument, profile, optimize)
OPTION_CHECK_DEPEND(debug,, optimize)
OPTION_CHECK_DEPEND(coverage, debug,)
OPTION_CHECK_DEPEND(gprof,,)
OPTION_CHECK_DEPEND(inline,, optimize)
OPTION_CHECK_DEPEND(optimize,, gcc_instrument debug inline)
OPTION_CHECK_DEPEND(fkt, linux,)


# Option activations
OPTION_APPLY(coverage, [ ADD_TO(GLOBAL_AM_CFLAGS, -O0 --coverage) ADD_TO(GLOBAL_AM_LDFLAGS, --coverage) ])
OPTION_APPLY(profile, ADD_TO(GLOBAL_AM_CPPFLAGS, -DDO_PROFILE))
OPTION_APPLY(gcc_instrument, [ ADD_TO(GLOBAL_AM_CPPFLAGS, -DPM2_INSTRUMENT) ADD_TO(GLOBAL_AM_CFLAGS, -finstrument-functions) ])
OPTION_APPLY(debug, [ ADD_TO(GLOBAL_AM_CPPFLAGS, -DPM2DEBUG -DTBX_DEBUG) ADD_TO(GLOBAL_AM_CFLAGS, -gdwarf-2 -g3 -fno-omit-frame-pointer) ])
OPTION_APPLY(gprof, [ ADD_TO(GLOBAL_AM_CFLAGS, -pg -fno-omit-frame-pointer) ADD_TO(GLOBAL_AMD_LDFLAGS, [-Wl,-pg -Wl,-fno-omit-frame-pointer]) ])
OPTION_APPLY(inline, [ ADD_TO(GLOBAL_AM_CPPFLAGS, -DPM2_NOINLINE) ADD_TO(GLOBAL_AM_CFLAGS, -fno-inline) ])
OPTION_APPLY(optimize, [ ADD_TO(GLOBAL_AM_CPPFLAGS, -DPM2_OPT -DNDEBUG) ADD_TO(GLOBAL_AM_CFLAGS, -O2) ADD_TO(GLOBAL_AM_LDFLAGS, [-Wl,-O2 -Wl,-s]) ])
OPTION_APPLY(implicit_profiling, ADD_TO(GLOBAL_AM_CPPFLAGS, -DNO_IMPLICIT_PROFILE))
OPTION_APPLY(autostart, ADD_TO(GLOBAL_AM_CPPFLAGS, -DPROFILE_AUTOSTART))


# Check dependencies
PKG_CHECK_MODULES(TBX, tbx, [ ADD_TO(REQUIRED_PKG, tbx) ])
PKG_CHECK_MODULES(FXT, fxt >= 0.2, [ ADD_TO(REQUIRED_PKG, fxt) ])

# Global build tool options
AC_SUBST(INTERNAL_CPPFLAGS)
AC_SUBST(GLOBAL_AM_CPPFLAGS)
AC_SUBST(GLOBAL_AM_CFLAGS)
AC_SUBST(GLOBAL_AM_LDFLAGS)
AC_SUBST(GLOBAL_AM_LDADD)


# profile.pc => package dependency list
AC_SUBST(REQUIRED_PKG)


# Here is the list of Automake files
AC_CONFIG_FILES(Makefile
		source/Makefile
		pkgconfig/profile.pc)

AC_OUTPUT
