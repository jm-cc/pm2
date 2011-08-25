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


###### Marcel specific macros ######


# MARCEL_IS_PTHREAD(pthread)
# --------------------------
AC_DEFUN([MARCEL_IS_PTHREAD],
[
	AM_CONDITIONAL(IS_LIBPTHREAD, test x$__opt_pthread = xyes)
	AM_CONDITIONAL(IS_LIBPMARCEL, test x$__opt_pmarcel = xyes)

	if test x$__opt_pthread = xyes; then
	   PKG_lLIB=-lpthread
	   AC_MSG_NOTICE(marcel library name will be libpthread.so)
	else
	   if test x$__opt_pmarcel = xyes; then
	      PKG_lLIB=-lpmarcel
	      AC_MSG_NOTICE(marcel library name will be libpmarcel.so)
	   else
  	      PKG_lLIB=-lmarcel
	      AC_MSG_NOTICE(marcel library name will be libmarcel.so)
	   fi
	fi

	AC_SUBST(PKG_lLIB)
])


# MARCEL_HAVE_GDBINIT_FILES
# -------------------------
# Check if we have all gdbinit files for the current platform
AC_DEFUN([MARCEL_HAVE_GDBINIT_FILES],
[
	ADD_TO(gdbinit_files, $srcdir/scripts/gdb/gdbinit)

	__gdbinit_file=$srcdir/scripts/gdb/gdbinit-$TARGET_ARCH
	if test -f $__gdbinit_file; then
	   ADD_TO(gdbinit_files, $__gdbinit_file)
	fi

	__gdbinit_file=$srcdir/scripts/gdb/gdbinit-$TARGET_ARCH-$TARGET_SYS
	if test -f $__gdbinit_file; then
	   ADD_TO(gdbinit_files, $__gdbinit_file)
	fi

	AC_SUBST(gdbinit_files)	
])


# DEP_TBX
# -------
# Check if TBX was built with hwloc
AC_DEFUN([DEP_TBX],
[
	# check tbx lib
	PKG_CHECK_MODULES(TBX, tbx)
        ADD_TO(REQUIRED_PKG, tbx)

	# check if tbx include topology support
	PUSH_BUILDENV
	CFLAGS="$TBX_CFLAGS"
	LIBS="$TBX_LIBS"
	AC_CHECK_LIB(tbx, tbx_topology_init,:, PRINT_ERROR(Please install hwloc and rebuild the TBX library))
	POP_BUILDENV

	# check if tbx was built with Fortran support
	echo $TBX_CFLAGS | grep FORTRAN_COMPILER >/dev/null 2>&1
	__tbx_have_fortran=$?
	if test "x$__opt_fortran_compiler" = xyes && test $__tbx_have_fortran -eq 0; then
	   ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_FORTRAN)
	   __opt_fortran=yes
	   AC_MSG_NOTICE(marcel will be built with the Fortran interface)
	else
	   __opt_fortran=no
	   PRINT_WARNING(marcel will not be built with the Fortran interface)
	fi
	AM_CONDITIONAL(HAVE_FORTRAN, test "$__opt_fortran" = yes)

	# check if TBX was built with --enable-debug if marcel use it
	if test "x$__opt_debug" = xyes; then
	   echo $TBX_CFLAGS | grep TBX_DEBUG >/dev/null 2>&1
	   if test $? -ne 0; then
	      PRINT_ERROR([TBX was not built with debug support: rebuild TBX with it or do not use --enable-debug])
	   fi
	fi
])



# DEP_LIBC
# --------
AC_DEFUN([DEP_LIBC], [
	AC_REQUIRE([AC_PROG_CC])
	AC_REQUIRE([AC_CANONICAL_HOST])

	AC_CHECK_HEADERS(gnu/lib-names.h, __opt_lib_names=1)

	if test "x$__opt_lib_names" = x; then
		if test $TARGET_SYS = DARWIN_SYS; then
			# darwin systems
			LIBC_FILE=/usr/lib/libc.dylib
		else
			# others systems: /lib/libc.so.{ver}
			AC_PATH_PROG([TOUCH], [touch])
			LIBC_FILE=`ldd "$TOUCH" | grep 'libc\.so' | cut -d " " -f 3`
		fi

		if test "x$LIBC_FILE" = "x" || ! test -f "$LIBC_FILE"; then
	   	   AC_MSG_ERROR([Could not determine the path to the C library; see `config.log' for details.])
		fi

		AC_MSG_NOTICE([system C library path is `${LIBC_FILE}'])
		ADD_TO(GLOBAL_AM_CPPFLAGS, -DLIBC_FILE=\"\\\"${LIBC_FILE}\\\"\")
	fi
])
	


# PTHREAD_OR_OPTION_ENABLE(ARGNAME, HELP, CONFIG) 
# -----------------------------------------------
# Convenience wrapper over AC_ARG_ENABLE with the option deactivated by default,
# option is enabled in pthread mode
AC_DEFUN([PTHREAD_OR_OPTION_ENABLE],
[
	AC_MSG_CHECKING($3)

	defaultval=no
	if test x$__opt_pthread = xyes; then
	   defaultval=yes
	fi
	AC_ARG_ENABLE($1, AC_HELP_STRING(--enable-$1, $2), __opt_$1=$enableval, 
			  			      	   __opt_$1=$defaultval)

	if test x$__opt_$1 = xyes ; then
	  AC_MSG_RESULT(yes)
	else
	  AC_MSG_RESULT(no)
	fi
])


# MARCEL_DETECT_NBCPUS
# --------------------
# Detect the number of CPUs available
AC_DEFUN([MARCEL_DETECT_NBCPUS],
[
	AC_PATH_PROG(HWLOC_INFO, hwloc-info)

	if test $__with_nbmaxcpus = autodetect; then
	   # Use hwloc to count available cores
	   if test "x$HWLOC_INFO" != x; then
	      __with_nbmaxcpus=`$HWLOC_INFO --only PU -s | wc -l | tr -d " "`
	   else
	      __with_nbmaxcpus=32
	   fi
        fi

	# We use uint32_t or uint64_t to represents vpset
	if test $__with_nbmaxcpus -le 32; then
	   __with_nbmaxcpus=32
	else
	   if test $__with_nbmaxcpus -le 64; then
	      __with_nbmaxcpus=64
	   fi
	fi

	OPTION_SET_POSITIVE_CONST(nbmaxcpus, MARCEL_NBMAXCPUS)
])

# CHECK__FORTRAN_COMPILER
# -----------------------
# Check if we have a Fortran compiler
AC_DEFUN([CHECK__FORTRAN_COMPILER],
[
	AC_REQUIRE([AC_PROG_FC])
	if test "x${FC}" != x; then
	   __opt_fortran_compiler=yes
	else
	   __opt_fortran_compiler=no
	fi
])


# CHECK_PROTO__LIBC_ALLOCATE_RTSIG
# ---------------------------------
# Check if libc provides the __libc_allocate_rtsig prototype
AC_DEFUN([CHECK_PROTO__LIBC_ALLOCATE_RTSIG],
[
  	# Old versions of glibc lack these declarations.
  	AC_CHECK_DECLS([__libc_allocate_rtsig],
	[], [],
    	[[
		#include <signal.h>
	]])
])


# CHECK_PROTO__PTHREAD_UNWIND
# ---------------------------
# Check if libc provides the __pthread_unwind prototype
AC_DEFUN([CHECK_PROTO__PTHREAD_UNWIND],
[
  	# Old versions of glibc lack these declarations.
  	AC_CHECK_DECLS([__pthread_unwind],
	[], [],
    	[[
		#include <pthread.h>
	]])
])


# CHECK_PROTO__PTHREAD_YIELD
# ---------------------------
# Check if libc provides the pthread_yield prototype
AC_DEFUN([CHECK_PROTO__PTHREAD_YIELD],
[
  	# Old versions of glibc lack these declarations.
  	AC_CHECK_DECLS([pthread_yield],
	[], [],
    	[[
		#include <pthread.h>
	]])
])


# CHECK_PROTO__PTHREAD_MUTEX_CONSISTENT
# -------------------------------------
# Check if libc provides the pthread_mutex_consistent prototype
AC_DEFUN([CHECK_PROTO__PTHREAD_MUTEX_CONSISTENT],
[
  	# Old versions of glibc lack these declarations.
  	AC_CHECK_DECLS([pthread_mutex_consistent],
	[], [],
    	[[
		#include <pthread.h>
	]])
])


# CHECK_PROTO__PTHREAD_KILL_OTHER_THREADS_NP
# ------------------------------------------
# Check if libc provides the pthread_kill_other_threads_np prototype
AC_DEFUN([CHECK_PROTO__PTHREAD_KILL_OTHER_THREADS_NP],
[
  	# Old versions of glibc lack these declarations.
  	AC_CHECK_DECLS([pthread_kill_other_threads_np],
	[], [],
    	[[
		#include <pthread.h>
	]])
])


# CHECK_PROTO__PTHREAD_SIGQUEUE
# -----------------------------
# Check if libc provides the pthread_sigqueue prototype
AC_DEFUN([CHECK_PROTO__PTHREAD_SIGQUEUE],
[
  	# Old versions of glibc lack these declarations.
  	AC_CHECK_DECLS([pthread_sigqueue],
	[], [],
    	[[
		#include <pthread.h>
		#include <signal.h>
	]])
])


# CHECK__PTHREAD_CONDATTR_GETCLOCK
# --------------------------------
# Check if pthread_condattr_get/setclock is available
AC_DEFUN([CHECK__PTHREAD_CONDATTR_GETCLOCK],
[
  	# Old versions of glibc lack these declarations.
  	AC_CHECK_DECLS([pthread_condattr_getclock, pthread_condattr_setclock],
	[], [],
    	[[
		#include <pthread.h>
		#include <stdlib.h>
	]])
])


# CHECK__POSIX_SPAWN
# ------------------
# Check if posix_spawn functions are available
AC_DEFUN([CHECK__POSIX_SPAWN],
[
  	# Old versions of glibc lack these declarations.
	AC_CHECK_HEADERS([spawn.h])
  	AC_CHECK_DECLS([posix_spawn],
	[], [],
    	[[
		#include <spawn.h>
		#include <stdlib.h>
	]])
])


# CHECK__POSIX_MEMALIGN
# ---------------------
# Check if posix_memalign is available
AC_DEFUN([CHECK__POSIX_MEMALIGN],
[
	# Function is not available on Darwin Leopard
  	AC_CHECK_DECLS([posix_memalign],
	[], [],
    	[[
		#include <stdlib.h>
	]])
])


# CHECK__MEMALIGN
# Check if memalign is available
AC_DEFUN([CHECK__MEMALIGN],
[
	# Function is not available on Darwin Leopard
  	AC_CHECK_DECLS([memalign],
	[], [],
    	[[
		#include <stdlib.h>
	]])
])


# CHECK__SIGANDSET
# Check if sigandset function/macro is available
AC_DEFUN([CHECK__SIGANDSET],
[
	# Not available on Solaris
  	AC_CHECK_DECLS([sigandset],
	[], [],
    	[[
		#include <signal.h>
	]])
])


# CHECK__SIGEVENT_THREAD_NOTIFY
# -----------------------------
# Check if the system libc supports notification delivery to a particular thread
AC_DEFUN([CHECK__SIGEVENT_THREAD_NOTIFY],
[
	AC_MSG_CHECKING([whether libc supports notification delivery to a particular thread])
	AC_COMPILE_IFELSE(
		AC_LANG_PROGRAM(
		[[
			#include <signal.h>
		]], 
		[[
			AC_LANG_SOURCE([
				sigevent_t ev;
				#ifndef sigev_notify_thread_id
				#define sigev_notify_thread_id _sigev_un._tid
				#endif
				ev.sigev_notify_thread_id = 0;
			])
        	]]),
		AC_DEFINE([HAVE__SIGEVENT_THREAD_NOTIFY], [1], [Define to 1 if the libc supports notification delivery to a particular thread])
		AC_MSG_RESULT([yes]),
        	AC_MSG_RESULT([no])
    	)
])


# CHECK__GNU_NPTL_CLEANUP
# -----------------------
# Check for features provided by Glibc's Native POSIX Thread Library (NPTL)
AC_DEFUN([CHECK__GNU_NPTL_CLEANUP],
[
  	# Old versions of glibc lack these declarations.
	AC_CHECK_HEADERS([bits/libc-lock.h])
  	AC_CHECK_DECLS([_pthread_cleanup_pop, _pthread_cleanup_push],
    	[], [],
    	[[
		#include <pthread.h>
      		#include <bits/libc-lock.h>
	]])
])


# CHECK__SIGEV_THREAD_ID
# ----------------------
# Check if SIGEV_SIGNAL can be targeted to one thread in particular
AC_DEFUN([CHECK__SIGEV_THREAD_ID], 
[
	AC_CHECK_DECLS([SIGEV_THREAD_ID], 
	[
		AC_CHECK_LIB([rt], [timer_create],
    		             AC_DEFINE([HAVE_DECL_SIGEV_THREAD_ID], [1], [Define to 1 if you have the declaration of `SIGEV_THREAD_ID'.]))
	], [:], [[#include <signal.h>]])
])


# CHECK__SIG_DELIVERY_NODEFER
# ---------------------------
# Check if one of SA_NOMASK or SA_NODEFER is supported by the current platform
AC_DEFUN([CHECK__SIG_DELIVERY_NODEFER],
[
  	# Old versions of glibc lack these declarations.
	AC_CHECK_HEADERS([signal.h])
  	AC_CHECK_DECLS([SA_NOMASK, SA_NODEFER],
    	[], [],
    	[[
		#include <signal.h>
	]])
])


# CHECK__SIG_DELIVERY_RESETHAND
# -----------------------------
# Check if one of SA_RESETHAND or SA_ONESHOT is supported by the current platform
AC_DEFUN([CHECK__SIG_DELIVERY_RESETHAND],
[
  	# Old versions of glibc lack these declarations.
	AC_CHECK_HEADERS([signal.h])
  	AC_CHECK_DECLS([SA_RESETHAND, SA_ONESHOT],
    	[], [],
    	[[
		#include <signal.h>
	]])
])


# CHECK__THREAD_LOCAL_STORAGE_SUPPORT
# -----------------------------------
# Check if the system support TLS
AC_DEFUN([CHECK__THREAD_LOCAL_STORAGE_SUPPORT],
[
	AC_MSG_CHECKING([whether the platform supports TLS])
	AC_LINK_IFELSE([
			AC_LANG_SOURCE([
				__thread int a; 
				int b; 
				int main() 
				{ 
					return a = b; 
				}
			])
		],
		AC_DEFINE([HAVE__TLS_SUPPORT], [1], [Define to 1 if the system supports TLS])
		AC_MSG_RESULT([yes]),
        	AC_MSG_RESULT([no])
    	)
])


# CHECK__SIMPLE_SIG_FUNCTIONS
# ---------------------------
# Check if sighold, sigrelse, sigset and sigignore exists (ex.: not on FreeBSD 7)
AC_DEFUN([CHECK__SIMPLE_SIG_FUNCTIONS],
[
  	# Old versions of glibc lack these declarations.
	AC_CHECK_HEADERS([signal.h])
  	AC_CHECK_DECLS([sighold, sigrelse, sigset, sigignore],
    	[], [],
    	[[
		#include <signal.h>
	]])
])


# CHECK__DLOPEN_IN_LIBC
# ----------------------
# Check if dlopen is defined in libc, libdl, libdld
AC_DEFUN([CHECK__DLOPEN_IN_LIBC],
[
	LIBS=
	AC_SEARCH_LIBS(dlopen, [dl dld])
	ADD_TO(GLOBAL_AM_LDFLAGS, $LIBS)
])


# CHECK__FAST_INTEGER_SUPPORT
# ---------------------------
# Check if uint_fast32_t is defined on this platform
AC_DEFUN([CHECK__FAST_INTEGER_SUPPORT],
[
	AC_CHECK_SIZEOF(uint_fast32_t)
])
