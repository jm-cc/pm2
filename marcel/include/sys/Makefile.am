#
# PM2: Parallel Multithreaded Machine
# Copyright (C) 2010 "the PM2 team" (see AUTHORS file)
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


SOLARIS_HFILES = 		solaris/marcel_flags.h solaris/marcel_isomalloc.h\
                                solaris/marcel_signal.h solaris/marcel_kthread.h\
                                solaris/marcel_misc.h

LINUX_HFILES =			linux/marcel_flags.h linux/marcel_isomalloc.h\
                                linux/marcel_signal.h linux/marcel_kthread.h\
                                linux/marcel_misc.h linux/marcel_gluec_realsym.h\
				linux/nptl_flockfile.h

DARWIN_HFILES =			darwin/marcel_flags.h darwin/marcel_isomalloc.h\
				darwin/marcel_kthread.h	darwin/marcel_misc.h\
				darwin/marcel_signal.h

FREEBSD_HFILES =		freebsd/marcel_flags.h freebsd/marcel_isomalloc.h\
				freebsd/marcel_kthread.h freebsd/marcel_misc.h\
				freebsd/marcel_signal.h

SYS_HFILES =			marcel_flags.h linux_thread_info.h marcel_lwp.h\
                                marcel_isomalloc.h marcel_fastlock.h linux_perlwp.h\
                                marcel_utils.h linux_spinlock.h marcel_valgrind.h\
                                linux_linkage.h marcel_debug.h marcel_compiler.h\
				marcel_kthread.h marcel_io_bridge.h inlinefunctions/linux_thread_info.h\
                                inlinefunctions/marcel_lwp.h inlinefunctions/marcel_fastlock.h\
                                inlinefunctions/linux_spinlock.h inlinefunctions/linux_jiffies.h\
                                inlinefunctions/linux_interrupt.h inlinefunctions/marcel_io_bridge.h\
                                inlinefunctions/marcel_stackjump.h inlinefunctions/linux_timer.h\
                                inlinefunctions/marcel_sched_generic.h inlinefunctions/marcel_container.h\
                                inlinefunctions/allfunctions.h inlinefunctions/linux_bitops.h\
                                linux_types.h marcel_switchto.h linux_notifier.h\
                                linux_softirq.h linux_preempt.h marcel_types.h\
                                marcel_alias.h linux_interrupt.h marcel_allocator.h\
                                marcel_misc.h marcel_stackjump.h linux_timer.h\
                                marcel_membarrier.h marcel_container.h marcel_sched_generic.h\
				marcel_gluec_stdio.h marcel_gluec_exec.h marcel_gluec_stdmem.h\
				marcel_gluec_realsym.h linux_jiffies.h linux_bitops.h\
				marcel_glue_pthread.h marcel_hwlocapi.h marcel_gluec_sysio.h


include_sysdir = 		$(includedir)/sys
nobase_include_sys_HEADERS = 	$(SOLARIS_HFILES) $(DARWIN_HFILES) $(FREEBSD_HFILES) $(LINUX_HFILES) $(SYS_HFILES)
