/*
 * NewMadeleine
 * Copyright (C) 2013 (see AUTHORS file)
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

#include "../sendrecv/helper.h"

#ifdef PIOMAN
#include <pioman.h>
#endif /* PIOMAN */

#ifdef PIOMAN_MULTITHREAD
#if defined(PIOMAN_MARCEL)
#include <marcel.h>
#define piom_thread_yield   marcel_yield
#define piom_thread_t       marcel_t
#define piom_thread_create  marcel_create
#define piom_thread_join    marcel_join
#define piom_mutex_init     marcel_mutex_init
#define piom_mutex_lock     marcel_mutex_lock
#define piom_mutex_unlock   marcel_mutex_unlock
#define piom_cond_init      marcel_cond_init
#define piom_cond_wait      marcel_cond_wait
#define piom_cond_signal    marcel_cond_signal
#define piom_cond_broadcast marcel_cond_broadcast
#elif defined(PIOMAN_PTHREAD)
#include <pthread.h> 
#define piom_thread_yield   pthread_yield
#define piom_thread_t       pthread_t
#define piom_thread_create  pthread_create
#define piom_thread_join    pthread_join
#define piom_thread_mutex_t        pthread_mutex_t
#define piom_thread_mutex_init     pthread_mutex_init
#define piom_thread_mutex_lock     pthread_mutex_lock
#define piom_thread_mutex_unlock   pthread_mutex_unlock
#define piom_thread_cond_t         pthread_cond_t
#define piom_thread_cond_init      pthread_cond_init
#define piom_thread_cond_wait      pthread_cond_wait
#define piom_thread_cond_signal    pthread_cond_signal
#define piom_thread_cond_broadcast pthread_cond_broadcast
#else /* PIOMAN_PTHREAD */
#error "Unknown multithreading lib"
#endif /* PIOMAN_PTHREAD */
#endif /* PIOMAN_MULTITHREAD */

