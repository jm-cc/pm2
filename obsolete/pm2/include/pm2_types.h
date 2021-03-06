
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

#ifndef PM2_TYPES_EST_DEF
#define PM2_TYPES_EST_DEF

#include "marcel.h"

#define _PRIVATE_

typedef void (*pm2_startup_func_t)(int argc, char *argv[], void *args);

typedef void (*pm2_rawrpc_func_t)(void);

typedef void (*pm2_completion_handler_t)(void *);

typedef void (*pm2_pre_migration_hook)(marcel_t pid);
typedef any_t (*pm2_post_migration_hook)(marcel_t pid);
typedef void (*pm2_post_post_migration_hook)(any_t key);


_PRIVATE_ typedef struct pm2_completion_struct_t {
  marcel_sem_t sem;
  int proc;
  struct pm2_completion_struct_t *c_ptr;
  pm2_completion_handler_t handler;
  void *arg;
} pm2_completion_t;

_PRIVATE_ typedef struct {
  void *stack_base;
  marcel_t task;
  unsigned long depl, blk;
} pm2_migr_ctl;

#endif
