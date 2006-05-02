
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

/*
 * Similar to:
 * include/linux/percpu.h
 */

#section marcel_macros
#depend "asm/linux_perlwp.h[marcel_macros]"
/* Must be an lvalue. */
#define ma_get_lwp_var(var) (*({ ma_preempt_disable(); &__ma_get_lwp_var(var); }))
#define ma_put_lwp_var(var) ma_preempt_enable()


