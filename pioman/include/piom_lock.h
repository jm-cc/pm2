
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

#ifndef PIOM_LOCK_H
#define PIOM_LOCK_H

#ifdef MARCEL

#define piom_spinlock_t ma_spinlock_t
#define piom_spin_lock_init(lock)      ma_spin_lock_init(lock)
#define piom_spin_lock(lock) 	       ma_spin_lock_softirq(lock)
#define piom_spin_unlock(lock) 	       ma_spin_unlock_softirq(lock)

#else /* MARCEL */

#define piom_spinlock_t
#define piom_spin_lock_init(lock)      (void) 0
#define piom_spin_lock(lock) 	       (void) 0
#define piom_spin_unlock(lock) 	       (void) 0

#endif /* MARCEL */

#endif
