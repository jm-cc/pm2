
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
extern ma_spinlock_t compareexchange_spinlock;

#section marcel_macros
#define sync() __asm__ __volatile__ ("sync")

#section marcel_functions
static __inline__ unsigned long pm2_compareexchange (volatile void *p, unsigned long oldval, unsigned long newval, int size);
#section marcel_inline
static __inline__ unsigned long pm2_compareexchange (volatile void *p, unsigned long oldval, unsigned long newval, int size)
{
  unsigned long prev;

  if (size == 4) {
    sync();
    __asm__ __volatile__(
  		       "0:    lwarx %0,0,%1 ;"
  		       "      xor. %0,%3,%0;"
  		       "      bne 1f;"
  		       "      stwcx. %2,0,%1;"
  		       "      bne- 0b;"
  		       "1:    "
  	: "=&r"(prev)
  	: "r"(p), "r"(newval), "r"(oldval)
  	: "cr0", "memory");
    sync();
  } else {
    ma_spin_lock_softirq(&compareexchange_spinlock);
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
    }
    ma_spin_unlock_softirq(&compareexchange_spinlock);
  }
  return prev;
}

#section marcel_macros
#define ma_cmpxchg(ptr,o,n)\
	((__typeof__(*(ptr)))pm2_compareexchange((ptr),(unsigned long)(o),\
					(unsigned long)(n),sizeof(*(ptr))))
