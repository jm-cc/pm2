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


# IS_GNU_BUILDCHAIN()
# -------------------
# Check if $CC points to the GNU cc tool and if $LD points to the GNU ld tool
# * adds -Wno-clobbered if version >= 4.3.0 
# * adds GNU CC specific warning flags
AC_DEFUN([IS_GNU_BUILDCHAIN],
[
	AC_REQUIRE([AC_PROG_CC])
	if test "x$GCC" = "xyes"; then
	   ADD_TO(GLOBAL_AM_CFLAGS, -Wextra -Wstrict-aliasing=2 -Wundef -Werror-implicit-function-declaration)

	   AC_COMPUTE_INT([GCC_MAJOR_VERSION], [__GNUC__])
	   AC_COMPUTE_INT([GCC_MINOR_VERSION], [__GNUC_MINOR__])
	   if test $GCC_MAJOR_VERSION -ge 4 && test $GCC_MINOR_VERSION -ge 3; then
	      ADD_TO(GLOBAL_AM_CFLAGS, -Wno-clobbered)
	   fi

	   AC_MSG_CHECKING([whether GNU ld is being used])
	   if $LD -v 2>&1 | grep -qi gnu; then
	      AC_MSG_RESULT(yes)
	   else
	      AC_MSG_RESULT(no)
	      AC_MSG_ERROR([Please use GNU ld.])
	   fi
	fi
])


# IS_INTEL_BUILDCHAIN()
# ---------------------
# Check if $CC points to the Intel cc tool and if $LD points to the Intel cc tool
# * disables #981: expression order evaluation
# * disables #1419: declaration in source code
# * disables #187: allow if (test == (toto = titi())
# * adds #266: change implicit declaration remark in to error (for CHECK_PROTO_ONLY_*)
AC_DEFUN([IS_INTEL_BUILDCHAIN],
[
	AC_MSG_CHECKING(if CC is the Intel C compiler)
	if $CC -help 2>&1 | grep -qi intel; then
	   ADD_TO(GLOBAL_AM_CFLAGS, -wd271 -wd981 -wd1419 -wd187)
	   ADD_TO(CFLAGS, -we266)

	   if $LD -help 2>&1 | grep -qi intel; then
	      AC_MSG_RESULT(yes)
	   else
	      PRINT_ERROR("Please use Intel compiler for both step (build and link)")
	   fi
        else
	   AC_MSG_RESULT(no)
        fi
])


# BUILDCHAIN_SUPPORT_MAVX()
# -------------------------
# Check if the buildchain -mavx switch
AC_DEFUN([BUILDCHAIN_SUPPORT_MAVX],
[
	AC_MSG_CHECKING(if the buildchain support -mavx switch)
	if $CC -mavx -xc /dev/null -S -o /dev/null 2>/dev/null; then
	   AC_DEFINE([HAVE_AVX_SUPPORT], [1], [Define to 1 if the buildchain support -mavx switch])
	   AC_MSG_RESULT(yes)
	else
	   AC_MSG_RESULT(no)
	fi
])	
