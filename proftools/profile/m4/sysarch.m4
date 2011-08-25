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

dnl Define and substitute TARGET_SYS, TARGET_ARCH, and TARGET_ASM.
dnl Written by Vincent Danjean.
AC_DEFUN([TARGET_SYS_ARCH], [dnl
  AC_REQUIRE([AC_CANONICAL_TARGET])
  case "$target_os" in
    solaris*)        TARGET_SYS=SOLARIS_SYS; TARGET_OS=solaris; __opt_solaris=yes;;
    linux-*)         TARGET_SYS=LINUX_SYS; TARGET_OS=linux; __opt_linux=yes;;
    gnu*)            TARGET_SYS=GNU_SYS; TARGET_OS=gnu; __opt_gnu=yes;;
    darwin*)	     TARGET_SYS=DARWIN_SYS; TARGET_OS=darwin; __opt_darwin=yes;;
    freebsd*)	     TARGET_SYS=FREEBSD_SYS; TARGET_OS=freebsd; __opt_freebsd=yes;;
    *)               AC_MSG_ERROR([Unknown OS '$target_os'. Please, update m4/sysarch.m4]);;
  esac

  case "$target_cpu" in
    i*86*)            TARGET_ARCH=X86_ARCH; TARGET_ASM=i386;;
    ia64*)            TARGET_ARCH=IA64_ARCH; TARGET_ASM=ia64;;
    powerpc64*)       TARGET_ARCH=PPC64_ARCH; TARGET_ASM=ppc64;;
    sparc64*)         TARGET_ARCH=SPARC64_ARCH; TARGET_ASM=sparc64;;
    x86_64*)          TARGET_ARCH=X86_64_ARCH; TARGET_ASM=x86_64;;
    *)                AC_MSG_ERROR([Unknown target '$target'. Please, update m4/sysarch.m4])
		      TARGET_ARCH=UNKNOWN_ARCH; TARGET_ASM=unknown;;
  esac

  ADD_TO(GLOBAL_AM_CPPFLAGS, -D$TARGET_ARCH -D$TARGET_SYS)


  AM_CONDITIONAL(ON_I386, test $TARGET_ASM = i386)
  AM_CONDITIONAL(ON_IA64, test $TARGET_ASM = ia64)
  AM_CONDITIONAL(ON_PPC64, test $TARGET_ASM = ppc64)
  AM_CONDITIONAL(ON_SPARC64, test $TARGET_ASM = sparc64)
  AM_CONDITIONAL(ON_X8664, test $TARGET_ASM = x86_64)


  ### Solaris
  if test $TARGET_SYS = SOLARIS_SYS; then
     ADD_TO(INTERNAL_AM_CPPFLAGS, -D_XOPEN_SOURCE=500 -D_STDC_C99 -D_XPG6 -D__EXTENSIONS__)
     ADD_TO(GLOBAL_AM_CFLAGS, -std=gnu99)
     if test $TARGET_ARCH = X86_ARCH; then
     	ADD_TO(GLOBAL_AM_LDADD, -lsocket -lrt)
     else
	ADD_TO(GLOBAL_AM_LDADD, -lsocket -lposix4)
     fi
  else
     ADD_TO(INTERNAL_AM_CPPFLAGS, -D_XOPEN_SOURCE=600)
  fi
  AM_CONDITIONAL(ON_SOLARIS, test $TARGET_OS = solaris)

  ### Darwin
  if test $TARGET_SYS = DARWIN_SYS; then
     ADD_TO(INTERNAL_AM_CPPFLAGS, -D_DARWIN_C_SOURCE=1)
  fi
  AM_CONDITIONAL(ON_DARWIN, test $TARGET_OS = darwin)

  ### FreeBSD
  if test $TARGET_SYS = FREEBSD_SYS; then
     ADD_TO(INTERNAL_AM_CPPFLAGS, -D_BSD_SOURCE=1 -D__BSD_VISIBLE=1)
  fi
  AM_CONDITIONAL(ON_FREEBSD, test $TARGET_OS = freebsd)

  ### Linux
  if test $TARGET_SYS = LINUX_SYS; then
     VERSION=`uname -r | cut -d . -f 1`
     PATCHLEVEL=`uname -r | cut -d . -f 2`
     
     ADD_TO(GLOBAL_AM_LDADD, -lrt)
     ADD_TO(INTERNAL_AM_CPPFLAGS, -DLINUX_VERSION=\"\\\"$VERSION.$PATCHLEVEL\\\"\" -D_GNU_SOURCE=1)
     if test "$VERSION" -lt 2 -o "$PATCHLEVEL" -lt 6; then
     	ADD_TO(INTERNAL_AM_CPPFLAGS, -DOLD_ITIMER_REAL)
     fi
  fi
  AM_CONDITIONAL(ON_LINUX, test $TARGET_OS = linux)


  AC_SUBST(TARGET_ARCH)
  AC_SUBST(TARGET_ASM)
  AC_SUBST(TARGET_SYS)
  AC_SUBST(TARGET_OS)
])