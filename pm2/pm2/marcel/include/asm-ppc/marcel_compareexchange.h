
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

#section macros
#define MA_HAVE_COMPAREEXCHANGE 1

#section marcel_variables
extern ma_spinlock_t ma_compareexchange_spinlock;

#section marcel_functions
static __inline__ unsigned long pm2_compareexchange (volatile void *ptr, unsigned long old, unsigned long new, int size);
#section marcel_inline
static __inline__ unsigned long pm2_compareexchange (volatile void *ptr, unsigned long old, unsigned long new, int size)
{
  unsigned long prev;

  if (size == 4) {
    volatile int *p = ptr;
    __asm__ __volatile__(
  		       "1:    lwarx %0,0,%2 ;"
  		       "      cmpw 0,%0,%3;"
  		       "      bne 2f;"
  		       "      stwcx. %4,0,%2;"
  		       "      bne- 1b;"
#ifdef MA__LWPS
		       "      sync;"
#endif
  		       "2:    "
  	: "=&r"(prev), "=m" (*p)
  	: "r"(p), "r" (old), "r"(new), "m" (*p)
  	: "cc", "memory");
  } else {
    ma_spin_lock_softirq(&ma_compareexchange_spinlock);
    switch(size) {
	case 1: {
			volatile ma_u8 *p = ptr;
			if (old == (prev=*p))
				*p = new;
			break;
		}
	case 2: {
			volatile ma_u16 *p = ptr;
			if (old == (prev=*p))
				*p = new;
			break;
		}
	default:
		MA_BUG();
    }
    ma_spin_unlock_softirq(&ma_compareexchange_spinlock);
  }
  return prev;
}

#section marcel_macros
#define ma_cmpxchg(ptr,o,n)\
	((__typeof__(*(ptr)))pm2_compareexchange((ptr),(unsigned long)(o),\
					(unsigned long)(n),sizeof(*(ptr))))
