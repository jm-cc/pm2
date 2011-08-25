/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
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

#include "marcel.h"


int marcel_futex_wait(marcel_futex_t * futex, unsigned long *addr, unsigned long mask,
		      unsigned long val, unsigned long timeout)
{
	int ret;

	ma_fastlock_acquire(&futex->__lock);
	if (tbx_unlikely(((*addr) & mask) != val)) {
		ma_fastlock_release(&futex->__lock);
		return EWOULDBLOCK;
	}
	ret = __marcel_lock_timed_wait(&futex->__lock, ma_self(), timeout, MA_CHECK_INTR);
	ma_fastlock_release(&futex->__lock);

	return ret;
}

int marcel_futex_wake(marcel_futex_t * futex, unsigned nb)
{
	int woken;
	blockcell *c;

	ma_fastlock_acquire(&futex->__lock);

	woken = 0;
	c = MA_MARCEL_FASTLOCK_WAIT_LIST(&futex->__lock);
	while (nb && c) {
		__marcel_unregister_lock_spinlocked(&futex->__lock, c);
		c->blocked = tbx_false;
 
		if (ma_wake_up_thread(c->task))
			woken++;
	
		nb--;		
		c = c->next;
	}
	
	ma_fastlock_release(&futex->__lock);
	return woken;
}
