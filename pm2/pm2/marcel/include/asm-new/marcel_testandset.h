
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

#section marcel_functions
#if 0
static __inline__ unsigned 
pm2_spinlock_testandset(volatile unsigned *spinlock) ;//__tbx_deprecated__;
#endif
#section marcel_inline
#if 0
static __inline__ unsigned __tbx_deprecated__
pm2_spinlock_testandset(volatile unsigned *spinlock)
{
#error "to write !"
}
#endif

//#warning "Bad implementation of test and set !"

#section marcel_macros
#define pm2_spinlock_testandset(spinlock) \
  (*(spinlock) ? 1 : (*(spinlock)=1,0))

#section marcel_macros
#define pm2_spinlock_release(spinlock) (*(spinlock) = 0)

