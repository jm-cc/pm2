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


AM_CPPFLAGS  = -imacros $(abs_top_builddir)/config.h -I $(abs_top_srcdir)/include -I $(abs_top_builddir)/include -D__MARCEL_KERNEL__ $(GLOBAL_AM_CPPFLAGS) $(PKG_AM_CPPFLAGS) $(TBX_CFLAGS)
AM_CFLAGS    = $(GLOBAL_AM_CFLAGS)
AM_LDFLAGS   = $(GLOBAL_AM_LDFLAGS) $(GLOBAL_AM_LDADD) $(TBX_LIBS)
AM_CCASFLAGS = $(GLOBAL_AM_CFLAGS) -D__ASSEMBLY__



## Common libmarcel sources
ARCH_GENERIC_CFILES = 	arch-generic/marcel_compareexchange.c arch-generic/marcel_semaphore.c

ARCH_I386_CFILES = 	arch-i386/linux_semaphore.c arch-i386/asm.S

ARCH_IA64_CFILES = 	arch-ia64/marcel_catch_error.c arch-ia64/linux_spinlock_contention.S\
			arch-ia64/marcel_longjmp.S arch-ia64/marcel_setjmp.S

ARCH_X8664_CFILES =     arch-x86_64/asm.S

BUBBLESCHED_CFILES = 	scheduler/marcel_sched_create.c scheduler/marcel_sched.c\
			scheduler/marcel_bubble_sched.c scheduler/linux_runqueues.c\
			scheduler/marcel_bubble_browse.c scheduler/marcel_bubble_helper.c\
			scheduler/profiles/marcel_bubble_null.c\
			scheduler/profiles/marcel_bubble_steal.c\
			scheduler/profiles/marcel_bubble_cache.c\
			scheduler/profiles/marcel_bubble_gang.c\
			scheduler/profiles/marcel_bubble_explode.c\
			scheduler/profiles/marcel_bubble_memory.c\
			scheduler/profiles/marcel_bubble_spread.c\
			scheduler/marcel_bubble_distribution.c scheduler/linux_sched.c \
			scheduler/marcel_holder.c

SYS_CFILES = 		sys/marcel_container.c sys/marcel_lwp.c sys/marcel_kthread.c\
			sys/marcel_membarrier.c sys/marcel_io_bridge.c\
			sys/pthread_todo.c sys/marcel_sched_generic.c\
			sys/linux_softirq.c sys/marcel_debug.c sys/linux_notifier.c\
			sys/marcel_allocator.c sys/linux_bitops.c sys/linux_timer.c\
			sys/marcel_gluec_stdio.c sys/marcel_gluec_realsym.c\
			sys/marcel_gluec_exec.c sys/marcel_gluec_stdmem.c sys/marcel_gluec_sysio.c

MARCEL_CFILES = 	marcel_seed.c marcel_stats.c marcel_topology.c\
			pthread_rwlock.c marcel_attr.c marcel_io.c marcel_futex.c\
			marcel_cond.c marcel_keys.c marcel_spin.c marcel_lpt_barrier.c\
			marcel_signal.c marcel_top.c marcel_errno.c marcel_once.c marcel_mutex.c\
			marcel_barrier.c marcel_fortran.c marcel_locking.c marcel_init.c\
			marcel_threads.c marcel_extern.c marcel_alloc.c marcel_timer.c\
			marcel_sem.c marcel_polling.c marcel_lpt_mutex.c marcel_lpt_cond.c\
			marcel_deviate.c linux_fork.c marcel_supervisor.c marcel_exception.c

LINUX_CFILES = 		sys/linux/marcel_kthread.c sys/linux/marcel_numa_alloc.c\
			sys/linux/marcel_random.c sys/linux/marcel_jmp.c \
			sys/linux/pthread_sigrt.c sys/linux/nptl_glue_init.c\
			sys/linux/nptl_checkabi.c sys/linux/nptl_glue_cleanup.c\
			sys/linux/nptl_flockfile.c

SOLARIS_CFILES =	sys/solaris/marcel_random.c sys/solaris/marcel_kthread.c

DARWIN_CFILES =		sys/darwin/marcel_random.c sys/darwin/marcel_kthread.c

FREEBSD_CFILES =	sys/freebsd/marcel_random.c sys/freebsd/marcel_kthread.c

marcel_sources = $(MARCEL_CFILES) $(SYS_CFILES) $(BUBBLESCHED_CFILES) $(ARCH_GENERIC_CFILES)


## Arch dependant sources
EXTRA_DIST =		arch-ia64/asmmacro.h arch-ia64/marcel_ucontext_i.h
if ON_I386
   marcel_sources += $(ARCH_I386_CFILES)
   ld_libpthread_map = -Wl,--version-script=$(abs_top_srcdir)/scripts/linker/arch-i386/libpthread.map
endif
if ON_IA64
   marcel_sources += $(ARCH_IA64_CFILES)
   ld_libpthread_map = -Wl,--version-script=$(abs_top_srcdir)/scripts/linker/arch-ia64/libpthread.map
endif
if ON_X8664
   marcel_sources += $(ARCH_X8664_CFILES)
   ld_libpthread_map = -Wl,--version-script=$(abs_top_srcdir)/scripts/linker/arch-x86_64/libpthread.map
endif


## System dependant sources
if ON_LINUX
   marcel_sources += $(LINUX_CFILES)
endif
if ON_DARWIN
   marcel_sources += $(DARWIN_CFILES)
endif
if ON_SOLARIS
   marcel_sources += $(SOLARIS_CFILES)
endif
if ON_FREEBSD
   marcel_sources += $(FREEBSD_CFILES)
endif


## Build a pthread or pmarcel lib
if IS_LIBPTHREAD
   pkglib_LTLIBRARIES = libpthread.la
   libpthread_la_SOURCES = $(marcel_sources)
   GLOBAL_AM_LDFLAGS += $(ld_libpthread_map)
else
if IS_LIBPMARCEL
   pkglib_LTLIBRARIES = libpmarcel.la
   libpmarcel_la_SOURCES = $(marcel_sources)
else
   pkglib_LTLIBRARIES = libmarcel.la
   libmarcel_la_SOURCES = $(marcel_sources)
endif
endif
