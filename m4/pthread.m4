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

dnl ACX_GNU_NPTL
dnl
dnl Check for features provided by Glibc's Native POSIX Thread Library
dnl (NPTL).
AC_DEFUN([ACX_GNU_NPTL], [

  AC_CHECK_HEADERS([bits/libc-lock.h])

  dnl Old versions of glibc lack these declarations.
  AC_CHECK_DECLS([_pthread_cleanup_pop, _pthread_cleanup_push],
    [], [],
    [[#include <pthread.h>
      #include <bits/libc-lock.h>]])
])
