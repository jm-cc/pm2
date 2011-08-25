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

AC_INIT(MaMI,
	1.0.0,
	http://gforge.inria.fr/projects/pm2/,
	mami)

AC_PREFIX_DEFAULT(/opt/memory)
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
AM_INIT_AUTOMAKE(1.9 [-Wall -Werror foreign color-tests])

# Check for usual build tools
AC_PROG_CPP
AC_PROG_CC
AM_PROG_CC_C_O
LT_INIT
AC_PROG_INSTALL

# Restore user CFLAGS/LDFLAGS
POP_BUILDENV

# Dependencies
DEP_TBX_WITH_TOPO(1.0)
CHECK_HWLOC_PKG

AC_ARG_ENABLE(marcel, [AS_HELP_STRING([--disable-marcel], [do not use Marcel])],, [enable_marcel=maybe])

if test x$enable_marcel = xyes -o x$enable_marcel = xmaybe; then
	# do we have a valid marcel setup ?
	PKG_CHECK_EXISTS(marcel, have_valid_marcel=yes, have_valid_marcel=no)

        AC_MSG_CHECKING(whether Marcel is available)
        AC_MSG_RESULT($have_valid_marcel)

        if test x$have_valid_marcel = xyes ; then
	    # check if marcel is build with MaMI support
            pkg-config marcel --cflags | grep -q MARCEL_MAMI_ENABLED
	    if test $? -ne 0; then
                AC_MSG_NOTICE(Marcel is NOT build with MaMI support. It will be disabled.)
                have_valid_marcel=no
	    fi
        fi

	# in case Marcel was explicitely required, but is not available, this is an error
	if test x$enable_marcel = xyes -a x$have_valid_marcel = xno; then
		AC_MSG_ERROR([cannot find a proper installation of Marcel])
	fi

	# now we enable marcel if and only if a proper setup is available
	enable_marcel=$have_valid_marcel
fi

AC_MSG_CHECKING(whether Marcel should be used)
AC_MSG_RESULT($enable_marcel)
AM_CONDITIONAL(MAMI_USE_MARCEL, test x$enable_marcel = xyes)

if test x$enable_marcel = xyes; then
    ADD_TO(GLOBAL_AM_CFLAGS, `pkg-config marcel --cflags`)
    ADD_TO(GLOBAL_AM_CPPFLAGS, -DMARCEL)
    ADD_TO(LIBS, `pkg-config marcel --libs`)
    ADD_TO(REQUIRED_PKG, marcel)
    AC_SUBST(MAMI_MARCEL_PREFIX, `pkg-config marcel --variable=prefix`)
fi

# Global build tool options
ADD_TO(GLOBAL_AM_CPPFLAGS, -DMAMI_ENABLED)
ADD_TO(GLOBAL_AM_CFLAGS, -std=gnu99 -Wall -Wmissing-prototypes)
ADD_TO(GLOBAL_AM_LDFLAGS, $LDFLAGS)

# Option declarations
OPTION_ENABLE(coverage, enables the coverage in this library, whether the library will be covered)
OPTION_ENABLE(debug, builds the library in debug mode, whether the library will be built in debug mode)
OPTION_ENABLE(gprof, instruments the code for gprof, whether the code will be instrumented for gprof)
OPTION_DISABLE(inline, does not inline function code, whether inline optimisation will be enabled)
OPTION_ENABLE(optimize, enables compiler optimisations, whether the library will be optimised)

# Check dependencies
OPTION_CHECK_DEPEND(profile,,)
OPTION_CHECK_DEPEND(debug,, optimize)
OPTION_CHECK_DEPEND(coverage, debug,)
OPTION_CHECK_DEPEND(gprof,,)
OPTION_CHECK_DEPEND(inline,, optimize)

# Option activations
OPTION_APPLY(coverage, [ ADD_TO(GLOBAL_AM_CFLAGS, -O0 --coverage) ADD_TO(GLOBAL_AM_LDFLAGS, --coverage) ])
OPTION_APPLY(debug, [ ADD_TO(GLOBAL_AM_CPPFLAGS, -DMEMORY_DEBUG) ADD_TO(GLOBAL_AM_CFLAGS, -gdwarf-2 -g3 -fno-omit-frame-pointer) ])
OPTION_APPLY(gprof, [ ADD_TO(GLOBAL_AM_CFLAGS, -pg -fno-omit-frame-pointer) ADD_TO(GLOBAL_AMD_LDFLAGS, [-Wl,-pg -Wl,-fno-omit-frame-pointer]) ])
OPTION_APPLY(inline, [ ADD_TO(GLOBAL_AM_CFLAGS, -fno-inline) ])
OPTION_APPLY(optimize, [ ADD_TO(GLOBAL_AM_CFLAGS, -O2) ADD_TO(GLOBAL_AM_LDFLAGS, [-Wl,-O2 -Wl,-s]) ])

# Global build tool options
AC_SUBST(GLOBAL_AM_CPPFLAGS)
AC_SUBST(GLOBAL_AM_CFLAGS)
AC_SUBST(GLOBAL_AM_LDFLAGS)
AC_SUBST(GLOBAL_AM_LDADD)

# test fortran compiler
AC_PROG_FC
if test "x${FC}" = x; then
    __opt_fortran_compiler=no
else
    __opt_fortran_compiler=yes
    # check if tbx was built with Fortran support
    pkg-config tbx --cflags | grep -q FORTRAN_COMPILER
    __tbx_have_fortran=$?
    if test $__tbx_have_fortran -eq 0; then
        ADD_TO(GLOBAL_AM_CPPFLAGS, -DMAMI_FORTRAN)
	   __opt_fortran_compiler=yes
    else
	__opt_fortran_compiler=no
	AC_MSG_NOTICE(tbx does not provide a Fortran interface)
    fi
fi
AC_MSG_CHECKING(MaMI will be built with the Fortran interface)
AC_MSG_RESULT($__opt_fortran_compiler)
AM_CONDITIONAL(HAVE_FORTRAN_COMPILER, test $__opt_fortran_compiler = yes)

AC_CONFIG_COMMANDS([executable-scripts], [chmod +x ]tools/mami_runSamplingForMemoryAccess.sh tools/mami_runSamplingForMemoryMigration.sh tools/mami_analyzeSamplingForMemoryMigration.pl)

# Here is the list of Automake files
AC_CONFIG_FILES(Makefile
		pkgconfig/mami.pc
		source/Makefile
                tests/Makefile
                tools/Makefile
                benchmarks/Makefile
                benchmarks/helpers/mami_bench_launcher
		examples/Makefile
                tools/mami_runSamplingForMemoryAccess.sh
                tools/mami_runSamplingForMemoryMigration.sh
                tools/mami_analyzeSamplingForMemoryMigration.pl)

AC_OUTPUT
