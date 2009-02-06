/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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

#section common
#ifdef MARCEL_FORTRAN

#section functions

extern
void marcel_init_(void);

extern
void marcel_end_(void);

extern
void marcel_self_(marcel_t *self);

extern
void marcel_create_(marcel_t *pid, void* (*func)(void*), void *arg);

extern
void marcel_join_(marcel_t *pid);

extern
void marcel_yield_(void);

extern
void marcel_set_load_(int *load);

extern
void marcel_rien_(void);

#ifdef MA__BUBBLES

extern
void marcel_bubble_submit_(void);

extern
void marcel_spread_(void);

#endif /* MA__BUBBLES */

#section common
#endif /* MARCEL_FORTRAN */
