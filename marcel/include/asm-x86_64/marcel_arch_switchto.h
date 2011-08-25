/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */


#ifndef __ASM_X86_64_MARCEL_ARCH_SWITCHTO_H__
#define __ASM_X86_64_MARCEL_ARCH_SWITCHTO_H__


#include "marcel_config.h"


#if defined(MA__LWPS) && !defined(MA__PROVIDE_TLS) && defined(__GLIBC__)

/* We need to save/restore the per-lwp rtld_must_xmm_save field on preemption */
/* Save per-lwp rtld_must_xmm_save into marcel_t field */
#define MA_ARCH_INTERRUPT_ENTER_LWP_FIX(current, p_ucontext_t) \
	(current)->rtld_must_xmm_save = MA_GET_TCB(rtld_must_xmm_save)

/* Restore marcel_t rtld_must_xmm_save into per-lwp */
#define MA_ARCH_INTERRUPT_EXIT_LWP_FIX(current, p_ucontext_t) \
	MA_SET_TCB(rtld_must_xmm_save, (current)->rtld_must_xmm_save)

/* When switching back to a preempted thread, restore its rtld_must_xmm_save to the lwp, too */
#define MA_ARCH_SWITCHTO_LWP_FIX(current) \
	MA_SET_TCB(rtld_must_xmm_save, (current)->rtld_must_xmm_save)

#else
#include "asm-generic/marcel_arch_switchto.h"
#endif


#ifdef MA__SELF_VAR_TLS
#  ifdef LINUX_SYS
#    define MA_SET_INITIAL_SELF(self)					\
 	do { unsigned long tmp TBX_UNUSED;						\
  	     asm volatile ("mov __ma_self@GOTTPOFF(%%rip), %0;\n\t"     \
			   "mov %1, %%fs:(%0)" : "=&r"(tmp) : "r"(self)); \
	} while (0)
#  endif
#endif


#endif /** __ASM_X86_64_MARCEL_ARCH_SWITCHTO_H__ **/
