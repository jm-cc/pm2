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

#ifdef MM_MAMI_ENABLED

#define MARCEL_INTERNAL_INCLUDE

#include <errno.h>
#include "mm_mami.h"
#include "mm_mami_private.h"
#include "mm_debug.h"
#include "mm_helper.h"

static
int _mami_entity_attach(mami_manager_t *memory_manager,
                        void *buffer,
                        size_t size,
                        marcel_entity_t *owner,
                        int *node) {
  int err=0;
  mami_data_t *data;

  MEMORY_ILOG_IN();
  marcel_mutex_lock(&(memory_manager->lock));

  mdebug_memory("Attaching [%p:%p:%ld] to entity %p\n", buffer, buffer+size, (long)size, owner);

  if (!buffer) {
    mdebug_memory("Cannot attach NULL buffer\n");
    errno = EINVAL;
    err = -errno;
  }
  else {
    void *aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
    void *aligned_endbuffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer+size, memory_manager->normalpagesize);
    size_t aligned_size = aligned_endbuffer-aligned_buffer;
    mami_data_link_t *area;

    if (!aligned_size) {
      aligned_endbuffer = aligned_buffer + memory_manager->normalpagesize;
      aligned_size = memory_manager->normalpagesize;
    }

    err = _mami_locate(memory_manager, memory_manager->root, aligned_buffer, 1, &data);
    if (err < 0) {
      mdebug_memory("The address interval [%p:%p] is not managed by MaMI. Let's register it\n", aligned_buffer, aligned_endbuffer);
      _mami_register(memory_manager, aligned_buffer, aligned_size, 0, &data);

      if (aligned_endbuffer > buffer+size) {
	data->mprotect_size = data->size - memory_manager->normalpagesize;
      }
      if (aligned_buffer < buffer) {
	data->mprotect_startaddress += memory_manager->normalpagesize;
	data->mprotect_size -= memory_manager->normalpagesize;
      }

      err = 0;
    }
    else {
      if (data->node == MAMI_FIRST_TOUCH_NODE || data->node == MAMI_UNKNOWN_LOCATION_NODE) {
        mdebug_memory("Need to find out the location of the memory area\n");
        _mami_get_pages_location(memory_manager, data->pageaddrs, data->nbpages, &(data->node), &(data->nodes));
      }
      if (size < data->size) {
        size_t newsize;
        mami_data_t *next_data;

        newsize = data->size-aligned_size;
        if (!newsize) {
          mdebug_memory("Cannot split a page\n");
        }
        else {
          mdebug_memory("Splitting [%p:%p] in [%p:%p] and [%p:%p]\n", data->startaddress,data->endaddress,
                      data->startaddress,data->startaddress+aligned_size, aligned_endbuffer,aligned_endbuffer+newsize);
          data->nbpages = aligned_size/memory_manager->normalpagesize;
          data->endaddress = data->startaddress + aligned_size;
          data->size = aligned_size;
          _mami_register(memory_manager, aligned_endbuffer, newsize, data->mami_allocated, &next_data);
          data->next = next_data;
        }
      }
    }

    mdebug_memory("Adding entity %p to data %p\n", owner, data);
    tbx_slist_push(data->owners, owner);

    *node = data->node;
    if (*node >= 0) {
      mdebug_memory("Adding %lu bits to memnode offset for node #%d\n", (long unsigned)data->size, *node);
      ((long *) ma_stats_get(owner, ma_stats_memnode_offset))[*node] += data->size;
    }
    else {
      mdebug_memory("Cannot attach data as location undefined #%d\n", *node);
    }

    area = tmalloc(sizeof(mami_data_link_t));
    area->data = data;
    INIT_LIST_HEAD(&(area->list));
    marcel_spin_lock(&(owner->memory_areas_lock));
    list_add(&(area->list), &(owner->memory_areas));
    marcel_spin_unlock(&(owner->memory_areas_lock));
  }
  marcel_mutex_unlock(&(memory_manager->lock));
  MEMORY_ILOG_OUT();
  return err;
}

static
int _mami_entity_unattach(mami_manager_t *memory_manager,
                          void *buffer,
                          marcel_entity_t *owner) {
  int err=0;
  mami_data_t *data = NULL;
  void *aligned_buffer;
  mami_data_link_t *area = NULL;

  MEMORY_ILOG_IN();
  marcel_mutex_lock(&(memory_manager->lock));
  aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);

  err = _mami_locate(memory_manager, memory_manager->root, aligned_buffer, 1, &data);
  if (err >= 0) {
    marcel_entity_t *res;

    mdebug_memory("Removing entity %p from data %p\n", owner, data);

    if (tbx_slist_is_nil(data->owners)) {
      mdebug_memory("The entity %p is not attached to the memory area %p(1).\n", owner, aligned_buffer);
      errno = ENOENT;
      err = -errno;
    }
    else {
      res = tbx_slist_search_and_extract(data->owners, NULL, owner);
      if (res == owner) {
        if (data->node >= 0) {
          mdebug_memory("Removing %lu bits from memnode offset for node #%d\n", (long unsigned)data->size, data->node);
          ((long *) ma_stats_get(owner, ma_stats_memnode_offset))[data->node] -= data->size;
        }

        mdebug_memory("Removing data %p from entity %p\n", data, owner);
	marcel_spin_lock(&(owner->memory_areas_lock));
	list_for_each_entry(area, &(owner->memory_areas), list) {
          if (area->data == data) {
            list_del_init(&area->list);
            tfree(area);
            break;
          }
        }
        marcel_spin_unlock(&(owner->memory_areas_lock));
      }
      else {
        mdebug_memory("The entity %p is not attached to the memory area %p.\n", owner, aligned_buffer);
        errno = ENOENT;
        err = -errno;
      }
    }
  }

  marcel_mutex_unlock(&(memory_manager->lock));
  if (!area) tfree(area);

  MEMORY_ILOG_OUT();
  return err;
}

static
int _mami_entity_unattach_all(mami_manager_t *memory_manager,
                              marcel_entity_t *owner) {
  mami_data_link_t *area, *narea;

  MEMORY_ILOG_IN();
  mdebug_memory("Unattaching all memory areas from entity %p\n", owner);
  //marcel_spin_lock(&(owner->memory_areas_lock));
  list_for_each_entry_safe(area, narea, &(owner->memory_areas), list) {
    _mami_entity_unattach(memory_manager, area->data->startaddress, owner);
  }
  //marcel_spin_unlock(&(owner->memory_areas_lock));
  MEMORY_ILOG_OUT();
  return 0;
}

static
int _mami_entity_migrate_all(mami_manager_t *memory_manager,
                             marcel_entity_t *owner,
                             int node) {
  mami_data_link_t *area, *narea;
  list_for_each_entry_safe(area, narea, &(owner->memory_areas), list) {
    _mami_migrate_pages(memory_manager, area->data, node);
  }
  return 0;
}

int mami_task_attach(mami_manager_t *memory_manager,
                     void *buffer,
                     size_t size,
                     marcel_t owner,
                     int *node) {
  marcel_entity_t *entity;
  entity = ma_entity_task(owner);
  return _mami_entity_attach(memory_manager, buffer, size, entity, node);
}

int mami_task_unattach(mami_manager_t *memory_manager,
                       void *buffer,
                       marcel_t owner) {
  marcel_entity_t *entity;
  entity = ma_entity_task(owner);
  return _mami_entity_unattach(memory_manager, buffer, entity);
}

int mami_task_unattach_all(mami_manager_t *memory_manager,
                           marcel_t owner) {
  marcel_entity_t *entity;
  entity = ma_entity_task(owner);
  return _mami_entity_unattach_all(memory_manager, entity);
}

int mami_task_migrate_all(mami_manager_t *memory_manager,
                          marcel_t owner,
                          int node) {
  marcel_entity_t *entity;
  entity = ma_entity_task(owner);
  return _mami_entity_migrate_all(memory_manager, entity, node);
}

int mami_bubble_attach(mami_manager_t *memory_manager,
                       void *buffer,
                       size_t size,
                       marcel_bubble_t *owner,
                       int *node) {
  marcel_entity_t *entity;
  entity = ma_entity_bubble(owner);
  return _mami_entity_attach(memory_manager, buffer, size, entity, node);
}

int mami_bubble_unattach(mami_manager_t *memory_manager,
                         void *buffer,
                         marcel_bubble_t *owner) {
  marcel_entity_t *entity;
  entity = ma_entity_bubble(owner);
  return _mami_entity_unattach(memory_manager, buffer, entity);
}

int mami_bubble_unattach_all(mami_manager_t *memory_manager,
                             marcel_bubble_t *owner) {
  marcel_entity_t *entity;
  entity = ma_entity_bubble(owner);
  return _mami_entity_unattach_all(memory_manager, entity);
}

int mami_bubble_migrate_all(mami_manager_t *memory_manager,
                            marcel_bubble_t *owner,
                            int node) {
  marcel_entity_t *entity;
  entity = ma_entity_bubble(owner);
  return _mami_entity_migrate_all(memory_manager, entity, node);
}

#endif /* MM_MAMI_ENABLED */
