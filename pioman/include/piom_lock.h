/* -*- Mode: C; c-basic-offset:4 ; -*- */
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
#define MARCEL_INTERNAL_INCLUDE

#include "pioman.h"

/* Mutual exclusion handling
 * 
 * Remarks about locking:
 * + most synchronisation is done through tasklets (see 
 *    marcel/include/linux_interrupt.h for a quick introduction)
 * + functions that access a server's data have to:
 *   + be called from the polling tasklet (ex: check_polling_for())
 *   + or
 *     . the server has to be locked
 *     . softirqs have to be disabled
 *     . the server's tasklet has to be disabled
 */
#if defined(PIOM_THREAD_ENABLED)
/* locks a server
 * this function can't be called from the server's tasklet
 */
void __piom_lock_server(piom_server_t server,
			piom_thread_t owner);

/* locks a server from the server's tasklet*/
void __piom_trylock_server(piom_server_t server,
			   piom_thread_t owner);

/* unlocks a server */
void __piom_unlock_server(piom_server_t server);

/* unlocks a server */
void __piom_tryunlock_server(piom_server_t server);

/* Locks a server and disable the tasklet
 * return  PIOM_SELF if we already have the lock
 *  NULL overwise
 */
piom_thread_t piom_ensure_lock_server(piom_server_t server);

/* Locks a server
 * return  PIOM_SELF if we already have the lock
 *  NULL overwise
 */
piom_thread_t piom_ensure_trylock_from_tasklet(piom_server_t server);

/* Locks a server
 * return  PIOM_SELF if we already have the lock
 *  NULL overwise
 */
piom_thread_t piom_ensure_trylock_server(piom_server_t server);

void piom_lock_server_owner(piom_server_t server,
			    piom_thread_t owner);

void piom_restore_lock_server_locked(piom_server_t server,
				     piom_thread_t old_owner);

void piom_restore_trylocked_from_tasklet(piom_server_t server, 
					 piom_thread_t old_owner);

void piom_restore_lock_server_trylocked(piom_server_t server,
					piom_thread_t old_owner);

void piom_restore_lock_server_unlocked(piom_server_t server,
				       piom_thread_t old_owner);



#ifdef MARCEL
#define _piom_spin_lock_softirq(lock)    ma_spin_lock_softirq(lock)
#define _piom_spin_unlock_softirq(lock)  ma_spin_unlock_softirq(lock)
#define _piom_spin_trylock_softirq(lock) ma_spin_trylock_softirq(lock)
#define _piom_write_lock_softirq(lock)   ma_write_lock_softirq(lock)
#define _piom_write_unlock_softirq(lock) ma_write_unlock_softirq(lock)
#define _piom_read_lock_softirq(lock)    ma_read_lock_softirq(lock)
#define _piom_read_unlock_softirq(lock)  ma_read_unlock_softirq(lock)

#define _piom_spin_lock(lock)   ma_spin_lock(lock)
#define _piom_spin_unlock(lock) ma_spin_unlock(lock)
#else
#define _piom_spin_lock_softirq(lock)    piom_spin_lock(lock)
#define _piom_spin_unlock_softirq(lock)  piom_spin_unlock(lock)
#define _piom_spin_trylock_softirq(lock) piom_spin_trylock(lock)
#define _piom_write_lock_softirq(lock)   piom_write_lock(lock)
#define _piom_write_unlock_softirq(lock) piom_write_unlock(lock)
#define _piom_read_lock_softirq(lock)    piom_read_lock(lock)
#define _piom_read_unlock_softirq(lock)  piom_read_unlock(lock)

#define _piom_spin_lock(lock)   piom_spin_lock(lock)
#define _piom_spin_unlock(lock) piom_spin_unlock(lock)
#endif

#else

#define __piom_lock_server(server, owner)                    (void) 0
#define __piom_unlock_server(server)                         (void) 0
#define _piom_spin_trylock_softirq(lock)                     (void) 0
#define piom_ensure_lock_server(server)                      0
#define piom_lock_server_owner(server, owner)                (void) 0
#define piom_restore_lock_server_locked(server, old_owner)   (void) 0
#define piom_restore_lock_server_unlocked(server, old_owner) (void) 0

#define _piom_spin_lock_softirq(lock)    (void) 0
#define _piom_spin_unlock_softirq(lock)  (void) 0
#define _piom_write_lock_softirq(lock)   (void) 0
#define _piom_write_unlock_softirq(lock) (void) 0
#define _piom_read_lock_softirq(lock)    (void) 0
#define _piom_read_unlock_softirq(lock)  (void) 0

#define _piom_spin_lock(lock)   (void) 0
#define _piom_spin_unlock(lock) (void) 0
#endif	/* MARCEL */


/* Events server's Mutex
 *
 * - Callbacks are ALWAYS called inside this mutex
 * - BLOCK_ONE|ALL callbacks have to unlock it before the syscall 
 *     and lock it after using piom_callback_[un]lock().
 * - Previous functions can be called with or without this lock
 * - The mutex is unlocked in the waiting functions (piom_*wait*())
 * - The callbacks and the wake up of the threads waiting for the
 *    events signaled by the callback are atomic.
 * - If the mutex is held before the waiting function ((piom_wait_*),
 *    the wake up will be atomic (ie: a callback signaling the waited 
 *    event will wake up the thread).
 * - If a request has the ONE_SHOT attribute, the disactivation is atomic
 */

#ifdef PIOM_THREAD_ENABLED
int piom_lock(piom_server_t server);
int piom_unlock(piom_server_t server);
#else
#define piom_lock(server)    (void) 0
#define piom_unlock(server)  (void) 0
#endif	/* PIOM_THREAD_ENABLED */

#endif	/* PIOM_LOCK_H */
