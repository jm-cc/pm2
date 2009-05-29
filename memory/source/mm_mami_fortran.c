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

#if defined(MM_MAMI_ENABLED) && defined(MM_FORTRAN)

#include "mm_mami_fortran.h"
#include "mm_mami.h"
#include "mm_mami_private.h"

void mami_init_(int *memory_manager) {
  mami_manager_t *manager;
  mami_init(&manager);
  *memory_manager = (intptr_t)manager;
}

void mami_exit_(int *memory_manager) {
  mami_manager_t *manager = (void *)*memory_manager;
  mami_exit(&manager);
}

void mami_unset_alignment_(int *memory_manager) {
  mami_manager_t *manager = (void *)*memory_manager;
  mami_unset_alignment(manager);
}

void mami_malloc_(int *memory_manager,
                  int *size,
                  int *policy,
                  int *node,
                  int *buffer) {
  mami_manager_t *manager = (void *)*memory_manager;
  int *_buffer = mami_malloc(manager, *size*sizeof(int), *policy, *node);
  *buffer = (int)_buffer;
}

void mami_free_(int *memory_manager,
                int *buffer) {
  mami_manager_t *manager = (void *)*memory_manager;
  mami_free(manager, (void *)buffer);
}

void mami_register_(int *memory_manager,
                    int *buffer,
                    int *size,
                    int *err) {
  int _err;
  mami_manager_t *manager = (void *)*memory_manager;
  _err = mami_register(manager, (void *)buffer, *size);
  *err = _err;
}

void mami_unregister_(int *memory_manager,
                      int *buffer,
                      int *err) {

  int _err;
  mami_manager_t *manager = (void *)*memory_manager;
  _err = mami_unregister(manager, (void *)buffer);
  *err = _err;
}

void mami_locate_(int *memory_manager,
                  int *address,
                  int *size,
                  int *node,
                  int *err) {
  int _node, _err;
  mami_manager_t *manager = (void *)*memory_manager;
  _err = mami_locate(manager, (void *)address, *size, &_node);
  *err = _err;
  *node = _node;
}

void mami_print_(int *memory_manager) {
  mami_manager_t *manager = (void *)*memory_manager;
  mami_print(manager);
}

void mami_task_attach_(int *memory_manager,
                       int *buffer,
                       int *size,
                       marcel_t *owner,
                       int *node,
                       int *err) {
  int _err, _node;
  mami_manager_t *manager = (void *)*memory_manager;
  _err = mami_task_attach(manager, (void *)buffer, *size, *owner, &_node);
  *node = _node;
  *err = _err;
}

void mami_task_unattach_(int *memory_manager,
                         int *buffer,
                         marcel_t *owner,
                         int *err) {
  int _err;
  mami_manager_t *manager = (void *)*memory_manager;
  _err = mami_task_unattach(manager, (void *)buffer, *owner);
  *err = _err;
}

#endif
