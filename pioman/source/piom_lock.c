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

#include "pioman.h"

#include <sched.h>

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

#if defined(PIOM_THREAD_ENABLED)

static __tbx_inline__
void __piom_lock_from_spare_vp(piom_spinlock_t* lock)
{
#ifdef PIOM_BLOCKING_CALLS
    int local_vp=marcel_current_vp();
    if(!ma_spin_trylock(lock)) {
	if(marcel_current_vp() >= marcel_nbvps()){
	    /* We are on a spare VP (RT thread with high priority). 
	     * So there's no chance that the spinlock gets unlocked.
	     * Release the CPU so that the corresponding thread can unlock the spinlock
	     */
	    int prio = __piom_lower_my_priority();
	    while(!ma_spin_trylock(lock)){
		sched_yield();
	    }
	    __piom_higher_my_priority(prio);
	}else{
	    while(!ma_spin_trylock(lock));
	}
    }
#else
    _piom_spin_lock_softirq(lock);
#endif
}

static __tbx_inline__
void __piom_unlock_from_spare_vp(piom_spinlock_t* lock)
{
#ifdef PIOM_BLOCKING_CALLS
    ma_spin_unlock(lock);
#else
    _piom_spin_unlock_softirq(lock);
#endif
}

/* Used by the application to lock a server */
int piom_lock(piom_server_t server)
{
    /* this lock is not reentrant ! */
    PIOM_BUG_ON(server->lock_owner == PIOM_SELF);

    __piom_lock_server(server, PIOM_SELF);
    return 0;
}

/* Used by the application to unlock a server */
int piom_unlock(piom_server_t server)
{
    /* check wether *we* have lock before unlocking */
    PIOM_BUG_ON(server->lock_owner != PIOM_SELF);

    __piom_unlock_server(server);
    return 0;
}


/* locks a server
 * this function can't be called from the server's tasklet
 */
void 
__piom_lock_server(piom_server_t server,
		   piom_thread_t owner)
{
    __piom_lock_from_spare_vp(&server->lock);
#ifdef PIOM_USE_TASKLETS
    ma_tasklet_disable(&server->poll_tasklet);
#endif
    server->lock_owner = owner;
}

/* locks a server from the server's tasklet*/
void 
__piom_trylock_server(piom_server_t server,
		      piom_thread_t owner)
{
    __piom_lock_from_spare_vp(&server->lock);
    server->lock_owner = owner;
}

/* unlocks a server */
void 
__piom_unlock_server(piom_server_t server)
{
    PIOM_BUG_ON(server->lock_owner != PIOM_SELF);
    server->lock_owner = PIOM_THREAD_NULL;
#ifdef PIOM_USE_TASKLETS
    ma_tasklet_enable(&server->poll_tasklet);
#endif
    __piom_unlock_from_spare_vp(&server->lock);
}

/* unlocks a server */
void 
__piom_tryunlock_server(piom_server_t server)
{
    PIOM_BUG_ON(server->lock_owner != PIOM_SELF);
    server->lock_owner = PIOM_THREAD_NULL;
    __piom_unlock_from_spare_vp(&server->lock);
}

/* Locks a server and disable the tasklet
 * return  PIOM_SELF if we already have the lock
 *  NULL overwise
 */
piom_thread_t
piom_ensure_lock_server(piom_server_t server)
{
    if (server->lock_owner == PIOM_SELF) {
	LOG_RETURN(PIOM_SELF);
    }
    __piom_lock_server(server, PIOM_SELF);
    return PIOM_THREAD_NULL;
}

/* Locks a server
 * return  PIOM_SELF if we already have the lock
 *  NULL overwise
 */
piom_thread_t
piom_ensure_trylock_from_tasklet(piom_server_t server)
{
    if (server->lock_owner == PIOM_SELF)
	/* we already have the lock */
	return PIOM_SELF;

    __piom_lock_from_spare_vp(&server->lock);
    server->lock_owner = PIOM_SELF;
    return PIOM_THREAD_NULL;
}

/* Locks a server
 * return  PIOM_SELF if we already have the lock
 *  NULL overwise
 */
piom_thread_t
piom_ensure_trylock_server(piom_server_t server)
{
    if (server->lock_owner == PIOM_SELF) {
	LOG_RETURN(PIOM_SELF);
    }
    __piom_trylock_server(server, PIOM_SELF);
    return PIOM_THREAD_NULL;
}

void 
piom_lock_server_owner(piom_server_t server,
		       piom_thread_t owner)
{
    __piom_lock_server(server, owner);
}

void 
piom_restore_lock_server_locked(piom_server_t server,
				piom_thread_t old_owner)
{
    if (old_owner == PIOM_THREAD_NULL) {
	__piom_unlock_server(server);
    }
}

void 
piom_restore_trylocked_from_tasklet(piom_server_t server, 
				    piom_thread_t old_owner)
{
    if(old_owner == PIOM_THREAD_NULL){
	__piom_tryunlock_server(server);	
    } 
}

void 
piom_restore_lock_server_trylocked(piom_server_t server,
				   piom_thread_t old_owner)
{
    if (old_owner == PIOM_THREAD_NULL) {
	__piom_tryunlock_server(server);
    }
}

void 
piom_restore_lock_server_unlocked(piom_server_t server,
				  piom_thread_t old_owner)
{
    if (old_owner != PIOM_THREAD_NULL) {
	__piom_lock_server(server, old_owner);
    }
}

#endif	/* PIOM_THREAD_ENABLED */
