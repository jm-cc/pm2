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

/** \brief Type of a memory migration cost from node to node */
typedef struct mami_migration_cost_s mami_migration_cost_t;

/** \brief Type of a link for a list of tree nodes */
typedef struct mami_data_link_s mami_data_link_t;

/** \brief Structure of a memory migration cost from node to node */
struct mami_migration_cost_s {
  size_t size_min;
  size_t size_max;
  float slope;
  float intercept;
  float correlation;
};

/** \brief Structure of a link for a list of tree nodes */
struct mami_data_link_s {
  struct list_head list;
  mami_data_t *data;
};

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
  void *startaddress;
  /** \brief End address of the memory area */
  void *endaddress;
  /** \brief Size of the memory area */
  size_t size;
  /** \brief Protection of the memory area */
  int protection;
  /** \brief Is the memory based on huge pages */
  int with_huge_pages;

  /** \brief Page addresses of the memory area */
  void **pageaddrs;
  /** \brief Number of pages holding the memory area */
  int nbpages;

  /** \brief Node where the memory area is located. Meaningful when all memory allocated on the same node. Otherwise value is -1 */
  int node;
  /** \brief Nodes where the memory area is located. Used when pages allocated on different nodes */
  int *nodes;

  /** \brief Tag indicating if the memory has been allocated by MaMI */
  int mami_allocated;
  /** \brief Tag indicating the memory status */
  mami_status_t status;
  /** \brief Entities which are attached to the memory area */
  p_tbx_slist_t owners;

  /** \brief */
  mami_data_t *next;

  /** \brief Start address of the area to use when calling mprotect */
  void *mprotect_startaddress;
  /** \brief Size of the area to use when calling mprotect */
  size_t mprotect_size;
};

/** \brief Structure of a sorted-binary tree of allocated memory areas */
struct mami_tree_s {
  /** \brief Left child of the tree */
  struct mami_tree_s *leftchild;
  /** \brief Right child of the tree */
  struct mami_tree_s *rightchild;
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
  int nbpages;
  /** \brief Page size */
  unsigned long pagesize;
  /** \brief Protection of the memory area */
  int protection;
  /** \brief Next pre-allocated space */
  struct mami_area_s *next;
};

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

extern
int _mm_mami_update_stats(marcel_entity_t *entity,
                          int source,
                          int dest,
                          size_t size);

#endif /* MM_MAMI_PRIVATE_H */
#endif /* MM_MAMI_ENABLED */
