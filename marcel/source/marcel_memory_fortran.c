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

#if defined(MARCEL_MAMI_ENABLED) && defined(MARCEL_FORTRAN)

#include "marcel.h"

void marcel_memory_init_(int *memory_manager);
void marcel_memory_malloc_(int *memory_manager,
			   int *size,
			   int *buffer);
void marcel_memory_locate_(int *memory_manager,
			   int *address,
			   int *node,
			   int *err);
void marcel_memory_print_(int *memory_manager);

void marcel_memory_init_(int *memory_manager) {
  marcel_memory_manager_t *manager = TBX_MALLOC(sizeof(marcel_memory_manager_t));
  marcel_memory_init(manager);
  *memory_manager = (intptr_t)manager;
}

void marcel_memory_malloc_(int *memory_manager,
			   int *size,
			   int *buffer) {
  marcel_memory_manager_t *manager = (void *)*memory_manager;
  int *_buffer = marcel_memory_malloc(manager, *size*sizeof(int));
  *buffer = _buffer;
}

void marcel_memory_locate_(int *memory_manager,
			   int *address,
			   int *node,
			   int *err) {
  int _node, _err;
  marcel_memory_manager_t *manager = (void *)*memory_manager;
  _err = marcel_memory_locate(manager, *address, &_node);
  *err = _err;
  *node = _node;
}

void marcel_memory_print_(int *memory_manager) {
  marcel_memory_manager_t *manager = (void *)*memory_manager;
  marcel_memory_print(manager);
}

#endif
