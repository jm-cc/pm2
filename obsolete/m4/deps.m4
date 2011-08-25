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
  PKG_CHECK_MODULES([FXT], [fxt], [
    HAVE_FXT="yes"
    _PKG_CONFIG([FXT_PREFIX], [variable=prefix], fxt)
    FXT_PREFIX="$pkg_cv_FXT_PREFIX"
  ], [HAVE_FXT="no"])

  PKG_CHECK_MODULES([MING], [libming], [HAVE_MING="yes"], [HAVE_MING="no"])

  AC_SUBST([HAVE_FXT])
  AC_SUBST([FXT_PREFIX])

  AC_SUBST([HAVE_MING])

  CPPFLAGS="$save_CPPFLAGS"
  LDFLAGS="$save_LDFLAGS"

  if test "x$HAVE_FXT$HAVE_MING" != "xyesyes"; then
     AC_MSG_WARN([missing dependencies for the `profile' module (see above)])
  fi])
