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

#ifdef MARCEL

#include <errno.h>
#include "mm_mami.h"
#include "mm_mami_private.h"
#include "mm_mami_marcel_private.h"
#include "mm_debug.h"
#include "mm_helper.h"

int _mami_entity_attach(mami_manager_t *memory_manager,
                        void *buffer,
                        size_t size,
                        mami_entity_t entity,
                        int *node)
{
        int err=0;
        mami_data_t *data;

        MEMORY_ILOG_IN();

        mdebug_memory("Attaching [%p:%p:%ld] to entity %p\n", buffer, buffer+size, (long)size, MAMI_ENTITY_GET_OWNER(entity));

        if (!buffer) {
                mdebug_memory("Cannot attach NULL buffer\n");
                errno = EINVAL;
                err = -1;
        }
        else {
                void *aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normal_page_size);
                void *aligned_endbuffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer+size, memory_manager->normal_page_size);
                size_t aligned_size = aligned_endbuffer-aligned_buffer;
                mami_data_link_t *area;

                if (!aligned_size) {
                        aligned_endbuffer = aligned_buffer + memory_manager->normal_page_size;
                        aligned_size = memory_manager->normal_page_size;
                }

                err = _mami_locate(memory_manager, memory_manager->root, aligned_buffer, 1, &data);
                if (err < 0) {
                        mdebug_memory("The address interval [%p:%p] is not managed by MaMI. Let's register it\n", aligned_buffer, aligned_endbuffer);
                        _mami_register(memory_manager, aligned_buffer, aligned_endbuffer, aligned_size, buffer, size, 0, &data);
                        err = 0;
                }
                else {
                        if (data->node == MAMI_FIRST_TOUCH_NODE || data->node < 0) {
                                mdebug_memory("Need to find out the location of the memory area\n");
                                if (data->nodes == NULL) data->nodes = malloc(data->nb_pages * sizeof(int));
                                _mami_get_pages_location(memory_manager, (const void**)data->pageaddrs, data->nb_pages, &(data->node), data->nodes);
                        }

                        if (size < data->size && aligned_size != data->size - memory_manager->normal_page_size) {
                                size_t newsize;
                                mami_data_t *next_data;

                                newsize = data->size-aligned_size;
                                if (!newsize) {
                                        mdebug_memory("Cannot split a page\n");
                                }
                                else {
                                        mdebug_memory("Splitting [%p:%p] in [%p:%p] and [%p:%p]\n", data->start_address,data->end_address,
                                                      data->start_address,data->start_address+aligned_size, aligned_endbuffer,aligned_endbuffer+newsize);
                                        data->nb_pages = aligned_size/memory_manager->normal_page_size;
                                        data->end_address = data->start_address + aligned_size;
                                        data->size = aligned_size;
                                        _mami_register(memory_manager, aligned_endbuffer, aligned_endbuffer+newsize, newsize, aligned_endbuffer, newsize,
                                                       data->mami_allocated, &next_data);
                                        data->next = next_data;
                                        mami_update_pages_location(memory_manager, data->start_address, data->size);
                                }
                        }
                }

                mdebug_memory("Adding entity %p to data %p\n", MAMI_ENTITY_GET_OWNER(entity), data);
                mami_entity_list_push_back(data->owners, entity);

                *node = data->node;
                _mami_update_stats_for_entity(memory_manager, data, entity, +1);

                area = tmalloc(sizeof(mami_data_link_t));
                area->data = data;
                TBX_INIT_FAST_LIST_HEAD(&(area->list));
                if (MAMI_ENTITY_IS_TASK(entity)) {
                        marcel_spin_lock(&(MARCEL_GET_TASK_MEMORY_AREAS_LOCK(entity->owner.task)));
                        tbx_fast_list_add(&(area->list), &(MARCEL_GET_TASK_MEMORY_AREAS(entity->owner.task)));
                        marcel_spin_unlock(&(MARCEL_GET_TASK_MEMORY_AREAS_LOCK(entity->owner.task)));
                }
#ifdef MA__BUBBLES
                else {
                        marcel_spin_lock(&(MARCEL_GET_BUBBLE_MEMORY_AREAS_LOCK(entity->owner.bubble)));
                        tbx_fast_list_add(&(area->list), &(MARCEL_GET_BUBBLE_MEMORY_AREAS(entity->owner.bubble)));
                        marcel_spin_unlock(&(MARCEL_GET_BUBBLE_MEMORY_AREAS_LOCK(entity->owner.bubble)));
                }
#endif
        }
        MEMORY_ILOG_OUT();
        return err;
}

static
int _mami_entity_unattach(mami_manager_t *memory_manager,
                          void *buffer,
                          mami_entity_t entity)
{
        int err=0;
        mami_data_t *data = NULL;
        void *aligned_buffer;
        mami_data_link_t *area = NULL;

        MEMORY_ILOG_IN();
        marcel_mutex_lock(&(memory_manager->lock));
        aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normal_page_size);

        err = _mami_locate(memory_manager, memory_manager->root, aligned_buffer, 1, &data);
        if (err >= 0) {
                mdebug_memory("Removing entity %p from data %p\n", MAMI_ENTITY_GET_OWNER(entity), data);

                if (mami_entity_list_empty(data->owners)) {
                        mdebug_memory("The entity %p is not attached to the memory area %p(1).\n", MAMI_ENTITY_GET_OWNER(entity), aligned_buffer);
                        errno = ENOENT;
                        err = -1;
                }
                else {
                        mami_entity_itor_t iter;
                        int found=0;
                        for(iter=mami_entity_list_begin(data->owners); iter!=mami_entity_list_end(data->owners); iter=mami_entity_list_next(iter)) {
                                if (iter->type == entity->type && MAMI_ENTITY_GET_OWNER(entity) == MAMI_ENTITY_GET_OWNER(iter)) {
                                        found=1;
                                        _mami_update_stats_for_entity(memory_manager, data, entity, -1);
                                        mdebug_memory("Removing data %p from entity %p\n", data, MAMI_ENTITY_GET_OWNER(entity));
                                        if (MAMI_ENTITY_IS_TASK(entity)) {
                                                marcel_spin_lock(&(MARCEL_GET_TASK_MEMORY_AREAS_LOCK(entity->owner.task)));
                                                tbx_fast_list_for_each_entry(area, &(MARCEL_GET_TASK_MEMORY_AREAS(entity->owner.task)), list) {
                                                        if (area->data == data) {
                                                                tbx_fast_list_del_init(&area->list);
                                                                tfree(area);
                                                                break;
                                                        }
                                                }
                                                marcel_spin_unlock(&(MARCEL_GET_TASK_MEMORY_AREAS_LOCK(entity->owner.task)));
                                        }
#ifdef MA__BUBBLES
                                        else {
                                                marcel_spin_lock(&(MARCEL_GET_BUBBLE_MEMORY_AREAS_LOCK(entity->owner.bubble)));
                                                tbx_fast_list_for_each_entry(area, &(MARCEL_GET_BUBBLE_MEMORY_AREAS(entity->owner.bubble)), list) {
                                                        if (area->data == data) {
                                                                tbx_fast_list_del_init(&area->list);
                                                                tfree(area);
                                                                break;
                                                        }
                                                }
                                                marcel_spin_unlock(&(MARCEL_GET_BUBBLE_MEMORY_AREAS_LOCK(entity->owner.bubble)));
                                        }
#endif /* MA__BUBBLES */
                                }
                                break;
                        }
                        if (found) {
                                mami_entity_list_erase(data->owners, entity);
                        }
                        else {
                                mdebug_memory("The entity %p is not attached to the memory area %p.\n", MAMI_ENTITY_GET_OWNER(entity), aligned_buffer);
                                errno = ENOENT;
                                err = -1;
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
                              mami_entity_t entity)
{
        mami_data_link_t *area, *narea;

        MEMORY_ILOG_IN();
        mdebug_memory("Unattaching all memory areas from entity %p\n", MAMI_ENTITY_GET_OWNER(entity));
        if (MAMI_ENTITY_IS_TASK(entity)) {
                //marcel_spin_lock(&(MARCEL_GET_TASK_MEMORY_AREAS_LOCK(entity->owner.task)));
                tbx_fast_list_for_each_entry_safe(area, narea, &(MARCEL_GET_TASK_MEMORY_AREAS(entity->owner.task)), list) {
                        _mami_entity_unattach(memory_manager, area->data->start_address, entity);
                }
                //marcel_spin_unlock(&(MARCEL_GET_TASK_MEMORY_AREAS_LOCK(entity->owner.task)));
        }
#ifdef MA__BUBBLES
        else {
                //marcel_spin_lock(&(MARCEL_GET_BUBBLE_MEMORY_AREAS_LOCK(entity->owner.bubble)));
                tbx_fast_list_for_each_entry_safe(area, narea, &(MARCEL_GET_BUBBLE_MEMORY_AREAS(entity->owner.bubble)), list) {
                        _mami_entity_unattach(memory_manager, area->data->start_address, entity);
                }
                //marcel_spin_unlock(&(MARCEL_GET_BUBBLE_MEMORY_AREAS_LOCK(entity->owner.bubble)));
        }
#endif /* MA__BUBBLES */

        MEMORY_ILOG_OUT();
        return 0;
}

static
int _mami_entity_migrate_all(mami_manager_t *memory_manager,
                             mami_entity_t entity,
                             int node)
{
        mami_data_link_t *area, *narea;
        if (MAMI_ENTITY_IS_TASK(entity)) {
                tbx_fast_list_for_each_entry_safe(area, narea, &(MARCEL_GET_TASK_MEMORY_AREAS(entity->owner.task)), list) {
                        _mami_migrate_on_node(memory_manager, area->data, node);
                }
        }
#ifdef MA__BUBBLES
        else {
                tbx_fast_list_for_each_entry_safe(area, narea, &(MARCEL_GET_BUBBLE_MEMORY_AREAS(entity->owner.bubble)), list) {
                        _mami_migrate_on_node(memory_manager, area->data, node);
                }
        }
#endif /* MA__BUBBLES */
        return 0;
}

int mami_task_attach(mami_manager_t *memory_manager,
                     void *buffer,
                     size_t size,
                     marcel_t owner,
                     int *node)
{
        mami_entity_t entity;
        int ret;

        MEMORY_LOG_IN();
        entity = mami_entity_new();
        MAMI_ENTITY_SET_TASK_OWNER(entity, owner);
        marcel_mutex_lock(&(memory_manager->lock));
        ret = _mami_entity_attach(memory_manager, buffer, size, entity, node);
        marcel_mutex_unlock(&(memory_manager->lock));
        MEMORY_LOG_OUT();
        return ret;
}

int mami_task_unattach(mami_manager_t *memory_manager,
                       void *buffer,
                       marcel_t owner)
{
        mami_entity_t entity;
        int ret;

        MEMORY_LOG_IN();
        entity = mami_entity_new();
        MAMI_ENTITY_SET_TASK_OWNER(entity, owner);
        ret = _mami_entity_unattach(memory_manager, buffer, entity);
        MEMORY_LOG_OUT();
        return ret;
}

int mami_task_unattach_all(mami_manager_t *memory_manager,
                           marcel_t owner)
{
        mami_entity_t entity;
        int ret;

        MEMORY_LOG_IN();
        entity = mami_entity_new();
        MAMI_ENTITY_SET_TASK_OWNER(entity, owner);
        ret = _mami_entity_unattach_all(memory_manager, entity);
        MEMORY_LOG_OUT();
        return ret;
}

int mami_task_migrate_all(mami_manager_t *memory_manager,
                          marcel_t owner,
                          int node)
{
        mami_entity_t entity;

        entity = mami_entity_new();
        MAMI_ENTITY_SET_TASK_OWNER(entity, owner);
        return _mami_entity_migrate_all(memory_manager, entity, node);
}

int mami_task_migrate_all_callback(void *memory_manager,
                                   marcel_t owner,
                                   int node)
{
        return mami_task_migrate_all((mami_manager_t *)memory_manager, owner, node);
}

#ifdef MA__BUBBLES
int mami_bubble_attach(mami_manager_t *memory_manager,
                       void *buffer,
                       size_t size,
                       marcel_bubble_t *owner,
                       int *node)
{
        int ret;
        mami_entity_t entity;

        entity = mami_entity_new();
        MAMI_ENTITY_SET_BUBBLE_OWNER(entity, owner);
        marcel_mutex_lock(&(memory_manager->lock));
        ret = _mami_entity_attach(memory_manager, buffer, size, entity, node);
        marcel_mutex_unlock(&(memory_manager->lock));
        return ret;
}

int mami_bubble_unattach(mami_manager_t *memory_manager,
                         void *buffer,
                         marcel_bubble_t *owner)
{
        mami_entity_t entity;

        entity = mami_entity_new();
        MAMI_ENTITY_SET_BUBBLE_OWNER(entity, owner);
        return _mami_entity_unattach(memory_manager, buffer, entity);
}

int mami_bubble_unattach_all(mami_manager_t *memory_manager,
                             marcel_bubble_t *owner)
{
        mami_entity_t entity;

        entity = mami_entity_new();
        MAMI_ENTITY_SET_BUBBLE_OWNER(entity, owner);
        return _mami_entity_unattach_all(memory_manager, entity);
}

int mami_bubble_migrate_all(mami_manager_t *memory_manager,
                            marcel_bubble_t *owner,
                            int node)
{
        mami_entity_t entity;

        entity = mami_entity_new();
        MAMI_ENTITY_SET_BUBBLE_OWNER(entity, owner);
        return _mami_entity_migrate_all(memory_manager, entity, node);
}

int mami_bubble_migrate_all_callback(void *memory_manager,
                                     marcel_bubble_t *owner,
                                     int node)
{
        return mami_bubble_migrate_all((mami_manager_t *)memory_manager, owner, node);
}

#endif /* MA__BUBBLES */

int _mami_update_stats_for_entities(mami_manager_t *memory_manager,
                                    mami_data_t *data,
                                    int coeff)
{
        mami_entity_itor_t iter;
        for(iter=mami_entity_list_begin(data->owners); iter!=mami_entity_list_end(data->owners); iter=mami_entity_list_next(iter)) {
                _mami_update_stats_for_entity(memory_manager, data, iter, coeff);
        }
        return 0;
}

int _mami_update_stats_for_entity(mami_manager_t *memory_manager,
                                  mami_data_t *data,
                                  mami_entity_t entity,
                                  int coeff)
{
        if (data->node >= 0 && data->node != MAMI_FIRST_TOUCH_NODE) {
                mdebug_memory("%s %lu bits to memnode offset for node #%d\n", coeff>0?"Adding":"Removing", (long unsigned)data->size, data->node);
                if (MAMI_ENTITY_IS_TASK(entity))
                        ((long *) marcel_task_stats_get (entity->owner.task, MEMNODE))[data->node] += (coeff * data->size);
#ifdef MA__BUBBLES
                else
                        ((long *) marcel_bubble_stats_get (entity->owner.bubble, MEMNODE))[data->node] += (coeff * data->size);
#endif
        }
        else if (data->node == MAMI_MULTIPLE_LOCATION_NODE) {
                int i;
                mdebug_memory("Memory scattered on different nodes. Update statistics accordingly\n");
                mdebug_memory("nb of pages on undefined node = %ld\n", data->pages_on_undefined_node);
                for(i=0 ; i<memory_manager->nb_nodes ; i++) {
                        if (data->pages_per_node[i]) {
                                size_t nsize = data->pages_per_node[i] * memory_manager->normal_page_size;
                                mdebug_memory("%s %lu bits (%ld pages) to memnode offset for node #%d\n", coeff>0?"Adding":"Removing",
                                              (long unsigned)nsize, data->pages_per_node[i], i);
                                if (MAMI_ENTITY_IS_TASK(entity))
                                        ((long *) marcel_task_stats_get(entity->owner.task, MEMNODE))[i] += (coeff * nsize);
#ifdef MA__BUBBLES
                                else
                                        ((long *) marcel_bubble_stats_get(entity->owner.bubble, MEMNODE))[i] += (coeff * nsize);
#endif
                        }
                }
        }
        return 0;
}

#endif /* MARCEL */
#endif /* MM_MAMI_ENABLED */
