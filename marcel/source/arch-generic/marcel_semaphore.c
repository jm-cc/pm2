
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

#include "marcel.h"

#ifndef MA_HAVE_RWLOCK
/*
 * rw spinlock fallbacks
 */
#ifdef MA__LWPS
void __ma_read_lock_failed(ma_rwlock_t * rw)
{
	do {
		ma_atomic_inc(rw);
		while (ma_atomic_read(rw) < 0);
	} while (ma_atomic_add_negative(-1, rw));
}

void __ma_write_lock_failed(ma_rwlock_t * rw)
{
	do {
		ma_atomic_add(MA_RW_LOCK_BIAS, rw);
		while (ma_atomic_read(rw) != MA_RW_LOCK_BIAS);
	} while (!ma_atomic_sub_and_test(MA_RW_LOCK_BIAS, rw));
}
#endif
#endif
