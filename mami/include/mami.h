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
/** \file
 * \brief MaMI: Marcel Memory Interface
 */

/** \defgroup mami MaMI: Marcel Memory Interface
 *
 * \htmlonly
 * <div class="section" id="mami">
 * \endhtmlonly
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
 * node. Both move pages using the Linux system call move_pages(). We
 * have also developed an implementation of a Next-touch policy in the
 * kernel (see http://hal.inria.fr/inria-00358172). On systems which
 * provide it, MaMI will use this method instead of the user-level
 * one.
 *
 * It is possible to evaluate reading and writing access costs to
 * remote memory areas. Migration cost is based on a linear function
 * on the number of pages being migrated.
 *
 * Moreover, MaMI gathers statistics on how much memory is available
 * and left on the different nodes. This information is potentially
 * helpful when deciding whether or not to migrate a memory area.
 *
 * More information about MaMI can be found at
 * http://runtime.bordeaux.inria.fr/MaMI/.
 *
 * @{
 */

#ifdef MAMI_ENABLED

#ifndef MAMI_H
#define MAMI_H

#include <stdio.h>
#include <tbx_topology.h>

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
 * @param argc
 * @param argv
 */
extern
void mami_init(mami_manager_t **memory_manager, int argc, char **argv);

/**
 * Shutdowns the memory manager.
 * @param memory_manager pointer to the memory manager
 */
extern
void mami_exit(mami_manager_t **memory_manager);

/**
 * Indicates that memory addresses given to MaMI should be aligned to page boundaries.
 * @param memory_manager pointer to the memory manager
 * @return 0
 */
extern
int mami_set_alignment(mami_manager_t *memory_manager);

/**
 * Indicates that memory addresses given to MaMI should NOT be aligned to page boundaries.
 * @param memory_manager pointer to the memory manager
 * @return 0
 */
extern
int mami_unset_alignment(mami_manager_t *memory_manager);

/**
 * Locates the given memory.
 * @param memory_manager pointer to the memory manager
 * @param buffer pointer to the memory to be located
 * @param size size of the memory area to be located
 * @param[out] node returns the location of the given memory
 * @return 0 on success, -1 and set errno to EINVAL when address not known by MaMI
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
 * @return 1 when huges pages are available, 0 otherwise
 */
extern
int mami_huge_pages_available(mami_manager_t *memory_manager);

/**
 * Tells MaMI to use kernel migration when it is available.
 * @param memory_manager pointer to the memory manager
 * @return 1 when kernel migration is enabled, 0 otherwise (the system
 * does not provide it)
 */
extern
int mami_set_kernel_migration(mami_manager_t *memory_manager);

/**
 * Tells MaMI not to use kernel migration even when it is available.
 * @param memory_manager pointer to the memory manager
 * @return 0
 */
extern
int mami_unset_kernel_migration(mami_manager_t *memory_manager);

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
 * @return the allocated buffer, or NULL if size or node invalid, or
 * when there is no space left
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
 * @return the allocated buffer, or NULL if size or node invalid, or
 * when there is no space left
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
 * @return 0
 */
extern
int mami_register(mami_manager_t *memory_manager,
                  void *buffer,
                  size_t size);

/**
 * Unregisters a memory area which has not been allocated by MaMI.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @return same code as mami_locate()
 */
extern
int mami_unregister(mami_manager_t *memory_manager,
                    void *buffer);

/**
 * Splits a memory area into subareas.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param subareas number of subareas
 * @param[out] newbuffers addresses of the new memory areas
 * @return same code as mami_locate() when address not known by MaMI
 * or when memory not big enough to be splitted in the given number of
 * subareas
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
 * @return 0 on success, -1 and sets errno to EINVAL when the policy is invalid
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
 * @return 0 on success, -1 and sets errno to EINVAL when address not
 * known by MaMI or pages located on another node
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
 * @return same code as mami_locate()
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
 * @param[out] value will contain the value of the given statistic for the given node
 * @return 0 on success, -1 and sets errno to EINVAL when invalid node or statistic
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
 * @param[out] node returns the id of the node
 * @return 0 on success, -1 and sets errno to EINVAL when invalid policy
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
 * @param[out] cost estimated cost of the migration
 * @return 0 on success, -1 and sets errno to EINVAL when invalid nodes
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
 * @param[out] cost estimated cost of the access
 * @return 0 on success, -1 and sets errno to EINVAL when invalid nodes
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
 * @param[out] cost estimated cost of the access
 * @return 0 on success, -1 and sets errno to EINVAL when invalid nodes
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
 * @return 0 on success, a negative value if output file cannot be created
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
 * @return 0 on success, a negative value if output file cannot be created
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
 * @return 0 on success, -1 and sets errno to EINVAL when address not
 * known by MaMI or to EALREADY when pages already located on the
 * given node
 */
extern
int mami_migrate_on_node(mami_manager_t *memory_manager,
                         void *buffer,
                         int dest);

/**
 * Marks the area to be migrated on next touch.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @return 0 on success, -1 and sets errno to EINVAL when address not
 * known by MaMI or to other specific error values when setting the
 * next touch policy
 */
extern
int mami_migrate_on_next_touch(mami_manager_t *memory_manager,
                               void *buffer);

/**
 * Distributes pages of the given memory on the given set of nodes
 * using a round robin policy. The first page is migrated to the first
 * node, the second page to the second node, ....
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param nodes destination nodes
 * @param nb_nodes number of nodes in the \e nodes array
 * @return 0 on success, -1 and sets errno to EINVAL when address not
 * known by MaMI or to other specific error values when moving pages
 */
extern
int mami_distribute(mami_manager_t *memory_manager,
                    void *buffer,
                    int *nodes,
                    int nb_nodes);

/* @} */

#ifdef MARCEL
#  include "mami_marcel.h"
#endif

#endif /* MAMI_H */
#endif /* MAMI_ENABLED */

/* @} */
