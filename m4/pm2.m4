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

dnl ACX_PM2_SYSARCH
dnl
dnl Define and substitute PM2_SYS, PM2_ARCH, and PM2_ASM.
dnl Written by Vincent Danjean.
AC_DEFUN([PM2_SYS_ARCH], [dnl
  AC_REQUIRE([AC_CANONICAL_TARGET])
  case "$target_os" in
    solaris*)        PM2_SYS=SOLARIS_SYS ;;
    aix*)            PM2_SYS=AIX_SYS ;;
    irix*)           PM2_SYS=IRIX_SYS ;;
    osf*)            PM2_SYS=OSF_SYS ;;
    linux-*)         PM2_SYS=LINUX_SYS ;;
    *freebsd*)       PM2_SYS=FREEBSD_SYS ;;
    cygwin*)         PM2_SYS=WIN_SYS ;;
    darwin*)         PM2_SYS=DARWIN_SYS ;;
    gnu*)            PM2_SYS=GNU_SYS ;;
    unicos*)         PM2_SYS=UNICOS_SYS ;;
    *)               AC_MSG_ERROR([Unknown OS '$target_os'. Please, update aclocal/pm2.m4])
		   ;;
  esac

  case "$target_cpu" in
    alpha*)           PM2_ARCH=ALPHA_ARCH ; PM2_ASM=alpha ;;
    arm*)             PM2_ARCH=ARM_ARCH ; PM2_ASM=arm ;;
    cris*)            PM2_ARCH=CRIS_ARCH ; PM2_ASM=cris ;;
    i*86*)            PM2_ARCH=X86_ARCH ; PM2_ASM=i386 ;;
    ia64*)            PM2_ARCH=IA64_ARCH ; PM2_ASM=ia64 ;;
    mips*)            PM2_ARCH=MIPS_ARCH ; PM2_ASM=mips ;;
    powerpc64*)       PM2_ARCH=PPC64_ARCH ; PM2_ASM=ppc64 ;;
    powerpc*)         PM2_ARCH=PPC_ARCH ; PM2_ASM=ppc ;;
    rs6000*)          PM2_ARCH=RS6K_ARCH ; PM2_ASM=rs8k ;;
    sparc64*)         PM2_ARCH=SPARC64_ARCH ; PM2_ASM=sparc64 ;;
    sparc*)           PM2_ARCH=SPARC_ARCH ; PM2_ASM=sparc ;;
    x86_64*)          PM2_ARCH=X86_64_ARCH ; PM2_ASM=x86_64 ;;
    *)                AC_MSG_ERROR([Unknown target '$target'. Please, update aclocal/pm2.m4])
		      M2_ARCH=UNKNOWN_ARCH ; PM2_ASM=unknown ;;
  esac
  AC_SUBST([PM2_SYS])
  AC_SUBST([PM2_ARCH])
  AC_SUBST([PM2_ASM])
  AC_DEFINE_UNQUOTED([$PM2_SYS],  1, [PM2 operating system type])
  AC_DEFINE_UNQUOTED([$PM2_ARCH], 1, [PM2 hardware architecture type])
])
