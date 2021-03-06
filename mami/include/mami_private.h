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

#ifndef MAMI_PRIVATE_H
#define MAMI_PRIVATE_H

#include <stdint.h>
#include <stdlib.h>
#include "mami_thread.h"
#include "mami_list.h"

#define _MAMI_LIKELY(x)		__builtin_expect((long)!!(x), 1L)
#define _MAMI_UNLIKELY(x)	__builtin_expect((long)!!(x), 0L)

/** \brief Type of a tree node */
typedef struct mami_data_s mami_data_t;

/** \brief Type of a sorted-binary tree of allocated memory areas */
typedef struct mami_tree_s mami_tree_t;

/** \brief Type of a pre-allocated space (start address + number of pages) */
typedef struct mami_area_s mami_area_t;

/** \brief Type of a reading or writing access cost from node to node */
typedef struct mami_access_cost_s mami_access_cost_t;

/** \brief Type of a preallocated space with huge pages */
typedef struct mami_huge_pages_area_s mami_huge_pages_area_t;

/** \brief Type of a memory status */
typedef int mami_status_t;
/** \brief Initial status: the memory has been allocated */
#define MAMI_INITIAL_STATUS	  ((mami_status_t)0)
/** \brief The memory has been migrated following a next touch mark */
#define MAMI_NEXT_TOUCHED_STATUS ((mami_status_t)1)
/** \brief The memory has been tagged for a kernel migration */
#define MAMI_KERNEL_MIGRATION_STATUS ((mami_status_t)2)
/** \brief The memory has been tagged for a user-space migration */
#define MAMI_USERSPACE_MIGRATION_STATUS ((mami_status_t)3)
/** \brief The memory has been tagged for a attach on next touch */
#define MAMI_ATTACH_ON_NEXT_TOUCH_STATUS ((mami_status_t)4)

#ifdef MARCEL
#  include "mami_marcel_private.h"
#endif

/** \brief Structure of a memory migration cost from node to node */
LIST_TYPE(mami_migration_cost,
        size_t size_min;
        size_t size_max;
        float slope;
        float intercept;
        float correlation;
          );

/** \brief Structure of a reading or writing access cost from node to node */
struct mami_access_cost_s {
        float cost;
};

/** \brief Structure of a preallocated space with huge pages */
struct mami_huge_pages_area_s {
        void *buffer;
        char *filename;
        int file;
        size_t size;
        mami_area_t *heap;
};

/** \brief Structure of a tree node */
struct mami_data_s {
        /** \brief Start address of the memory area */
        void *start_address;
        /** \brief End address of the memory area */
        void *end_address;
        /** \brief Size of the memory area */
        size_t size;
        /** \brief Protection of the memory area */
        int protection;
        /** \brief Is the memory based on huge pages */
        int with_huge_pages;

        /** \brief Page addresses of the memory area */
        void **pageaddrs;
        /** \brief Number of pages holding the memory area */
        unsigned long nb_pages;

        /** \brief Node where the memory area is located. Meaningful when all memory allocated on the same node. Otherwise value is -1 */
        int node;
        /** \brief Nodes where the memory area is located. Used when pages allocated on different nodes */
        int *nodes;
        /** \brief Number of pages per node. Used when pages allocated on different nodes */
        unsigned long *pages_per_node;
        /** \brief Number of pages on undefined node. Used when pages allocated on different nodes */
        unsigned long pages_on_undefined_node;

        /** \brief Tag indicating if the memory has been allocated by MaMI */
        int mami_allocated;
        /** \brief Tag indicating the memory status */
        mami_status_t status;

#ifdef MARCEL
        /** \brief Entities (tasks and bubbles) which are attached to the memory area */
        mami_entity_list_t owners;
#endif

        /** \brief */
        mami_data_t *next;

        /** \brief Start address of the area to use when calling mprotect */
        void *mprotect_start_address;
        /** \brief Size of the area to use when calling mprotect */
        size_t mprotect_size;
};

/** \brief Structure of a sorted-binary tree of allocated memory areas */
struct mami_tree_s {
        /** \brief Left child of the tree */
        struct mami_tree_s *left_child;
        /** \brief Right child of the tree */
        struct mami_tree_s *right_child;
        /** \brief Node of the tree */
        mami_data_t *data;
};

/** \brief Structure of a pre-allocated space (start address + number of pages) */
struct mami_area_s {
        /** \brief Start address of the memory area */
        void *start;
        /** \brief End address of the memory area */
        void *end;
        /** \brief Number of pages of the memory area */
        unsigned long nb_pages;
        /** \brief Page size */
        unsigned long page_size;
        /** \brief Protection of the memory area */
        int protection;
        /** \brief Next pre-allocated space */
        struct mami_area_s *next;
};

/** \brief Structure of a memory manager */
struct mami_manager_s {
        /** \brief Number of nodes */
        unsigned nb_nodes;
        /** \brief Max node (used for mbind) */
        unsigned max_node;
        /** brief Id of the OS node for each node */
        int* os_nodes;
        /** \brief Memory migration costs from all the nodes to all the nodes */
        mami_migration_cost_list_t **migration_costs;
        /** \brief Reading access costs from all the nodes to all the nodes */
        mami_access_cost_t **costs_for_read_access;
        /** \brief Writing access costs from all the nodes to all the nodes */
        mami_access_cost_t **costs_for_write_access;
        /** \brief Tree containing all the allocated memory areas */
        mami_tree_t *root;
        /** \brief List of pre-allocated memory areas for huge pages */
        mami_huge_pages_area_t **huge_pages_heaps;
        /** \brief List of pre-allocated memory areas */
        mami_area_t **heaps;
        /** \brief Total memory per node (in bytes) */
        unsigned long *mem_total;
        /** \brief Free memory per node (in bytes) */
        unsigned long *mem_free;
        /** \brief Lock to manipulate the data */
        th_mami_mutex_t lock;
        /** \brief System page size */
        unsigned long normal_page_size;
        /** \brief System huge page size */
        unsigned long huge_page_size;
        /** \brief number of huge pages per node */
        int *huge_page_free;
        /** \brief Number of initially pre-allocated pages */
        int initially_preallocated_pages;
        /** \brief Cache line size */
        int cache_line_size;
        /** \brief Current membind policy */
        mami_membind_policy_t membind_policy;
        /** \brief Node for the membind policy */
        int membind_node;
        /** \brief Are memory addresses aligned on page boundaries or not? */
        int alignment;
        /** \brief Is the kernel next touch migration available */
        int kernel_nexttouch_migration_available;
        /** \brief Is the kernel next touch migration requested by the user */
        int kernel_nexttouch_migration_requested;
        /** \brief Is the migration available */
        int migration_flag;
};

#define MAMI_KERNEL_NEXT_TOUCH_MOF   1
#define MAMI_KERNEL_NEXT_TOUCH_BRICE 2

/* align a application-given address to the closest page-boundary:
 * re-add the lower bits to increase the bit above pagesize if needed, and truncate
 */
#define _MAMI_ALIGN_ON_PAGE(address, page_size) (void*)((((uintptr_t) address) + (page_size >> 1)) & (~(page_size-1)))
#define MAMI_ALIGN_ON_PAGE(memory_manager, address, page_size) (memory_manager->alignment?_MAMI_ALIGN_ON_PAGE(address, page_size):address)

/* Node id used for first touch allocated memory */
#define MAMI_FIRST_TOUCH_NODE memory_manager->nb_nodes
/* Node id used when memory area located on several nodes */
#define MAMI_MULTIPLE_LOCATION_NODE -20
/* Node id used when unknown location */
#define MAMI_UNKNOWN_LOCATION_NODE -1

extern
void _mami_delete_tree(mami_manager_t *memory_manager,
                       mami_tree_t **memory_tree);

extern
int _mami_register(mami_manager_t *memory_manager,
                   void *buffer,
                   void *endbuffer,
                   size_t size,
                   void *initial_buffer,
                   size_t initial_size,
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
                      unsigned long nbpages,
                      int vnode,
                      int pnode);

extern
int _mami_preallocate_huge_pages(mami_manager_t *memory_manager,
                                 mami_huge_pages_area_t **space,
                                 int node);

extern
int _mami_check_pages_location(const void **pageaddrs,
                               unsigned long pages,
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
                             const void **pageaddrs,
                             unsigned long nbpages,
                             int *node,
                             int *nodes);

extern
int _mami_migrate_on_node(mami_manager_t *memory_manager,
                          mami_data_t *data,
                          int dest);

extern
int _mami_current_node(void);

extern
int _mami_attr_settopo_level(th_mami_attr_t *attr, int node);

#endif /* MAMI_PRIVATE_H */
#endif /* MAMI_ENABLED */
