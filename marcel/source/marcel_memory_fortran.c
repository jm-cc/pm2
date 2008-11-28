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

void marcel_memory_init_(int *memory_manager) {
  marcel_memory_manager_t *manager = TBX_MALLOC(sizeof(marcel_memory_manager_t));
  marcel_memory_init(manager);
  *memory_manager = (intptr_t)manager;
}

void marcel_memory_exit_(int *memory_manager) {
  marcel_memory_manager_t *manager = (void *)*memory_manager;
  marcel_memory_exit(manager);
}

void marcel_memory_unset_alignment_(int *memory_manager) {
  marcel_memory_manager_t *manager = (void *)*memory_manager;
  marcel_memory_unset_alignment(manager);
}

void marcel_memory_malloc_(int *memory_manager,
			   int *size,
			   int *buffer) {
  marcel_memory_manager_t *manager = (void *)*memory_manager;
  int *_buffer = marcel_memory_malloc(manager, *size*sizeof(int));
  *buffer = (int)_buffer;
}

void marcel_memory_free_(int *memory_manager,
			 int *buffer) {
  marcel_memory_manager_t *manager = (void *)*memory_manager;
  marcel_memory_free(manager, (void *)buffer);
}

void marcel_memory_register_(int *memory_manager,
			     int *buffer,
			     int *size,
			     int *err) {
  int _err;
  marcel_memory_manager_t *manager = (void *)*memory_manager;
  _err = marcel_memory_register(manager, (void *)buffer, *size);
  *err = _err;
}

void marcel_memory_unregister_(int *memory_manager,
			       int *buffer,
			       int *err) {

  int _err;
  marcel_memory_manager_t *manager = (void *)*memory_manager;
  _err = marcel_memory_unregister(manager, (void *)buffer);
  *err = _err;
}

void marcel_memory_locate_(int *memory_manager,
			   int *address,
                           int *size,
			   int *node,
			   int *err) {
  int _node, _err;
  marcel_memory_manager_t *manager = (void *)*memory_manager;
  _err = marcel_memory_locate(manager, (void *)address, *size, &_node);
  *err = _err;
  *node = _node;
}

void marcel_memory_print_(int *memory_manager) {
  marcel_memory_manager_t *manager = (void *)*memory_manager;
  marcel_memory_print(manager);
}

void marcel_memory_task_attach_(int *memory_manager,
				int *buffer,
                                int *size,
				marcel_t *owner,
                                int *node,
				int *err) {
  int _err, _node;
  marcel_memory_manager_t *manager = (void *)*memory_manager;
  _err = marcel_memory_task_attach(manager, (void *)buffer, *size, *owner, &_node);
  *node = _node;
  *err = _err;
}

void marcel_memory_task_unattach_(int *memory_manager,
				  int *buffer,
				  marcel_t *owner,
				  int *err) {
  int _err;
  marcel_memory_manager_t *manager = (void *)*memory_manager;
  _err = marcel_memory_task_unattach(manager, (void *)buffer, *owner);
  *err = _err;
}

#endif
