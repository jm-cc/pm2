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


dnl ACX_GNU_MAKE
dnl
dnl Raise an error if GNU Make cannot be found.
AC_DEFUN([ACX_GNU_MAKE], [
  for make in $MAKE make gmake
  do
    AC_MSG_CHECKING([whether $make is GNU Make])
    if ( $make --version | grep GNU >/dev/null) 2>/dev/null ; then
      AC_MSG_RESULT([yes])
      found_gnu_make="yes"
      break
    else
      AC_MSG_RESULT([no])
    fi
  done

  if test "x$found_gnu_make" != "xyes"; then
     AC_MSG_ERROR([Cannot find GNU Make.  Please install it or set the \$MAKE variable.])
  else
     MAKE="$make"
  fi])
