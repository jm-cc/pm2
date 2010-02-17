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

/* TODO: add some comments on locking in PIOMan */
/* TODO: inline some functions */

#if defined(PIOM_THREAD_ENABLED)

void piom_server_lock_from_callback(piom_server_t server);
void piom_server_unlock_from_callback(piom_server_t server);

piom_thread_t piom_server_lock_reentrant(piom_server_t server);
void piom_server_unlock_reentrant(piom_server_t server, piom_thread_t old_owner);

piom_thread_t piom_server_lock_reentrant_from_callback(piom_server_t server);
void piom_server_relock_reentrant_from_callback(piom_server_t server, piom_thread_t old_owner);
void piom_server_unlock_reentrant_from_callback(piom_server_t server, piom_thread_t old_owner);

void piom_server_relock_reentrant(piom_server_t server, piom_thread_t old_owner);

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

#else  /* MARCEL */

#define _piom_spin_lock_softirq(lock)    piom_spin_lock(lock)
#define _piom_spin_unlock_softirq(lock)  piom_spin_unlock(lock)
#define _piom_spin_trylock_softirq(lock) piom_spin_trylock(lock)
#define _piom_write_lock_softirq(lock)   piom_write_lock(lock)
#define _piom_write_unlock_softirq(lock) piom_write_unlock(lock)
#define _piom_read_lock_softirq(lock)    piom_read_lock(lock)
#define _piom_read_unlock_softirq(lock)  piom_read_unlock(lock)

#define _piom_spin_lock(lock)   piom_spin_lock(lock)
#define _piom_spin_unlock(lock) piom_spin_unlock(lock)
#endif	/* MARCEL */

int piom_server_lock(piom_server_t server);
int piom_server_unlock(piom_server_t server);
#else  /* PIOM_THREAD_ENABLED */


#define piom_server_lock_from_callback(server)   (void) 0
#define piom_server_unlock_from_callback(server) (void) 0

#define piom_server_lock_reentrant(server) 0
#define piom_server_unlock_reentrant(server, old_owner) (void) 0

#define piom_server_lock_reentrant_from_callback(server) (void) 0
#define piom_server_relock_reentrant_from_callback(server, old_owner) (void) 0
#define piom_server_unlock_reentrant_from_callback(server, old_owner) (void) 0

#define piom_server_relock_reentrant(server, old_owner) (void) 0

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
#define piom_server_lock(server)    (void) 0
#define piom_server_unlock(server)  (void) 0
#endif	/* PIOM_THREAD_ENABLED */

#endif	/* PIOM_LOCK_H */
