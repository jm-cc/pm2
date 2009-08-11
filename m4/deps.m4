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

dnl PM2_PROFILE_DEPENDENCIES
dnl
dnl Look for the dependencies of the `profile' module.
AC_DEFUN([PM2_PROFILE_DEPENDENCIES], [
  save_CPPFLAGS="$CPPFLAGS"
  save_LDFLAGS="$LDFLAGS"

  # Starting from 2009-06-24, FxT's CVS has a `pkg-config' file.
  PKG_CHECK_MODULES([FXT], [fxt], [HAVE_FXT="yes"], [HAVE_FXT="no"])

  AC_ARG_WITH([ming-prefix],
    [AS_HELP_STRING([--with-ming-prefix=DIRECTORY],
      [use DIRECTORY as the Ming library installation prefix])],
    [ming_prefix="$withval"
     MING_CPPFLAGS="-I$ming_prefix/include"
     MING_LDFLAGS="-L$ming_prefix/lib"
     CPPFLAGS="$MING_CPPFLAGS $CPPFLAGS"
     LDFLAGS="$MING_LDFLAGS $LDFLAGS"],
    [ming_prefix=""])

  AC_CHECK_LIB([ming], [Ming_init], [HAVE_MING="yes"], [HAVE_MING="no"])
  if test "x$HAVE_MING" = "xyes"; then
     AC_CHECK_HEADER([ming.h], [HAVE_MING="yes"], [HAVE_MING="no"])
  fi

  AC_SUBST([HAVE_FXT])

  AC_SUBST([HAVE_MING])
  AC_SUBST([MING_CPPFLAGS])
  AC_SUBST([MING_LDFLAGS])

  CPPFLAGS="$save_CPPFLAGS"
  LDFLAGS="$save_LDFLAGS"

  if test "x$HAVE_FXT$HAVE_MING" != "xyesyes"; then
     AC_MSG_WARN([missing dependencies for the `profile' module (see above)])
  fi])
