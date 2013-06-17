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
#define piom_thread_yield  marcel_yield
#define piom_thread_t      marcel_t
#define piom_thread_create marcel_create
#define piom_thread_join   marcel_join
#elif defined(PIOMAN_PTHREAD)
#include <pthread.h>
#define piom_thread_yield  pthread_yield
#define piom_thread_t      pthread_t
#define piom_thread_create pthread_create
#define piom_thread_join   pthread_join
#else /* PIOMAN_PTHREAD */
#error "Unknown multithreading lib"
#endif /* PIOMAN_PTHREAD */
#endif /* PIOMAN_MULTITHREAD */

