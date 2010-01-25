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


#ifndef __MARCEL_SUPERVISOR_H__
#define __MARCEL_SUPERVISOR_H__


/** Public functions **/
void
marcel_supervisor_init (void);

void
marcel_supervisor_sync (void);

void
marcel_supervisor_enable_nbour_vp (int vpnum);

void
marcel_supervisor_disable_nbour_vp (int vpnum);

void
marcel_supervisor_enable_nbour_vpset (const marcel_vpset_t * vpset);

void
marcel_supervisor_disable_nbour_vpset (const marcel_vpset_t * vpset);


#endif /** __MARCEL_SUPERVISOR_H__ **/
