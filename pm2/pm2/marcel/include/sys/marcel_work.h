
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

#ifndef MARCEL_WORK_EST_DEF
#define MARCEL_WORK_EST_DEF

#ifdef MA__WORK

/* flags indiquant le type de travail à faire */
enum {
  MARCEL_WORK_DEVIATE = 0x1,
};

#define HAS_WORK(self)          ((self)->has_work)
#define HAS_DEVIATE_WORK(self)  ((self)->has_work & MARCEL_WORK_DEVIATE)

void do_work(marcel_t self);

#endif /* MA__WORK */

#endif /* MARCEL_WORK_EST_DEF */

