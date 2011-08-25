/* MaMI --- NUMA Memory Interface
 *
 * Copyright (C) 2008, 2009, 2010, 2011  Centre National de la Recherche Scientifique
 *
 * MaMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * MaMI is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU Lesser General Public License in COPYING.LGPL for more details.
 */
#ifdef MAMI_ENABLED

#ifndef MAMI_MARCEL_H
#define MAMI_MARCEL_H

#include <marcel.h>

/** \addtogroup mami
 * @{ */

/** \addtogroup mami_marcel MaMI Marcel Specific Interface
 * The following functionalities are only available when MaMI is build
 * on top of Marcel.
 * @{ */

/**
 * Attaches the memory to the specified thread. If the memory is not
 * known by MaMI, it will be registered.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param size size of the memory area
 * @param owner thread
 * @param[out] node will contain the node id where the data is located
 * @return 0 on success, -1 and sets errno to EINVAL when the NULL address is provided
 */
extern
int mami_task_attach(mami_manager_t *memory_manager,
                     void *buffer,
                     size_t size,
                     marcel_t owner,
                     int *node);

/**
 * Unattaches the memory from the specified thread
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param owner thread
 * @return same code as mami_locate() when address not known by MaMI,
 * -1 and sets errno to ENOENT when entity not attached to the
 * memory, 0 otherwise
 */
extern
int mami_task_unattach(mami_manager_t *memory_manager,
                       void *buffer,
                       marcel_t owner);

/**
 * Unattaches all the memory areas attached to the specified thread.
 * @param memory_manager pointer to the memory manager
 * @param owner thread
 * @return same code as mami_task_unattach()
 */
extern
int mami_task_unattach_all(mami_manager_t *memory_manager,
                           marcel_t owner);

/**
 * Migrates all the memory areas attached to the specified thread to the given node.
 * @param memory_manager pointer to the memory manager
 * @param owner thread
 * @param node destination
 * @return 0 on success, -1 and sets errno to EALREADY when pages
 * already located on the given node
 */
extern
int mami_task_migrate_all(mami_manager_t *memory_manager,
                          marcel_t owner,
                          int node);

extern
int mami_task_migrate_all_callback(void *memory_manager,
                                   marcel_t owner,
                                   int node);

#ifdef MARCEL_BUBBLES
/**
 * Attaches the memory to the specified bubble. If the memory is not
 * known by MaMI, it will be registered.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param size size of the memory area
 * @param owner bubble
 * @param[out] node will contain the node id where the data is located
 * @return 0 on success, -1 and sets errno to EINVAL when the NULL address is provided
 */
extern
int mami_bubble_attach(mami_manager_t *memory_manager,
                       void *buffer,
                       size_t size,
                       marcel_bubble_t *owner,
                       int *node);

/**
 * Unattaches the memory from the specified bubble
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param owner bubble
 * @return same code as mami_locate() when address not known by MaMI,
 * -1 and sets errno to ENOENT when entity not attached to the
 * memory, 0 otherwise
 */
extern
int mami_bubble_unattach(mami_manager_t *memory_manager,
                         void *buffer,
                         marcel_bubble_t *owner);

/**
 * Unattaches all the memory areas attached to the specified bubble.
 * @param memory_manager pointer to the memory manager
 * @param owner bubble
 * @return same code as mami_bubble_unattach()
 */
extern
int mami_bubble_unattach_all(mami_manager_t *memory_manager,
                             marcel_bubble_t *owner);

/**
 * Migrates to the given node all the memory areas attached to the specified bubble
 * @param memory_manager pointer to the memory manager
 * @param owner bubble
 * @param node destination
 * @return 0 on success, -1 and sets errno to EALREADY when pages
 * already located on the given node
 */
extern
int mami_bubble_migrate_all(mami_manager_t *memory_manager,
                            marcel_bubble_t *owner,
                            int node);

extern
int mami_bubble_migrate_all_callback(void *memory_manager,
                                     marcel_bubble_t *owner,
                                     int node);
#endif /* MARCEL_BUBBLES */

/**
 * Marks the area to be attached on next touch.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @return 0 on success, -1 and sets errno to EINVAL when address not
 * known by MaMI or to other specific error values when setting the
 * next touch policy
 */
extern
int mami_attach_on_next_touch(mami_manager_t *memory_manager,
			      void *buffer);

/* @} */
/* @} */

#endif /* MAMI_MARCEL_H */
#endif /* MAMI_ENABLED */
