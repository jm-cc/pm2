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

extern debug_type_t debug_memory;
extern debug_type_t debug_memory_log;
extern debug_type_t debug_memory_ilog;
extern debug_type_t debug_memory_warn;

#define mdebug_mami(fmt, args...) \
    debug_printf(&debug_memory, "[%s] " fmt , __TBX_FUNCTION__, ##args)

#if defined(PM2DEBUG)
#  define MAMI_LOG_IN()        debug_printf(&debug_memory_log, "%s: -->\n", __TBX_FUNCTION__)
#  define MAMI_LOG_OUT()       debug_printf(&debug_memory_log, "%s: <--\n", __TBX_FUNCTION__)
#  define MAMI_ILOG_IN()       debug_printf(&debug_memory_ilog, "%s: -->\n", __TBX_FUNCTION__)
#  define MAMI_ILOG_OUT()      debug_printf(&debug_memory_ilog, "%s: <--\n", __TBX_FUNCTION__)
#else
#  define MAMI_LOG_IN()
#  define MAMI_LOG_OUT()
#  define MAMI_ILOG_IN()
#  define MAMI_ILOG_OUT()
#endif

extern
void ma_memory_delete_tree(marcel_memory_manager_t *memory_manager,
			   marcel_memory_tree_t **memory_tree);

extern
void ma_memory_register(marcel_memory_manager_t *memory_manager,
                        void *buffer,
                        size_t size,
                        int mami_allocated,
                        marcel_memory_data_t **data);

extern
void ma_memory_unregister(marcel_memory_manager_t *memory_manager,
			  marcel_memory_tree_t **memory_tree,
			  void *buffer);

/*
 * Preallocates some memory (in number of pages) on the specified numa node.
 */
extern
int ma_memory_preallocate(marcel_memory_manager_t *memory_manager,
                          marcel_memory_area_t **space,
                          int nbpages,
                          int vnode,
                          int pnode);

extern
int ma_memory_preallocate_huge_pages(marcel_memory_manager_t *memory_manager,
                                     marcel_memory_huge_pages_area_t **space,
                                     int node);

extern
int ma_memory_check_pages_location(void **pageaddrs,
                                   int pages,
                                   int node);

extern
int ma_memory_move_pages(void **pageaddrs,
                         int pages,
                         int *nodes,
                         int *status,
                         int flag);

extern
int ma_memory_mbind(void *start, unsigned long len, int mode,
                    const unsigned long *nmask, unsigned long maxnode, unsigned flags);

extern
int ma_memory_set_mempolicy(int mode, const unsigned long *nmask, unsigned long maxnode);

extern
int ma_memory_load_model_for_memory_migration(marcel_memory_manager_t *memory_manager);

extern
int ma_memory_load_model_for_memory_access(marcel_memory_manager_t *memory_manager);

#endif /* MM_MAMI_PRIVATE_H */
#endif /* MM_MAMI_ENABLED */
