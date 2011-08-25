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


# CHECK_HWLOC_PKG
# ---------------
AC_DEFUN([CHECK_HWLOC_PKG],
[
	PKG_CHECK_MODULES(HWLOC, hwloc, __with_hwloc=yes, __with_hwloc=no)
	if test x$__with_hwloc = xyes; then
		ADD_TO(PKG_AM_CPPFLAGS, -DPM2_TOPOLOGY)
		ADD_TO(GLOBAL_AM_CFLAGS, `pkg-config hwloc --cflags`)
		ADD_TO(LIBS, `pkg-config hwloc --libs`)
		ADD_TO(REQUIRED_PKG, hwloc)
	fi

	AC_MSG_NOTICE(whether hwloc is available: $__with_hwloc)
])


# CHECK__FORTRAN_COMPILER
# -----------------------
# Check if we have a Fortran compiler
AC_DEFUN([CHECK__FORTRAN_COMPILER],
[
	AC_REQUIRE([AC_PROG_FC])

	if test "x${FC}" != x; then
	   # Gnu Fortran compiler ?
	   AC_MSG_CHECKING([whether GNU Fortran compiler is being used])
	   if $FC -v 2>&1 | grep -i gnu >/dev/null 2>&1; then
	      ADD_TO(PKG_AM_CPPFLAGS, -DGNU_FORTRAN_COMPILER)
	      AC_MSG_RESULT(yes)
	   else
	      AC_MSG_RESULT(no)
	   fi

	   # Intel Fortran compiler ?
	   AC_MSG_CHECKING([whether Intel Fortran compiler is being used])
	   if $FC -v 2>&1 | grep -i intel >/dev/null 2>&1; then
	      ADD_TO(PKG_AM_CPPFLAGS, -DINTEL_FORTRAN_COMPILER)
	      AC_MSG_RESULT(yes)
	   else
	      AC_MSG_RESULT(no)
           fi
	else
	   PRINT_WARNING(tbx will be built with no Fortran support)
	fi
])


# TBX__TIMING_METHOD
# ------------------
# Check if we have clock_gettime() or mach_absolute_time() [Darwin system] (default=gettimeofday)
AC_DEFUN([TBX__TIMING_METHOD],
[
	AC_CHECK_DECLS([clock_gettime],
	[
		AC_SEARCH_LIBS([clock_gettime], 
          		       [c, rt], 
			       [ 
					ADD_TO(PKG_AM_CPPFLAGS, -DUSE_CLOCK_GETTIME)
					ADD_TO(PKG_AM_LDFLAGS, $LIBS) 
			       ],
			       [], [])
	], 
	[
		AC_CHECK_DECLS([mach_absolute_time],
		[
			ADD_TO(PKG_AM_CPPFLAGS, -DUSE_MACH_ABSOLUTE_TIME)
		], [],
		[[
			#include <mach/mach_time.h>
		]])
	],
	[[
		#include <time.h>
	]])
])
