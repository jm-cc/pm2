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


# IS_SUPPORTED_CFLAG(flag)
# ------------------------
# Check if the CFLAGS `flag' is supported by the compiler
AC_DEFUN([IS_SUPPORTED_CFLAG],
[
	AC_REQUIRE([AC_PROG_CC])
	
	AC_MSG_CHECKING([whether compiler support $1])

	__OLD_CFLAGS=$CFLAGS
	CFLAGS="$1 $__OLD_CFLAGS"

	AC_COMPILE_IFELSE(
		AC_LANG_PROGRAM(
			[[]],
			[[
				AC_LANG_SOURCE([
					const char *hello = "Hello World";
				])
			]]
		),
		[ 
			ADD_TO(GLOBAL_AM_CFLAGS, $1) AC_MSG_RESULT(yes)
		],
			AC_MSG_RESULT(no)
	)

	CFLAGS="$__OLD_CFLAGS"
])


# IS_SUPPORTED_VENDOR_CFLAG(flag, vendor-list)
# --------------------------------------------
# Check if CC_VENDROR is in vendor-list before issuing IS_SUPPORTED_CFLAGS
AC_DEFUN([IS_SUPPORTED_VENDOR_CFLAG],
[
	for v in $2; do
	    if test $CC_VENDOR = $v; then
	       IS_SUPPORTED_CFLAG($1)
	       break;
	    fi
	done
])


# COMPILER_VENDOR
# ---------------
# Check if $CC points to the GNU cc tool or Intel cc tool
# * disables #981: expression order evaluation
# * disables #1419: declaration in source code
# * disables #187: allow if (test == (toto = titi())
# * adds #266: change implicit declaration remark into error (for CHECK_PROTO_ONLY_*)
# * adds #10006: change unsupported option warning into error
# * adds #10156: change no Werror argument required warning into error
# * adds #10159: change -march support into error
AC_DEFUN([COMPILER_VENDOR],
[
	AC_REQUIRE([AC_PROG_CC])
	AC_MSG_CHECKING(CC compiler vendor)
	
	if $CC -help 2>&1 | grep -i intel >/dev/null 2>&1; then
	   ADD_TO(GLOBAL_AM_CFLAGS, -wd187)
	   ADD_TO(GLOBAL_AM_CFLAGS, -wd981)
	   ADD_TO(GLOBAL_AM_CFLAGS, -wd1419)
	   ADD_TO(CFLAGS, -we266)
	   ADD_TO(CFLAGS, -we10006)
	   ADD_TO(CFLAGS, -we10156)
	   ADD_TO(CFLAGS, -we10159)
	   CC_VENDOR="intel"
	elif $CC --version 2>&1 | grep -i clang >/dev/null 2>&1; then
	   CC_VENDOR="llvm"
	elif test "x$GCC" = "xyes"; then
	   CC_VENDOR="GNU"
	else
	   CC_VENDOR="unknow"
 	fi

	AC_MSG_RESULT($CC_VENDOR)
])


# BUILDCHAIN_SUPPORT_MAVX()
# -------------------------
# Check if the buildchain -mavx switch
AC_DEFUN([BUILDCHAIN_SUPPORT_MAVX],
[
	AC_MSG_CHECKING(whether the buildchain support -mavx switch)
	if $CC -mavx -xc /dev/null -S -o /dev/null 2>/dev/null; then
	   AC_DEFINE([HAVE_AVX_SUPPORT], [1], [Define to 1 if the buildchain support -mavx switch])
	   AC_MSG_RESULT(yes)
	else
	   AC_MSG_RESULT(no)
	fi
])	


# CHECK__C_COMPILER()
# -------------------
# Detect C compiler and check supported CFLAGS
AC_DEFUN([CHECK__C_COMPILER],
[
	# Gcc, Icc ?
	COMPILER_VENDOR

	# Check -mavx suport
	BUILDCHAIN_SUPPORT_MAVX

	# Check other flags
	IS_SUPPORTED_CFLAG(-g)
	IS_SUPPORTED_CFLAG(-fno-common)
	IS_SUPPORTED_CFLAG(-std=gnu99)
	IS_SUPPORTED_CFLAG(-Wstrict-aliasing=2 -fstrict-aliasing)
	IS_SUPPORTED_CFLAG(-Wundef)
	IS_SUPPORTED_CFLAG(-Werror-implicit-function-declaration)
	IS_SUPPORTED_CFLAG(-Wall)
	IS_SUPPORTED_CFLAG(-Wextra)
	IS_SUPPORTED_CFLAG(-Winit-self)
	IS_SUPPORTED_CFLAG(-Wpointer-arith)

	IS_SUPPORTED_VENDOR_CFLAG(-Wjump-misses-init, GNU intel)
	IS_SUPPORTED_VENDOR_CFLAG(-Wno-clobbered, GNU intel)
])
