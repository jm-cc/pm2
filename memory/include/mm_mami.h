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

/*! \mainpage The Memory API Documentation
 *
 * \section intro_sec Introduction
 * Two memory management interfaces are provided:
 * - \ref mami
 * - \ref memory_heap
 */

/** \file
 * \brief MaMI: Marcel Memory Interface
 */

/** \defgroup mami MaMI: Marcel Memory Interface
 *
 * MaMI is a memory interface implemented within our user-level thread
 * library, Marcel. It allows developers to manage memory with regard
 * to NUMA nodes thanks to an automatically gathered knowledge of the
 * underlying architecture. The initialization stage preallocates
 * memory heaps on all the NUMA nodes of the target machine, and
 * user-made allocations then pick up memory areas from the
 * preallocated heaps.
 *
 * MaMI implements two methods to migrate data. The first method is
 * based on the Next-touch policy, it is implemented as a user-level
 * pager (mprotect() and signal handler for SIGSEGV). The second
 * migration method is synchronous and allows to move data on a given
 * node. Both move pages using the Linux system call move_pages().
 *
 * It is possible to evaluate reading and writing access costs to
 * remote memory areas. Migration cost is based on a linear function
 * on the number of pages being migrated.
 *
 * Moreover, MaMI gathers statistics on how much memory is available
 * and left on the different nodes. This information is potentially
 * helpful when deciding whether or not to migrate a memory area. This
 * is the interface for memory management.
 *
 * More information about MaMI can be found at
 * found at http://runtime.bordeaux.inria.fr/MaMI/.
 *
 * @{
 */

#ifdef MM_MAMI_ENABLED

#ifndef MM_MAMI_H
#define MM_MAMI_H

#include "pm2_common.h"

/** \addtogroup mami_init
 * @{ */
/** \brief Type of a memory manager */
typedef struct mami_manager_s  mami_manager_t;
/* @} */

/** \addtogroup mami_alloc
 * @{ */
/** \brief Type of a memory location policy */
typedef int mami_membind_policy_t;
/** \brief The memory will be allocated with the policy specified by the last call to mami_membind() */
#define MAMI_MEMBIND_POLICY_DEFAULT           ((mami_membind_policy_t)0)
/** \brief The memory will be allocated on the current node */
#define MAMI_MEMBIND_POLICY_NONE              ((mami_membind_policy_t)2)
/** \brief The memory will be allocated on the given node */
#define MAMI_MEMBIND_POLICY_SPECIFIC_NODE     ((mami_membind_policy_t)4)
/** \brief The memory will be allocated on the least loaded node */
#define MAMI_MEMBIND_POLICY_LEAST_LOADED_NODE ((mami_membind_policy_t)8)
/** \brief The memory will be allocated via huge pages */
#define MAMI_MEMBIND_POLICY_HUGE_PAGES        ((mami_membind_policy_t)16)
/** \brief The memory will not be bound to any node */
#define MAMI_MEMBIND_POLICY_FIRST_TOUCH       ((mami_membind_policy_t)32)
/* @} */

/** \addtogroup mami_stats
 * @{ */
/** \brief Node selection policy */
typedef int mami_node_selection_policy_t;
/** \brief Selects the least loaded node */
#define MAMI_LEAST_LOADED_NODE      ((mami_node_selection_policy_t)0)

/** \brief Type of a statistic */
typedef int mami_stats_t;
/** \brief The total amount of memory */
#define MAMI_STAT_MEMORY_TOTAL      ((mami_stats_t)0)
/** \brief The free amount of memory */
#define MAMI_STAT_MEMORY_FREE       ((mami_stats_t)1)
/* @} */

/** \addtogroup mami_init MaMI Init
 * This is the interface for MaMI Init
 * @{ */

/**
 * Initialises the memory manager.
 * @param memory_manager pointer to the memory manager
 */
extern
void mami_init(mami_manager_t **memory_manager);

/**
 * Shutdowns the memory manager.
 * @param memory_manager pointer to the memory manager
 */
extern
void mami_exit(mami_manager_t **memory_manager);

/**
 * Indicates that memory addresses given to MaMI should be aligned to page boundaries.
 * @param memory_manager pointer to the memory manager
 */
extern
int mami_set_alignment(mami_manager_t *memory_manager);

/**
 * Indicates that memory addresses given to MaMI should NOT be aligned to page boundaries.
 * @param memory_manager pointer to the memory manager
 */
extern
int mami_unset_alignment(mami_manager_t *memory_manager);

/**
 * Locates the given memory.
 * @param memory_manager pointer to the memory manager
 * @param buffer pointer to the memory to be located
 * @param size size of the memory area to be located
 * @param node returns the location of the given memory
 * @return a negative value and set errno to EINVAL when address not found
 */
extern
int mami_locate(mami_manager_t *memory_manager,
		void *buffer,
		size_t size,
		int *node);

/**
 * Prints on the standard output the currently managed memory areas.
 * @param memory_manager pointer to the memory manager
 */
extern
void mami_print(mami_manager_t *memory_manager);

/**
 * Prints in the given file the currently managed memory areas.
 * @param memory_manager pointer to the memory manager
 * @param stream
 */
extern
void mami_fprint(mami_manager_t *memory_manager, FILE *stream);

/**
 * Indicates if huge pages are available on the system.
 * @param memory_manager pointer to the memory manager
 */
extern
int mami_huge_pages_available(mami_manager_t *memory_manager);

/* @} */

/** \addtogroup mami_alloc MaMI Allocation
 * This is the interface for MaMI Allocation
 * @{
 */

/**
 * Allocates memory w.r.t the specified allocation policy. Size will be rounded up to the system page size.
 * @param memory_manager pointer to the memory manager
 * @param size size of the required memory
 * @param policy allocation policy
 * @param node allocation node
 */
extern
void* mami_malloc(mami_manager_t *memory_manager,
                  size_t size,
                  mami_membind_policy_t policy,
                  int node);

/**
 * Allocates memory w.r.t the specified allocation policy. Size will be rounded up to the system page size.
 * @param memory_manager pointer to the memory manager
 * @param nmemb number of elements to allocate
 * @param size size of a single element
 * @param policy memory allocation policy
 * @param node allocation node
 */
extern
void* mami_calloc(mami_manager_t *memory_manager,
                  size_t nmemb,
                  size_t size,
                  mami_membind_policy_t policy,
                  int node);

/**
 * Registers a memory area which has not been allocated by MaMI.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param size size of the memory area
 */
extern
int mami_register(mami_manager_t *memory_manager,
                  void *buffer,
                  size_t size);

/**
 * Unregisters a memory area which has not been allocated by MaMI.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 */
extern
int mami_unregister(mami_manager_t *memory_manager,
                    void *buffer);

/**
 * Splits a memory area into subareas.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param subareas number of subareas
 * @param newbuffers addresses of the new memory areas
 */
extern
int mami_split(mami_manager_t *memory_manager,
               void *buffer,
               unsigned int subareas,
               void **newbuffers);

/**
 * Sets the default allocation policy.
 * @param memory_manager pointer to the memory manager
 * @param policy new default allocation policy
 * @param node new default allocation node
 */
extern
int mami_membind(mami_manager_t *memory_manager,
                 mami_membind_policy_t policy,
                 int node);

/**
 * Frees the given memory.
 * @param memory_manager pointer to the memory manager
 * @param buffer pointer to the memory
 */
extern
void mami_free(mami_manager_t *memory_manager,
               void *buffer);

/**
 * Checks if the location of the memory area is the given node.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param size size of the memory area
 * @param node location to be checked
 */
extern
int mami_check_pages_location(mami_manager_t *memory_manager,
                              void *buffer,
                              size_t size,
                              int node);

/**
 * Updates the location of the memory area.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param size size of the memory area
 */
extern
int mami_update_pages_location(mami_manager_t *memory_manager,
                               void *buffer,
                               size_t size);

/* @} */

/** \addtogroup mami_stats MaMI Statistics
 * This is the interface for MaMI Statistics
 * @{
 */

/**
 * Indicates the value of the statistic \e stat for the node \e node
 * @param memory_manager pointer to the memory manager
 * @param node node identifier
 * @param stat statistic
 * @param value will contain the value of the given statistic for the given node
 */
extern
int mami_stats(mami_manager_t *memory_manager,
               int node,
               mami_stats_t stat,
               unsigned long *value);

/**
 * Selects the "best" node based on the given policy.
 * @param memory_manager pointer to the memory manager
 * @param policy selection policy
 * @param node returns the id of the node
 */
extern
int mami_select_node(mami_manager_t *memory_manager,
                     mami_node_selection_policy_t policy,
                     int *node);


/* @} */

/** \addtogroup mami_cost MaMI Cost
 * This is the interface for MaMI Cost
 * @{
 */

/**
 * Indicates the migration cost for \e size bits from node \e source to node \e dest.
 * @param memory_manager pointer to the memory manager
 * @param source source node
 * @param dest destination node
 * @param size how many bits do we want to migrate
 * @param cost estimated cost of the migration
 */
extern
int mami_migration_cost(mami_manager_t *memory_manager,
                        int source,
                        int dest,
                        size_t size,
                        float *cost);

/**
 * Indicates the cost for write accessing \e size bits from node \e source to node \e dest.
 * @param memory_manager pointer to the memory manager
 * @param source source node
 * @param dest destination node
 * @param size how many bits do we want to access
 * @param cost estimated cost of the access
 */
extern
int mami_cost_for_write_access(mami_manager_t *memory_manager,
                               int source,
                               int dest,
                               size_t size,
                               float *cost);

/**
 * Indicates the cost for read accessing \e size bits from node \e source to node \e dest.
 * @param memory_manager pointer to the memory manager
 * @param source source node
 * @param dest destination node
 * @param size how many bits do we want to access
 * @param cost estimated cost of the access
 */
extern
int mami_cost_for_read_access(mami_manager_t *memory_manager,
                              int source,
                              int dest,
                              size_t size,
                              float *cost);

/**
 * Performs the sampling for the memory migration between the specified nodes.
 * @param memory_manager pointer to the memory manager
 * @param minsource
 * @param maxsource
 * @param mindest
 * @param maxdest
 * @param extended_mode
 */
extern
int mami_sampling_of_memory_migration(mami_manager_t *memory_manager,
                                      unsigned long minsource,
                                      unsigned long maxsource,
                                      unsigned long mindest,
                                      unsigned long maxdest,
                                      int extended_mode);

/**
 * Performs the sampling for the memory access between the specified nodes.
 * @param memory_manager pointer to the memory manager
 * @param minsource
 * @param maxsource
 * @param mindest
 * @param maxdest
 */
extern
int mami_sampling_of_memory_access(mami_manager_t *memory_manager,
                                   unsigned long minsource,
                                   unsigned long maxsource,
                                   unsigned long mindest,
                                   unsigned long maxdest);

/* @} */

/** \addtogroup mami_migration MaMI Migration
 * This is the interface for MaMI Migration
 * @{
 */

/**
 * Migrates the pages to the specified node.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the buffer to be migrated
 * @param dest identifier of the destination node
 */
extern
int mami_migrate_pages(mami_manager_t *memory_manager,
                       void *buffer,
                       int dest);

/**
 * Marks the area to be migrated on next touch.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 */
extern
int mami_migrate_on_next_touch(mami_manager_t *memory_manager,
                               void *buffer);

/**
 * Migrates the area to the specified node.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param node destination
 */
extern
int mami_migrate_on_node(mami_manager_t *memory_manager,
                         void *buffer,
                         int node);

/**
 * Attaches the memory to the specified thread
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param size size of the memory area
 * @param owner thread
 * @param node will contain the node id where the data is located
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
 */
extern
int mami_task_unattach(mami_manager_t *memory_manager,
                       void *buffer,
                       marcel_t owner);

/**
 * Unattaches all the memory areas attached to the specified thread.
 * @param memory_manager pointer to the memory manager
 * @param owner thread
 */
extern
int mami_task_unattach_all(mami_manager_t *memory_manager,
                           marcel_t owner);

/**
 * Migrates all the memory areas attached to the specified thread to the given node.
 * @param memory_manager pointer to the memory manager
 * @param owner thread
 * @param node destination
 */
extern
int mami_task_migrate_all(mami_manager_t *memory_manager,
                          marcel_t owner,
                          int node);

/**
 * Attaches the memory to the specified bubble
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param size size of the memory area
 * @param owner bubble
 * @param node will contain the node id where the data is located
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
 */
extern
int mami_bubble_unattach(mami_manager_t *memory_manager,
                         void *buffer,
                         marcel_bubble_t *owner);

/**
 * Unattaches all the memory areas attached to the specified bubble.
 * @param memory_manager pointer to the memory manager
 * @param owner bubble
 */
extern
int mami_bubble_unattach_all(mami_manager_t *memory_manager,
                             marcel_bubble_t *owner);

/**
 * Migrates to the given node all the memory areas attached to the specified bubble
 * @param memory_manager pointer to the memory manager
 * @param owner bubble
 * @param node destination
 */
extern
int mami_bubble_migrate_all(mami_manager_t *memory_manager,
                            marcel_bubble_t *owner,
                            int node);

/**
 * Distributes pages of the given memory on the given set of nodes
 * using a round robin policy. The first page is migrated to the first
 * node, the second page to the second node, ....
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param nodes destination nodes
 * @param nb_nodes number of nodes in the nodes array
 */
extern
int mami_distribute(mami_manager_t *memory_manager,
                    void *buffer,
                    int *nodes,
                    int nb_nodes);

/**
 * Inverse operation of mami_distribute. All the pages of the
 * given memory are moved back to the given node.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param node destination node
 */
extern
int mami_gather(mami_manager_t *memory_manager,
                void *buffer,
                int node);

/* @} */

#endif /* MM_MAMI_H */
#endif /* MM_MAMI_ENABLED */

/* @} */
