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


#ifndef __MARCEL_SPIN_H__
#define __MARCEL_SPIN_H__


#include "marcel_config.h"
#include "sys/linux_spinlock.h"
#include "sys/marcel_types.h"
#include "sys/marcel_alias.h"


/** Public macros **/
#define MARCEL_SPINLOCK_INITIALIZER { .lock = MA_SPIN_LOCK_UNLOCKED }
#ifdef MA__IFACE_PMARCEL
#  define PMARCEL_SPINLOCK_INITIALIZER MARCEL_SPINLOCK_INITIALIZER
#endif


/** Public data types **/
struct marcel_spinlock {
	ma_spinlock_t lock;
};


/** Public functions **/
DEC_MARCEL(int, spin_init, (marcel_spinlock_t * lock, int pshared) __THROW);
DEC_MARCEL(int, spin_destroy, (marcel_spinlock_t * lock) __THROW);
DEC_MARCEL(int, spin_lock, (marcel_spinlock_t * lock) __THROW);
DEC_MARCEL(int, spin_trylock, (marcel_spinlock_t * lock) __THROW);
DEC_MARCEL(int, spin_unlock, (marcel_spinlock_t * lock) __THROW);


#endif /** __MARCEL_SPIN_H__ **/
