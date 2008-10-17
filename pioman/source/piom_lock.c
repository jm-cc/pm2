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

#ifdef MARCEL
/* Used by the application to lock a server */
int piom_lock(piom_server_t server)
{
    /* this lock is not reentrant ! */
    PIOM_BUG_ON(server->lock_owner == MARCEL_SELF);

    __piom_lock_server(server, MARCEL_SELF);
    return 0;
}

/* Used by the application to unlock a server */
int piom_unlock(piom_server_t server)
{
    /* check wether *we* have lock before unlocking */
    PIOM_BUG_ON(server->lock_owner != MARCEL_SELF);

    __piom_unlock_server(server);
    return 0;
}


/* locks a server
 * this function can't be called from the server's tasklet
 */
void 
__piom_lock_server(piom_server_t server,
		   marcel_task_t *owner)
{
    ma_spin_lock_softirq(&server->lock);
    ma_tasklet_disable(&server->poll_tasklet);
    server->lock_owner = owner;
}

/* locks a server from the server's tasklet*/
void 
__piom_trylock_server(piom_server_t server,
		      marcel_task_t *owner)
{
    ma_spin_lock_softirq(&server->lock);
    server->lock_owner = owner;
}

/* unlocks a server */
void 
__piom_unlock_server(piom_server_t server)
{
    server->lock_owner = NULL;
    ma_tasklet_enable(&server->poll_tasklet);
    ma_spin_unlock_softirq(&server->lock);
}

/* unlocks a server */
void 
__piom_tryunlock_server(piom_server_t server)
{
    server->lock_owner = NULL;
    ma_spin_unlock_softirq(&server->lock);
}

/* Locks a server and disable the tasklet
 * return  MARCEL_SELF if we already have the lock
 *  NULL overwise
 */
marcel_task_t *
piom_ensure_lock_server(piom_server_t server)
{
    if (server->lock_owner == MARCEL_SELF) {
	LOG_RETURN(MARCEL_SELF);
    }
    __piom_lock_server(server, MARCEL_SELF);
    return NULL;
}

/* Locks a server
 * return  MARCEL_SELF if we already have the lock
 *  NULL overwise
 */
marcel_task_t *
piom_ensure_trylock_from_tasklet(piom_server_t server)
{
    if(!server->lock_owner) {
	server->lock_owner = MARCEL_SELF;
	__piom_trylock_server(server, MARCEL_SELF);
	LOG_RETURN(MARCEL_SELF);
    } else if (server->lock_owner == MARCEL_SELF )
	LOG_RETURN(MARCEL_SELF);
    ma_tasklet_disable(&server->poll_tasklet);
    ma_spin_lock_softirq(&server->lock);
    server->lock_owner = MARCEL_SELF;
    return NULL;
}

/* Locks a server
 * return  MARCEL_SELF if we already have the lock
 *  NULL overwise
 */
marcel_task_t *
piom_ensure_trylock_server(piom_server_t server)
{
    if (server->lock_owner == MARCEL_SELF) {
	LOG_RETURN(MARCEL_SELF);
    }
    __piom_trylock_server(server, MARCEL_SELF);
    return NULL;
}

void 
piom_lock_server_owner(piom_server_t server,
		       marcel_task_t *owner)
{
    __piom_lock_server(server, owner);
}

void 
piom_restore_lock_server_locked(piom_server_t server,
				marcel_task_t *old_owner)
{
    if (!old_owner) {
	__piom_unlock_server(server);
    }
}

void 
piom_restore_trylocked_from_tasklet(piom_server_t server, 
				    marcel_task_t *old_owner)
{
    if(!old_owner){
	__piom_unlock_server(server);	
    } else {
	__piom_tryunlock_server(server);	
    }
}

void 
piom_restore_lock_server_trylocked(piom_server_t server,
				   marcel_task_t *old_owner)
{
    if (!old_owner) {
	__piom_tryunlock_server(server);
    }
}

void 
piom_restore_lock_server_unlocked(piom_server_t server,
				  marcel_task_t *old_owner)
{
    if (old_owner) {
	__piom_lock_server(server, old_owner);
    }
}

#endif	/* MARCEL */
