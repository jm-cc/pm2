dnl -*- Autoconf -*-
dnl
dnl PM2: Parallel Multithreaded Machine
dnl Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or (at
dnl your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl General Public License for more details.

dnl ACX_GNU_C
dnl
dnl Make sure we're using GCC.
AC_DEFUN([ACX_GNU_C], [
  AC_REQUIRE([AC_PROG_CC])
  if test "x$GCC" != "xyes"; then
     AC_MSG_ERROR([Marcel must be built with the GNU C Compiler.])
  fi])

dnl ACX_GNU_C_MAJOR_VERSION
dnl
dnl Define output variable `GCC_MAJOR_VERSION' to the major number of
dnl the GCC version being used.
AC_DEFUN([ACX_GNU_C_MAJOR_VERSION], [
  AC_REQUIRE([ACX_GNU_C])
  AC_CACHE_CHECK([the major version of GNU C being used],
    [ac_cv_gcc_major_version],
    [AC_COMPUTE_INT([ac_cv_gcc_major_version], [__GNUC__])])

  GCC_MAJOR_VERSION="$ac_cv_gcc_major_version"
  AC_SUBST([GCC_MAJOR_VERSION])])


dnl ACX_LD_PATH
dnl
dnl Look for the linker and set $LD to its path.
AC_DEFUN([ACX_LD_PATH], [
  AC_CACHE_CHECK([where the linker is],
    [ac_cv_ld_path],
    [AC_REQUIRE([ACX_GNU_C])
     if test "x$GCC" = "xyes"; then
       ac_cv_ld_path="`$CC --print-prog-name=ld`"
     else
       AC_PATH_PROG([ac_cv_ld_path], [ld], [not-found])
       AC_MSG_ERROR([`ld' not found in \$PATH.])
     fi])
  LD="$ac_cv_ld_path"
  AC_SUBST([LD])])

dnl ACX_GNU_LD [ACTION-IF-GNU [ACTION-IF-NOT-GNU]]
dnl
dnl Check whether $LD is GNU ld.  Set $HAVE_GNU_LD to "yes" or "no".
AC_DEFUN([ACX_GNU_LD], [
  AC_REQUIRE([ACX_LD_PATH])
  AC_CACHE_CHECK([whether $LD is GNU ld],
    [ac_cv_gnu_ld],
    [if "$LD" --version | grep -q GNU 2>/dev/null; then
       ac_cv_gnu_ld="yes"
     else
       ac_cv_gnu_ld="no"
     fi])
  HAVE_GNU_LD="$ac_cv_gnu_ld"
  AC_SUBST([HAVE_GNU_LD])

  if test "x$ac_cv_gnu_ld" = "xyes"; then
    :
    $1
  else
    :
    $2
  fi])
