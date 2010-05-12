
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
#include "marcel_fastlock.h"
#include <errno.h>

struct blockcell {
	marcel_t task;
	struct blockcell *next;
	struct blockcell **prev;	/* Address of pointer pointing to us */
};

int marcel_futex_wait(marcel_futex_t *futex, unsigned long *addr, unsigned long mask, unsigned long val, signed long timeout)
{
	ma_set_current_state(MA_TASK_INTERRUPTIBLE);
	if (((*addr) & mask) == val) {
		struct blockcell cell = { .task = MARCEL_SELF };
		struct blockcell **pcell;
		int ret = 0;

		/* Enqueue */
		ma_fastlock_acquire(&futex->__lock);
		pcell = (struct blockcell **) &futex->__lock.__status;
		cell.prev = pcell;
		cell.next = *pcell;
		if (cell.next)
			cell.next->prev = &cell.next;
		*pcell = &cell;
		ma_fastlock_release(&futex->__lock);

		if (((*addr) & mask) == val)
			ma_schedule_timeout(timeout);

		/* Dequeue if needed */
		ma_fastlock_acquire(&futex->__lock);
		if (cell.prev) {
			*cell.prev = cell.next;
			ret = ETIMEDOUT;
		}
		ma_fastlock_release(&futex->__lock);
		return ret;
	} else {
		ma_set_current_state(MA_TASK_RUNNING);
		return EAGAIN;
	}
}

int marcel_futex_wake(marcel_futex_t *futex, unsigned nb)
{
	struct blockcell *cell;
	struct blockcell **pcell;
	int woken = 0;

	/* Make sure the written value is seen by others */
	/* This is already done by acquire,
	ma_mb();
	 */

	ma_fastlock_acquire(&futex->__lock);
	pcell = (struct blockcell **) &futex->__lock.__status;
	cell = *pcell;
	while (nb && cell) {
		if (ma_wake_up_thread(cell->task)) {
			if (nb != UINT_MAX)
				nb--;
			woken++;
		}
		/* Dequeue and proceed */
		cell->prev = NULL;
		cell = cell->next;
	}
	*pcell = cell;
	if (cell)
		cell->prev = pcell;
	ma_fastlock_release(&futex->__lock);
	return woken;
}
