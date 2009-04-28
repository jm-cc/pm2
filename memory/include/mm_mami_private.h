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

#ifndef MM_MAMI_PRIVATE_H
#define MM_MAMI_PRIVATE_H

/* align a application-given address to the closest page-boundary:
 * re-add the lower bits to increase the bit above pagesize if needed, and truncate
 */
#define _MAMI_ALIGN_ON_PAGE(address, pagesize) (void*)((((uintptr_t) address) + (pagesize >> 1)) & (~(pagesize-1)))
#define MAMI_ALIGN_ON_PAGE(memory_manager, address, pagesize) (memory_manager->alignment?_MAMI_ALIGN_ON_PAGE(address, pagesize):address)

/* Node id used for first touch allocated memory */
#define MAMI_FIRST_TOUCH_NODE memory_manager->nb_nodes
/* Node id used when memory area located on several nodes */
#define MAMI_MULTIPLE_LOCATION_NODE -2
/* Node id used when unknown location */
#define MAMI_UNKNOWN_LOCATION_NODE -1

extern
void _mami_delete_tree(mami_manager_t *memory_manager,
                       mami_tree_t **memory_tree);

extern
void _mami_register(mami_manager_t *memory_manager,
                    void *buffer,
                    size_t size,
                    int mami_allocated,
                    mami_data_t **data);

extern
void _mami_unregister(mami_manager_t *memory_manager,
                      mami_tree_t **memory_tree,
                      void *buffer);

/*
 * Preallocates some memory (in number of pages) on the specified numa node.
 */
extern
int _mami_preallocate(mami_manager_t *memory_manager,
                      mami_area_t **space,
                      int nbpages,
                      int vnode,
                      int pnode);

extern
int _mami_preallocate_huge_pages(mami_manager_t *memory_manager,
                                 mami_huge_pages_area_t **space,
                                 int node);

extern
int _mami_check_pages_location(void **pageaddrs,
                               int pages,
                               int node);

extern
int _mami_set_mempolicy(int mode, const unsigned long *nmask, unsigned long maxnode);

extern
int _mami_load_model_for_memory_migration(mami_manager_t *memory_manager);

extern
int _mami_load_model_for_memory_access(mami_manager_t *memory_manager);

extern
int _mami_locate(mami_manager_t *memory_manager,
                 mami_tree_t *memory_tree,
                 void *buffer,
                 size_t size,
                 mami_data_t **data);

extern
int _mami_get_pages_location(mami_manager_t *memory_manager,
                             void **pageaddrs,
                             int nbpages,
                             int *node,
                             int **nodes);

extern
int _mami_migrate_pages(mami_manager_t *memory_manager,
                        mami_data_t *data,
                        int dest);

#endif /* MM_MAMI_PRIVATE_H */
#endif /* MM_MAMI_ENABLED */
