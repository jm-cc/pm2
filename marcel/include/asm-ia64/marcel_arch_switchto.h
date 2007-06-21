
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#section common
#ifndef MA__INTERRUPT_FIX_LWP
#depend "asm-generic/marcel_arch_switchto.h[]"
#endif

#section marcel_macros
#ifdef MA__INTERRUPT_FIX_LWP
/* TODO: ne pas le faire lorsqu'on fournit la TLS */

#define MA_ARCH_SWITCHTO_LWP_FIX(current) \
  do { \
	unsigned long *p_tp=__ma_get_lwp_var(ma_ia64_tp); \
	if (p_tp) { \
		register unsigned long reg asm("r13"); \
		*p_tp=reg; \
	} \
  } while(0)

#define MA_ARCH_INTERRUPT_ENTER_LWP_FIX(current, p_ucontext_t) \
  do { \
	unsigned long *ma_ia64_interrupt_save=__ma_get_lwp_var(ma_ia64_tp); \
	do { \
		__ma_get_lwp_var(ma_ia64_tp)=&(((ucontext_t *)p_ucontext_t)->uc_mcontext.sc_gr[13]) ; \
  	} while (0)

#define MA_ARCH_INTERRUPT_EXIT_LWP_FIX(current, p_ucontext_t) \
	do { \
		__ma_get_lwp_var(ma_ia64_tp)=ma_ia64_interrupt_save; \
	} while (0); \
  } while (0)

#endif

