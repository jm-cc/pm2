
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

#ifndef MARCEL_DEVIATE_EST_DEF
#define MARCEL_DEVIATE_EST_DEF


void marcel_enable_deviation(void);
void marcel_disable_deviation(void);

void marcel_deviate(marcel_t pid, handler_func_t h, any_t arg);


_PRIVATE_ typedef struct deviate_record_struct_t {
  handler_func_t func;
  any_t arg;
  struct deviate_record_struct_t *next;
} deviate_record_t;

_PRIVATE_ void marcel_execute_deviate_work(void);


#endif // MARCEL_DEVIATE_EST_DEF

