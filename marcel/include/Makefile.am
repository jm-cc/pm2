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

SUBDIRS = scheduler sys

all-local:
	[ -L asm ]       || $(LN_S) $(abs_top_srcdir)/include/asm-$(TARGET_ASM) asm
	[ -L sys/os ]    || $(LN_S) $(abs_top_srcdir)/include/sys/$(TARGET_OS) sys/os

marcel_abi.h: all-local
	@[ -f $@ ] || (hash=`$(top_builddir)/scripts/gen-header/cpp-hash.sh "$(abs_top_srcdir)/include/marcel.h"` ;	\
	echo "/* Automatically generated, do not edit.  */" ;							\
	echo "#ifndef __MARCEL_ABI_H__" ; 									\
	echo "#define MARCEL_HEADER_HASH \"$$hash\"" ; 								\
	echo "#endif /** __MARCEL_ABI_H__ **/" ) > "$(abs_top_builddir)/include/$@"

scheduler/marcel_bubble_sched_lookup.h: marcel_abi.h
	@[ -f $@ ] || $(top_builddir)/scripts/gen-header/scheduler-list.sh > "$(abs_top_builddir)/include/$@"

clean-local:
	$(RM_F) asm
	$(RM_F) sys/os
	$(RM_F) $(abs_top_builddir)/include/marcel_abi.h
	$(RM_F) $(abs_top_builddir)/include/scheduler/marcel_bubble_sched_lookup.h
	$(RM_F) $(abs_top_builddir)/include/sys/marcel_rtld_savespace.h

install-data-hook:
	[ -L $(prefix)/include/sys/os ] || $(LN_SF) $(prefix)/include/sys/$(TARGET_OS) $(prefix)/include/sys/os
	[ -L $(prefix)/include/asm ] || $(LN_SF) $(prefix)/include/asm-$(TARGET_ASM) $(prefix)/include/asm

uninstall-hook:
	[ -L $(prefix)/include/sys/os ] || $(RM_F) $(prefix)/include/sys/os
	[ -L $(prefix)/include/asm ] || $(RM_F) $(prefix)/include/asm


ASM_GENERIC_HFILES = 	asm-generic/marcel_ctx.h asm-generic/marcel_ctx_jmp.h\
			asm-generic/linux_perlwp.h asm-generic/linux_spinlock.h\
			asm-generic/linux_linkage.h asm-generic/nptl_types.h\
			asm-generic/inlinefunctions/marcel_compareexchange.h\
			asm-generic/inlinefunctions/linux_rwlock.h\
			asm-generic/inlinefunctions/linux_atomic.h\
			asm-generic/inlinefunctions/marcel_testandset.h\
			asm-generic/inlinefunctions/allfunctions.h\
			asm-generic/inlinefunctions/linux_bitops.h\
			asm-generic/linux_types.h asm-generic/marcel_compareexchange.h\
			asm-generic/linux_rwlock.h asm-generic/linux_atomic.h\
			asm-generic/marcel_testandset.h asm-generic/marcel_archdep.h\
			asm-generic/marcel_arch_switchto.h asm-generic/linux_bitops.h\
			asm-generic/linux_hardirq.h

ASM_PPC64_HFILES = 	asm-ppc64/marcel_ctx.h asm-ppc64/marcel_ctx_jmp.h asm-ppc64/linux_perlwp.h\
			asm-ppc64/linux_spinlock.h asm-ppc64/linux_linkage.h\
			asm-ppc64/nptl_types.h asm-ppc64/linux_system.h\
			asm-ppc64/inlinefunctions/linux_system.h\
			asm-ppc64/inlinefunctions/marcel_compareexchange.h\
			asm-ppc64/inlinefunctions/linux_rwlock.h\
			asm-ppc64/inlinefunctions/linux_atomic.h\
			asm-ppc64/inlinefunctions/marcel_testandset.h\
			asm-ppc64/inlinefunctions/allfunctions.h\
			asm-ppc64/inlinefunctions/linux_bitops.h\
			asm-ppc64/linux_types.h asm-ppc64/marcel_compareexchange.h\
			asm-ppc64/linux_rwlock.h asm-ppc64/linux_atomic.h\
			asm-ppc64/marcel_testandset.h asm-ppc64/marcel_archdep.h\
			asm-ppc64/marcel_arch_switchto.h asm-ppc64/linux_bitops.h\
			asm-ppc64/linux_hardirq.h

ASM_I386_HFILES = 	asm-i386/marcel_ctx.h asm-i386/marcel_ctx_jmp.h asm-i386/linux_perlwp.h\
			asm-i386/linux_spinlock.h asm-i386/linux_linkage.h asm-i386/linux_system.h\
			asm-i386/nptl_types.h asm-i386/inlinefunctions/marcel_ctx_jmp.h\
			asm-i386/inlinefunctions/linux_spinlock.h\
			asm-i386/inlinefunctions/linux_system.h\
			asm-i386/inlinefunctions/linux_rwlock.h\
			asm-i386/inlinefunctions/linux_atomic.h\
			asm-i386/inlinefunctions/marcel_testandset.h\
			asm-i386/inlinefunctions/marcel_archdep.h\
			asm-i386/inlinefunctions/allfunctions.h\
			asm-i386/inlinefunctions/linux_bitops.h\
			asm-i386/linux_types.h asm-i386/marcel_compareexchange.h\
			asm-i386/linux_rwlock.h asm-i386/linux_atomic.h\
			asm-i386/marcel_testandset.h asm-i386/marcel_archdep.h\
			asm-i386/marcel_arch_switchto.h asm-i386/linux_bitops.h\
			asm-i386/linux_hardirq.h

ASM_SPARC64_HFILES = 	asm-sparc64/marcel_ctx.h asm-sparc64/marcel_ctx_jmp.h\
			asm-sparc64/linux_perlwp.h asm-sparc64/linux_spinlock.h\
			asm-sparc64/linux_linkage.h asm-sparc64/linux_system.h\
			asm-sparc64/nptl_types.h\
			asm-sparc64/inlinefunctions/marcel_compareexchange.h\
			asm-sparc64/inlinefunctions/linux_rwlock.h\
			asm-sparc64/inlinefunctions/linux_atomic.h\
			asm-sparc64/inlinefunctions/marcel_testandset.h\
			asm-sparc64/inlinefunctions/allfunctions.h\
			asm-sparc64/inlinefunctions/linux_bitops.h\
			asm-sparc64/linux_types.h asm-sparc64/marcel_compareexchange.h\
			asm-sparc64/linux_rwlock.h asm-sparc64/linux_atomic.h\
			asm-sparc64/marcel_testandset.h asm-sparc64/marcel_archdep.h\
			asm-sparc64/marcel_arch_switchto.h asm-sparc64/linux_bitops.h\
			asm-sparc64/linux_hardirq.h

ASM_IA64_HFILES = 	asm-ia64/marcel_ctx.h asm-ia64/marcel_ctx_jmp.h asm-ia64/linux_perlwp.h\
			asm-ia64/linux_spinlock.h asm-ia64/linux_intel_intrin.h\
			asm-ia64/linux_linkage.h asm-ia64/linux_system.h\
			asm-ia64/linux_intrinsics.h asm-ia64/nptl_types.h\
			asm-ia64/inlinefunctions/linux_spinlock.h\
			asm-ia64/inlinefunctions/linux_rwlock.h\
			asm-ia64/inlinefunctions/linux_atomic.h\
			asm-ia64/inlinefunctions/marcel_testandset.h\
			asm-ia64/inlinefunctions/marcel_archdep.h\
			asm-ia64/inlinefunctions/allfunctions.h\
			asm-ia64/inlinefunctions/linux_bitops.h\
			asm-ia64/linux_gcc_intrin.h asm-ia64/linux_types.h\
			asm-ia64/marcel_compareexchange.h asm-ia64/linux_rwlock.h\
			asm-ia64/linux_ia64regs.h asm-ia64/linux_atomic.h\
			asm-ia64/marcel_testandset.h asm-ia64/marcel_archdep.h\
			asm-ia64/marcel_arch_switchto.h asm-ia64/linux_bitops.h\
			asm-ia64/linux_hardirq.h asm-ia64/marcel_helper.h

ASM_X86_64_HFILES =  	asm-x86_64/marcel_ctx.h asm-x86_64/linux_hardirq.h\
			asm-x86_64/marcel_ctx_jmp.h asm-x86_64/linux_perlwp.h\
			asm-x86_64/linux_spinlock.h asm-x86_64/linux_linkage.h\
			asm-x86_64/linux_system.h asm-x86_64/nptl_types.h\
			asm-x86_64/inlinefunctions/marcel_ctx_jmp.h\
			asm-x86_64/inlinefunctions/linux_spinlock.h\
			asm-x86_64/inlinefunctions/linux_system.h\
			asm-x86_64/inlinefunctions/linux_rwlock.h\
			asm-x86_64/inlinefunctions/linux_atomic.h\
			asm-x86_64/inlinefunctions/marcel_testandset.h\
			asm-x86_64/inlinefunctions/allfunctions.h\
			asm-x86_64/inlinefunctions/linux_bitops.h\
			asm-x86_64/linux_types.h asm-x86_64/marcel_compareexchange.h\
			asm-x86_64/linux_rwlock.h asm-x86_64/linux_atomic.h\
			asm-x86_64/marcel_testandset.h asm-x86_64/marcel_archdep.h\
			asm-x86_64/marcel_arch_switchto.h asm-x86_64/linux_bitops.h

MARCEL_HFILES = 	marcel_supervisor.h marcel_polling.h marcel_topology.h marcel_rwlock.h\
			marcel.h marcel_top.h marcel_signal.h marcel_alloc.h\
			marcel_lpt_mutex.h marcel_errno.h marcel_lpt_cond.h marcel_deviate.h\
			marcel_descr.h marcel_cond.h marcel_init.h\
			inlinefunctions/marcel_topology.h inlinefunctions/marcel_alloc.h\
			inlinefunctions/marcel_descr.h inlinefunctions/marcel_timer.h\
			inlinefunctions/marcel_threads.h inlinefunctions/marcel_mutex.h\
			inlinefunctions/marcel_keys.h inlinefunctions/allfunctions.h\
			marcel_timer.h pthread/pthread.h marcel_fortran.h marcel_sem.h\
			marcel_futex.h marcel_seed.h marcel_threads.h\
			marcel_exception.h marcel_attr.h marcel_spin.h marcel_mutex.h\
			marcel_io.h marcel_locking.h marcel_lpt_barrier.h marcel_config.h\
			marcel_pmarcel.h marcel_keys.h marcel_stats.h marcel_barrier.h\
			marcel_fork.h

GEN_HFILES = 		marcel_abi.h scheduler/marcel_bubble_sched_lookup.h


nobase_include_HEADERS = 	$(MARCEL_HFILES) $(ASM_X86_64_HFILES)\
				$(ASM_I386_HFILES) $(ASM_IA64_HFILES) $(ASM_SPARC64_HFILES)\
				$(ASM_PPC64_HFILES) $(ASM_GENERIC_HFILES)

nodist_include_HEADERS = 	$(GEN_HFILES)
