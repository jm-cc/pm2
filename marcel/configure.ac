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

AC_INIT(Marcel,
	2.99.3,
	http://runtime.bordeaux.inria.fr/marcel/,
	marcel)
AC_PREREQ(2.59)


AC_PREFIX_DEFAULT(/opt/marcel)
AC_CONFIG_SRCDIR(./configure.ac)
AC_CONFIG_AUX_DIR(./build-aux)
AC_CONFIG_MACRO_DIR([m4])


# Preserve order options in help
m4_ifdef([AC_PRESERVE_HELP_ORDER], [AC_PRESERVE_HELP_ORDER])


# Make the build output less verbose
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])


# Get ARCH & SYS specifications
TARGET_SYS_ARCH


# Save user CFLAGS/LDFLAGS/CPPFLAGS
PUSH_BUILDENV


# Activate Automake support
AM_INIT_AUTOMAKE([subdir-objects foreign])


# Check for usual build tools
AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX
AC_PROG_FC
AC_PROG_LN_S
AM_PROG_AS
AC_PROG_LIBTOOL
AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG
CHECK__FORTRAN_COMPILER
CHECK__C_COMPILER
LT_INIT

RM_F="rm -f"
AC_SUBST(RM_F)
LN_SF="ln -sf"
AC_SUBST(LN_SF)

### automake-1.9 does not substitute these two by default
AC_SUBST([abs_top_srcdir])
AC_SUBST([abs_top_builddir])


# Check prototype ...
ADD_TO(CFLAGS, -Wall -D_POSIX_C_SOURCE=200112 -std=c99 ${GLOBAL_AM_CPPFLAGS})
CHECK_PROTO__LIBC_ALLOCATE_RTSIG
CHECK_PROTO__PTHREAD_UNWIND
CHECK_PROTO__PTHREAD_MUTEX_CONSISTENT
CHECK_PROTO__PTHREAD_KILL_OTHER_THREADS_NP
CHECK_PROTO__PTHREAD_SIGQUEUE
CHECK_PROTO__PTHREAD_YIELD
CHECK__SIMPLE_SIG_FUNCTIONS
CHECK__PTHREAD_CONDATTR_GETCLOCK
CHECK__POSIX_SPAWN
CHECK__POSIX_MEMALIGN
CHECK__MEMALIGN
CHECK__SIGEVENT_THREAD_NOTIFY
CHECK__GNU_NPTL_CLEANUP
CHECK__SIGANDSET
CHECK__SIGEV_THREAD_ID
CHECK__SIG_DELIVERY_NODEFER
CHECK__SIG_DELIVERY_RESETHAND
CHECK__THREAD_LOCAL_STORAGE_SUPPORT
CHECK__DLOPEN_IN_LIBC
CHECK__FAST_INTEGER_SUPPORT


# Restore environment
POP_BUILDENV


# Global build tool options
ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL -U_FORTIFY_SOURCE)
ADD_TO(GLOBAL_AM_CPPFLAGS, -D_POSIX_C_SOURCE=200112 -D__STDC_LIMIT_MACROS=1)
ADD_TO(GLOBAL_AM_CFLAGS, $CFLAGS)
ADD_TO(GLOBAL_AM_LDFLAGS, $LDFLAGS)


# Option declarations
#
### Debug
OPTION_ENABLE(maintainer_mode, enables the maintainer_mode, whether the maintainer mode is enabled)
OPTION_ENABLE(profile, enables the profiling in this library, whether the library will be profiled)
OPTION_ENABLE(gcc_instrument, auto-profiles all functions of this module, whether library functions will be profiled)
OPTION_ENABLE(debug, builds the library in debug mode, whether the library will be built in debug mode)
OPTION_ENABLE(gprof, instruments the code for gprof, whether the code will be instrumented for gprof)
OPTION_ENABLE(coverage, instruments the code for coverage test, whether the code will be instrumented for coverage test) 
OPTION_DISABLE(inline, does not inline function code, whether inline optimisation will be disabled)
OPTION_ENABLE(stacks_guard, insert guards (memory holes) between stacks, whether the code will be compiled with stacks guard)
OPTION_ENABLE(trace, enables basic scheduler event tracing (thread creation, block, ...), whether the code will be compiled with traces)
OPTION_ENABLE(bug_on, enables BUG_ON macros family, whether BUG_ON is activated)
OPTION_ENABLE(valgrind, support for valgrind debugging, whether the code will be compiled with valgrind support)
OPTION_ENABLE(debug_spinlock, enables spinlock debugging, whether the code will be compiled with spinlock debug code)
OPTION_ENABLE(verbose_tests, enables test traces, whether the test code will be compiled with traces)
OPTION_DISABLE(allocator_check, disable external allocator check, whether the external allocator check is enabled)


### Optimisation
OPTION_ENABLE(optimize, enables compiler optimisations, whether the library will be optimised)
OPTION_ENABLE(strip, enables linker strip operation, whether the library will be striped)

### BubbleSched
OPTION_ENABLE(bubbles, enable bubbles, whether the code will be compiled with BubbleSched support)

### Library type
OPTION_WITH(libtype, marcel | pmarcel | pthread, 
	    marcel library interface: marcel - pmarcel (POSIX API) - pthread (NPTL ABI), marcel library interface, pmarcel)

### Targeted topology
OPTION_WITH(topo, mono | smp | numa, defines the topology supported by Marcel, Marcel targeted topology, numa)

### Main function
OPTION_WITH(mainfunc, marcel_main | standard_main | main_as_func, 
	    marcel_main: Marcel provides 'main' - standard_main: app provides 'main' - main_as_func: app provides 'main' and starts Marcel,
	    application main type, standard_main)

### Marcel features
OPTION_ENABLE(atexit, enables the marcel_atexit function, whether atexit function is enabled)
OPTION_ENABLE(blocking, enables marcel_enter/leave_blocking_section functions, whether *_blocking_section is enabled)
PTHREAD_OR_OPTION_ENABLE(check_prio_on_locks, locks take care about thread priority, whether thread priority check on locks is enabled) 
PTHREAD_OR_OPTION_ENABLE(cleanup, enables the marcel_cleanup_push/pop functions, whether marcel_cleanup_push/pop functions are enabled)
PTHREAD_OR_OPTION_ENABLE(deviation, enables the marcel_deviate and marcel_cancel functions, whether marcel_deviate/_cleanup are enabled)
OPTION_ENABLE(exceptions, enables the exception handling mechanism of Marcel, whether the exception handling mechanism is enabled)
PTHREAD_OR_OPTION_ENABLE(keys, enables the keys/thread-specific data handling mechanism of Marcel, whether the keys/thread-specific data handling mechanism is enabled)
OPTION_ENABLE(migration, permits Marcel threads to be migrated between machines, whether thread migration is enabled)
PTHREAD_OR_OPTION_ENABLE(once, enables the marcel_once function, whether marcel_once is enabled)
OPTION_ENABLE(postexit, enables the marcel_postexit function, whether marcel_postexit is enabled)
PTHREAD_OR_OPTION_ENABLE(signals, enables the POSIX-compatible signal functions, whether POSIX-compatible signal functions are enabled)
PTHREAD_OR_OPTION_ENABLE(fork, enables process forking mechanism, whether Marcel will handle process fork)
OPTION_ENABLE(stats, enables the stats gathering mechanism of Marcel, whether the stats gathering mechanism is enabled)
OPTION_ENABLE(suspend, enables the marcel_suspend/resume functions, whether marcel_suspend/resume functions are enabled)
OPTION_ENABLE(userspace, enables the thread userspace extra data storage mechanism of Marcel, 
			 whether the thread userspace extra data storage mechanism is enabled)
OPTION_DISABLE(group_tcp, does not factorize TCP calls (FOR TESTS ONLY), whether TCP calls are not factorized)
OPTION_DISABLE(timer, disables periodic preemption (timer), whether periodic preemption is disabled)
OPTION_ENABLE(idle_pause, enables pause() calls when idle -  may decrease I/O reactivity, whether pause() is used when idle)
OPTION_ENABLE(smp_remote_tasklet, enables remote tasklet execution, whether remote tasklet execution is enabled)
OPTION_ENABLE(smt, enables SMT core technology - may decrease perf., whether SMT core technology is enabled)
OPTION_ENABLE(stack_jump, allows a thread to jump to another stack, whether thread stack jumping is allowed)
OPTION_ENABLE(use_tls, use system's TLS to store marcel_self instead of computing from the SP value, whether Marcel use system's TLS)
OPTION_ENABLE(mami, enable Marcel memory interface, whether Marcel memory interface is enabled)

### Marcel parameters
OPTION_WITH(nbmaxcpus, max cpu number, set the maximum number of cpus, maximum number of cpus, autodetect)
OPTION_WITH(nbmaxnodes, max node number, set the maximum number of NUMA nodes, maximum number of NUMA nodes, 8)
OPTION_WITH(tls_area_size, size in Bytes, set the default TLS (thread local storage) area size, default TLS area size in Bytes, 8192)
OPTION_WITH(max_keys, number of thread keys, set the default number of keys for each thread, default number of keys, 1024)


# Check dependencies
#
### Debug
OPTION_CHECK_DEPEND(profile, maintainer_mode,)
OPTION_CHECK_DEPEND(gcc_instrument, profile, optimize)
OPTION_CHECK_DEPEND(debug,, optimize)
OPTION_CHECK_DEPEND(inline, maintainer_mode, optimize)
OPTION_CHECK_DEPEND(trace, maintainer_mode, optimize)
OPTION_CHECK_DEPEND(valgrind, maintainer_mode, optimize)
OPTION_CHECK_DEPEND(debug_spinlock, maintainer_mode, pthread)
OPTION_CHECK_DEPEND(bug_on, maintainer_mode,)
OPTION_CHECK_DEPEND(coverage, debug,)
OPTION_CHECK_DEPEND(mami, bubbles)

### Optimisation
OPTION_CHECK_DEPEND(strip,, profile gcc_instrument debug inline trace valgrind gprof coverage)

### BubbleSched
OPTION_CHECK_DEPEND(bubbles, numa stats,)

### Library type
OPTION_CHECK_ONEOFTHEM(libtype, marcel pmarcel pthread)
OPTION_CHECK_DEPEND(pthread, linux standard_main, freebsd darwin solaris)

### Targeted topology
OPTION_CHECK_ONEOFTHEM(topo, mono smp numa)
OPTION_CHECK_DEPEND(mono,, blocking)

### Main function
OPTION_CHECK_ONEOFTHEM(mainfunc, marcel_main standard_main main_as_func)

### Marcel features
OPTION_CHECK_DEPEND(once, cleanup,)
OPTION_CHECK_DEPEND(postexit, atexit,)
OPTION_CHECK_DEPEND(signals, deviation, solaris)
OPTION_CHECK_DEPEND(suspend, deviation,)
OPTION_CHECK_DEPEND(smp_remote_tasklet, smp,)
OPTION_CHECK_DEPEND(use_tls,, pthread)


# Option activations
#
### Debug
OPTION_APPLY(profile, ADD_TO(PKG_AM_CPPFLAGS, -DDO_PROFILE))
OPTION_APPLY(gcc_instrument, [ ADD_TO(PKG_AM_CPPFLAGS, -DPM2_INSTRUMENT) ADD_TO(GLOBAL_AM_CFLAGS, -finstrument-functions) ])
OPTION_APPLY(debug, [ ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_DEBUG -DMARCEL_GDB) 
		      ADD_TO(GLOBAL_AM_CFLAGS, -gdwarf-2 -g3 -fno-omit-frame-pointer) ])
OPTION_APPLY(gprof, [ ADD_TO(GLOBAL_AM_CFLAGS, -fprofile-arcs -pg -fno-omit-frame-pointer) 
		      ADD_TO(GLOBAL_AM_LDFLAGS, [-pg -fprofile-arcs]) ])
OPTION_APPLY(coverage, [ ADD_TO(GLOBAL_AM_CFLAGS, -O0 --coverage) ADD_TO(GLOBAL_AM_LDFLAGS, --coverage) ])
OPTION_APPLY(inline, [ ADD_TO(PKG_AM_CPPFLAGS, -DPM2_NOINLINE -DTBX_DISABLE_VISIBILITY)
		       ADD_TO(GLOBAL_AM_CFLAGS, -fno-inline) ])
OPTION_APPLY(stacks_guard, ADD_TO(PKG_AM_CPPFLAGS, -DPM2STACKSGUARD))
OPTION_APPLY(trace, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_TRACE))
OPTION_APPLY(bug_on, ADD_TO(PKG_AM_CPPFLAGS, -DPM2_BUG_ON))
OPTION_APPLY(valgrind, ADD_TO(PKG_AM_CPPFLAGS, -DPM2VALGRIND))
OPTION_APPLY(debug_spinlock, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_DEBUG_SPINLOCK))
OPTION_APPLY(verbose_tests, ADD_TO(PKG_AM_CPPFLAGS, -DVERBOSE_TESTS))
OPTION_APPLY(maintainer_mode, ADD_TO(PKG_AM_CPPFLAGS, -DTBX_DISABLE_VISIBILITY -DWHITE_BOX -D__MARCEL_KERNEL__))
OPTION_APPLY(allocator_check, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_ALLOCATOR_NOCHECK))

### Optimisation
OPTION_APPLY(optimize, [ ADD_TO(PKG_AM_CPPFLAGS, -DPM2_OPT -DNDEBUG) 
		       	 ADD_TO(GLOBAL_AM_CPPFLAGS, -DNDEBUG)
		       	 ADD_TO(GLOBAL_AM_CFLAGS, -ffast-math -O3)
			 ADD_TO(GLOBAL_AM_LDFLAGS, [-Wl,-O2]) ])
OPTION_APPLY(strip, [ ADD_TO(GLOBAL_AM_LDFLAGS, [-Wl,-s]) ])

### BubbleSched
OPTION_APPLY(bubbles, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_BUBBLES))

### Library type
OPTION_APPLY(marcel, [ ADD_TO(GLOBAL_AM_CPPFLAGS, -D_REENTRANT) ADD_TO(GLOBAL_AM_LDADD, -lpthread) ])
OPTION_APPLY(pmarcel, [ ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_POSIX)
		      	ADD_TO(GLOBAL_AM_CPPFLAGS, -D_REENTRANT) 
			ADD_TO(GLOBAL_AM_LDADD, -lpthread) ])
OPTION_APPLY(pthread, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_LIBPTHREAD -DMARCEL_DONT_USE_POSIX_THREADS -DMARCEL_POSIX))

### Targeted topology
OPTION_APPLY(mono, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_MONO))
OPTION_APPLY(smp, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_SMP))
OPTION_APPLY(numa, [ ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_NUMA) 
		     ADD_TO(GLOBAL_AM_LDADD, -lm) 
		     test "$BUILD_SYS" = SOLARIS_SYS && ADD_TO(GLOBAL_AM_LDADD, -llgrp -lkstat) ])

### Main function
OPTION_APPLY(standard_main, ADD_TO(PKG_AM_CPPFLAGS, -DSTANDARD_MAIN))
OPTION_APPLY(main_as_func, ADD_TO(PKG_AM_CPPFLAGS, -DMAIN_AS_FUNC))

### Marcel features
OPTION_APPLY(atexit, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_ATEXIT_ENABLED))
OPTION_APPLY(blocking, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_BLOCKING_ENABLED))
OPTION_APPLY(check_prio_on_locks, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_CHECK_PRIO_ON_LOCKS))
OPTION_APPLY(cleanup, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_CLEANUP_ENABLED))
OPTION_APPLY(deviation, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_DEVIATION_ENABLED))
OPTION_APPLY(exceptions, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_EXCEPTIONS_ENABLED))
OPTION_APPLY(keys, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_KEYS_ENABLED))
OPTION_APPLY(migration, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_MIGRATION_ENABLED))
OPTION_APPLY(once, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_ONCE_ENABLED))
OPTION_APPLY(postexit, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_POSTEXIT_ENABLED))
OPTION_APPLY(fork, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_FORK_ENABLED))
OPTION_APPLY(signals, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_SIGNALS_ENABLED))
OPTION_APPLY(stats, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_STATS_ENABLED))
OPTION_APPLY(suspend, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_SUSPEND_ENABLED))
OPTION_APPLY(userspace, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_USERSPACE_ENABLED))
OPTION_APPLY(group_tcp, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_DO_NOT_GROUP_TCP))
OPTION_APPLY(timer, ADD_TO(PKG_AM_CPPFLAGS, -DMA_DO_NOT_LAUNCH_SIGNAL_TIMER))
OPTION_APPLY(idle_pause, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_IDLE_PAUSE))
OPTION_APPLY(smp_remote_tasklet, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_REMOTE_TASKLETS))
OPTION_APPLY(smt, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_SMT_ENABLED))
OPTION_APPLY(stack_jump, ADD_TO(PKG_AM_CPPFLAGS, -DENABLE_STACK_JUMPING))
OPTION_APPLY(use_tls, [ ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_USE_TLS) ADD_TO(GLOBAL_AM_CFLAGS, -ftls-model=initial-exec) ])
OPTION_APPLY(mami, ADD_TO(PKG_AM_CPPFLAGS, -DMARCEL_MAMI_ENABLED))

### Marcel parameters
OPTION_SET_POSITIVE_CONST(nbmaxnodes, MARCEL_NBMAXNODES)
OPTION_SET_POSITIVE_CONST(tls_area_size, MARCEL_TLS_AREA_SIZE)
OPTION_SET_POSITIVE_CONST(max_keys, MARCEL_MAX_KEYS)
MARCEL_DETECT_NBCPUS


# Dependencies
PKG_CHECK_MODULES(HWLOC, [hwloc >= 1.0])
DEP_TBX
DEP_LIBC


# Global build tool options
AC_SUBST(PKG_AM_CPPFLAGS)
AC_SUBST(GLOBAL_AM_CPPFLAGS)
AC_SUBST(GLOBAL_AM_CFLAGS)
AC_SUBST(GLOBAL_AM_LDFLAGS)
AC_SUBST(GLOBAL_AM_LDADD)


MARCEL_IS_PTHREAD
MARCEL_HAVE_GDBINIT_FILES

# marcel.pc => package dependcy list
AC_SUBST(REQUIRED_PKG)

# Automake outputs
AC_CONFIG_COMMANDS([marcel_abi], [chmod +x scripts/gen-header/cpp-hash.sh])
AC_CONFIG_COMMANDS([bubble_sched_list], [chmod +x scripts/gen-header/scheduler-list.sh])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES(scripts/gen-header/cpp-hash.sh
		scripts/gen-header/scheduler-list.sh
		scripts/marcel-exec
		include/sys/Makefile
		include/scheduler/Makefile
		include/Makefile
		source/Makefile
		tests/Makefile
                benchmarks/Makefile
                benchmarks/helpers/marcel_bench_launcher
		Makefile
		pkgconfig/marcel.pc)


AC_OUTPUT
