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

#if defined(PIOM_THREAD_ENABLED)

static __tbx_inline__
void __piom_server_lock_from_spare_vp(piom_spinlock_t* lock)
{
#ifdef PIOM_BLOCKING_CALLS
    int local_vp=marcel_current_vp();
    if(!marcel_spin_trylock(lock)) {
	if(marcel_current_vp() >= marcel_nbvps()){
	    /* We are on a spare VP (RT thread with high priority). 
	     * So there's no chance that the spinlock gets unlocked.
	     * Release the CPU so that the corresponding thread can unlock the spinlock
	     */
	    int prio = __piom_lower_my_priority();
	    while(!marcel_spin_trylock(lock)){
		sched_yield();
	    }
	    __piom_higher_my_priority(prio);
	}else{
	    while(!marcel_spin_trylock(lock));
	}
    }
#else
    _piom_spin_lock_softirq(lock);
#endif
}

static __tbx_inline__
void __piom_server_unlock_from_spare_vp(piom_spinlock_t* lock)
{
#ifdef PIOM_BLOCKING_CALLS
    marcel_spin_unlock(lock);
#else
    _piom_spin_unlock_softirq(lock);
#endif
}

/* locks a server
 * this function can't be called from the server's tasklet
 */
static __tbx_inline__
void 
__piom_server_lock(piom_server_t server,
		   piom_thread_t owner)
{
    __piom_server_lock_from_spare_vp(&server->lock);
#ifdef PIOM_USE_TASKLETS
    /* todo: fix me ! */
    //ma_tasklet_disable(&server->poll_tasklet);
    marcel_tasklet_disable();
#endif
    server->lock_owner = owner;
}

/* unlocks a server */
static __tbx_inline__
void 
__piom_server_unlock(piom_server_t server)
{
    PIOM_BUG_ON(server->lock_owner != PIOM_SELF);
    server->lock_owner = PIOM_THREAD_NULL;
#ifdef PIOM_USE_TASKLETS
    /* todo: fix me ! */
    //ma_tasklet_enable(&server->poll_tasklet);
    marcel_tasklet_enable();
#endif
    __piom_server_unlock_from_spare_vp(&server->lock);
}

/* locks a server from the server callback */
static __tbx_inline__ 
void 
__piom_server_lock_from_callback(piom_server_t server,
				 piom_thread_t owner)
{
    __piom_server_lock_from_spare_vp(&server->lock);
    server->lock_owner = owner;
}

/* unlocks a server from a callback */
static __tbx_inline__ 
void 
__piom_server_unlock_from_callback(piom_server_t server)
{
    PIOM_BUG_ON(server->lock_owner != PIOM_SELF);
    server->lock_owner = PIOM_THREAD_NULL;
    __piom_server_unlock_from_spare_vp(&server->lock);
}


/* usual locking primitives:
 * when using tasklets, these locks disable softirq, so that
 * while the lock is hold, the thread cannot be preempted.
 * DO NOT call these function from a callback if you don't want 
 * the thread to hang forever !
 */

/* Lock a server 
 */
int piom_server_lock(piom_server_t server)
{
    /* this lock is not reentrant ! */
    PIOM_BUG_ON(server->lock_owner == PIOM_SELF);

    __piom_server_lock(server, PIOM_SELF);
    return 0;
}

/* Used by the application to unlock a server */
int piom_server_unlock(piom_server_t server)
{
    /* check wether *we* have locked before unlocking */
    PIOM_BUG_ON(server->lock_owner != PIOM_SELF);

    __piom_server_unlock(server);
    return 0;
}

/* Lock a server
 * This function if reentrant: if the calling thread already has 
    the lock, the function won't hang
 * return  PIOM_SELF if we already have the lock
 * NULL overwise
 */
piom_thread_t
piom_server_lock_reentrant(piom_server_t server)
{
    if (server->lock_owner == PIOM_SELF) {
	/* the calling thread already has the lock */
	PIOM_LOG_RETURN(PIOM_SELF);
    }
    /* the calling thread doesn't hold the lock */
    __piom_server_lock(server, PIOM_SELF);
    return PIOM_THREAD_NULL;
}

/* Unlock a server
 * @param old_owner: the thread_id that was returned by piom_server_lock_reentrant
 */
void 
piom_server_unlock_reentrant(piom_server_t server,
			     piom_thread_t old_owner)
{
    if (old_owner == PIOM_THREAD_NULL) {
	__piom_server_unlock(server);
    }
}

/* Lock a server that may have been unlocked.
 * This can be used when the current thread calls piom_server_lock_reentrant, 
 * then calls a function that unlocks the server. 
 * @param old_owner: the thread_id that was returned by piom_server_lock_reentrant
 */
void 
piom_server_relock_reentrant(piom_server_t server,
			     piom_thread_t old_owner)
{
    if (old_owner != PIOM_THREAD_NULL) {
	__piom_server_lock(server, old_owner);
    }
}

/*
 * 'safe' locking primitives:
 * these function can be called safely from a callback
 */

/* Locks a server from a callback
 */
void
piom_server_lock_from_callback(piom_server_t server)
{
    __piom_server_lock_from_spare_vp(&server->lock);
}

void 
piom_server_unlock_from_callback(piom_server_t server)
{
    __piom_server_unlock_from_callback(server);	
}


/* Locks a server from a callback.
 * This function is reentrant: if the calling thread already has the lock,
 * this function won't hang
 * return  PIOM_SELF if the calling thread already has the lock
 * NULL overwise
 */
piom_thread_t
piom_server_lock_reentrant_from_callback(piom_server_t server)
{
    if (server->lock_owner == PIOM_SELF)
	/* we already have the lock */
	return PIOM_SELF;

    __piom_server_lock_from_spare_vp(&server->lock);
    server->lock_owner = PIOM_SELF;
    return PIOM_THREAD_NULL;
}


/* Unlocks a server from a callback.
 */
void 
piom_server_unlock_reentrant_from_callback(piom_server_t server, 
					   piom_thread_t old_owner)
{
    if(old_owner == PIOM_THREAD_NULL){
	__piom_server_unlock_from_callback(server);	
    } 
}

#endif	/* PIOM_THREAD_ENABLED */

