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

/** \file
 * \brief MAMI: MArcel Memory Interface
 */

/** \defgroup marcel_memory MAMI: MArcel Memory Interface
 *
 * This is the interface for memory management.
 *
 * @{
 */

#section common
#ifdef MARCEL_MAMI_ENABLED

#section types
/** \brief Type of a link for a list of tree nodes */
typedef struct marcel_memory_data_link_s marcel_memory_data_link_t;

/** \brief Type of a tree node */
typedef struct marcel_memory_data_s marcel_memory_data_t;

/** \brief Type of a sorted-binary tree of allocated memory areas */
typedef struct marcel_memory_tree_s marcel_memory_tree_t;

/** \brief Type of a pre-allocated space (start address + number of pages) */
typedef struct marcel_memory_area_s marcel_memory_area_t;

/** \brief Type of a reading or writing access cost from node to node */
typedef struct marcel_access_cost_s marcel_access_cost_t;

/** \brief Type of a memory migration cost from node to node */
typedef struct marcel_memory_migration_cost_s marcel_memory_migration_cost_t;

/** \brief Type of a preallocated space with huge pages */
typedef struct marcel_memory_huge_pages_area_s marcel_memory_huge_pages_area_t;

/** \brief Type of a memory manager */
typedef struct marcel_memory_manager_s  marcel_memory_manager_t;

/** \brief Node selection policy */
typedef int marcel_memory_node_selection_policy_t;
/** \brief Selects the least loaded node */
#define MARCEL_MEMORY_LEAST_LOADED_NODE      ((marcel_memory_node_selection_policy_t)0)

/** \brief Type of a memory status */
typedef int marcel_memory_status_t;
/** \brief Initial status: the memory has been allocated */
#define MARCEL_MEMORY_INITIAL_STATUS	  ((marcel_memory_status_t)0)
/** \brief The memory has been migrated following a next touch mark */
#define MARCEL_MEMORY_NEXT_TOUCHED_STATUS ((marcel_memory_status_t)1)
/** \brief The memory has been tagged for a kernel migration */
#define MARCEL_MEMORY_KERNEL_MIGRATION_STATUS ((marcel_memory_status_t)2)

/** \brief Type of a memory location policy */
typedef int marcel_memory_membind_policy_t;
/** \brief The memory will be allocated with the policy specified by the last call to marcel_memory_membind() */
#define MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT           ((marcel_memory_membind_policy_t)0)
/** \brief The memory will be allocated on the current node */
#define MARCEL_MEMORY_MEMBIND_POLICY_NONE              ((marcel_memory_membind_policy_t)2)
/** \brief The memory will be allocated on the given node */
#define MARCEL_MEMORY_MEMBIND_POLICY_SPECIFIC_NODE     ((marcel_memory_membind_policy_t)4)
/** \brief The memory will be allocated on the least loaded node */
#define MARCEL_MEMORY_MEMBIND_POLICY_LEAST_LOADED_NODE ((marcel_memory_membind_policy_t)8)
/** \brief The memory will be allocated via huge pages */
#define MARCEL_MEMORY_MEMBIND_POLICY_HUGE_PAGES        ((marcel_memory_membind_policy_t)16)
/** \brief The memory will not be bound to any node */
#define MARCEL_MEMORY_MEMBIND_POLICY_FIRST_TOUCH       ((marcel_memory_membind_policy_t)32)

/** \brief Type of a statistic */
typedef int marcel_memory_stats_t;
/** \brief The total amount of memory */
#define MARCEL_MEMORY_STAT_MEMORY_TOTAL      ((marcel_memory_stats_t)0)
/** \brief The free amount of memory */
#define MARCEL_MEMORY_STAT_MEMORY_FREE       ((marcel_memory_stats_t)1)

#section structures

#depend "marcel_mutex.h[types]"
#include "tbx_pointers.h"

/** \brief Structure of a link for a list of tree nodes */
struct marcel_memory_data_link_s {
  struct list_head list;
  marcel_memory_data_t *data;
};

/** \brief Structure of a tree node */
struct marcel_memory_data_s {
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
  /** \brief Node where the memory area is located */
  int node;

  /** \brief Tag indicating if the memory has been allocated by MAMI */
  int mami_allocated;
  /** \brief Tag indicating the memory status */
  marcel_memory_status_t status;
  /** \brief Entities which are attached to the memory area */
  p_tbx_slist_t owners;

  /** \brief */
  marcel_memory_data_t *next;
};

/** \brief Structure of a sorted-binary tree of allocated memory areas */
struct marcel_memory_tree_s {
  /** \brief Left child of the tree */
  struct marcel_memory_tree_s *leftchild;
  /** \brief Right child of the tree */
  struct marcel_memory_tree_s *rightchild;
  /** \brief Node of the tree */
  marcel_memory_data_t *data;
};

/** \brief Structure of a pre-allocated space (start address + number of pages) */
struct marcel_memory_area_s {
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
  struct marcel_memory_area_s *next;
};

/** \brief Structure of a reading or writing access cost from node to node */
struct marcel_access_cost_s {
  float cost;
};

/** \brief Structure of a memory migration cost from node to node */
struct marcel_memory_migration_cost_s {
  size_t size_min;
  size_t size_max;
  float slope;
  float intercept;
  float correlation;
};

/** \brief Structure of a preallocated space with huge pages */
struct marcel_memory_huge_pages_area_s {
  void *buffer;
  char *filename;
  int file;
  size_t size;
  marcel_memory_area_t *heap;
};

/** \brief Structure of a memory manager */
struct marcel_memory_manager_s {
  /** \brief Number of nodes */
  unsigned nb_nodes;
  /** \brief Memory migration costs from all the nodes to all the nodes */
  p_tbx_slist_t **migration_costs;
  /** \brief Reading access costs from all the nodes to all the nodes */
  marcel_access_cost_t **reading_access_costs;
  /** \brief Writing access costs from all the nodes to all the nodes */
  marcel_access_cost_t **writing_access_costs;
  /** \brief Tree containing all the allocated memory areas */
  marcel_memory_tree_t *root;
  /** \brief List of pre-allocated memory areas for huge pages */
  marcel_memory_huge_pages_area_t **huge_pages_heaps;
  /** \brief List of pre-allocated memory areas */
  marcel_memory_area_t **heaps;
  /** \brief Total memory per node */
  unsigned long *memtotal;
  /** \brief Free memory per node */
  unsigned long *memfree;
  /** \brief Lock to manipulate the data */
  marcel_mutex_t lock;
  /** \brief System page size */
  unsigned long normalpagesize;
  /** \brief System huge page size */
  unsigned long hugepagesize;
  /** \brief Number of initially pre-allocated pages */
  int initially_preallocated_pages;
  /** \brief Cache line size */
  int cache_line_size;
  /** \brief Current membind policy */
  marcel_memory_membind_policy_t membind_policy;
  /** \brief Node for the membind policy */
  int membind_node;
  /** \brief Are memory addresses aligned on page boundaries or not? */
  int alignment;
  /** \brief Is the kernel next touch migration available */
  int kernel_nexttouch_migration;
};

#section marcel_functions

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
                          int node);

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

#section functions

/**
 * Initialises the memory manager.
 * @param memory_manager pointer to the memory manager
 */
extern
void marcel_memory_init(marcel_memory_manager_t *memory_manager);

/**
 * Shutdowns the memory manager.
 * @param memory_manager pointer to the memory manager
 */
extern
void marcel_memory_exit(marcel_memory_manager_t *memory_manager);

/**
 * Indicates memory addresses given to MaMI should be aligned to page boundaries.
 * @param memory_manager pointer to the memory manager
 */
extern
void marcel_memory_set_alignment(marcel_memory_manager_t *memory_manager);

/**
 * Indicates memory addresses given to MaMI should NOT be aligned to page boundaries.
 * @param memory_manager pointer to the memory manager
 */
extern
void marcel_memory_unset_alignment(marcel_memory_manager_t *memory_manager);

/**
 * Allocates memory on the current node. Size will be rounded up to the system page size.
 * @param memory_manager pointer to the memory manager
 * @param size size of the required memory
 * @param policy
 * @param node
 */
extern
void* marcel_memory_malloc(marcel_memory_manager_t *memory_manager,
			   size_t size,
                           marcel_memory_membind_policy_t policy,
                           int node);

/**
 * Allocates memory on the current node. Size will be rounded up to the system page size.
 * @param memory_manager pointer to the memory manager
 * @param nmemb number of elements to allocate
 * @param size size of a single element
 * @param policy memory allocation policy
 * @param node
 */
extern
void* marcel_memory_calloc(marcel_memory_manager_t *memory_manager,
			   size_t nmemb,
			   size_t size,
                           marcel_memory_membind_policy_t policy,
                           int node);

/**
 * Register a memory area which has not been allocated by MAMI.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param size size of the memory area
 */
extern
int marcel_memory_register(marcel_memory_manager_t *memory_manager,
			   void *buffer,
			   size_t size);

/**
 * Unregister a memory area which has not been allocated by MAMI.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 */
extern
int marcel_memory_unregister(marcel_memory_manager_t *memory_manager,
			     void *buffer);

/**
 * Split a memory area into subareas.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param subareas number of subareas
 * @param newbuffers addresses of the new memory areas
 */
extern
int marcel_memory_split(marcel_memory_manager_t *memory_manager,
                        void *buffer,
                        unsigned int subareas,
                        void **newbuffers);

/**
 *
 */
extern
int marcel_memory_membind(marcel_memory_manager_t *memory_manager,
                          marcel_memory_membind_policy_t policy,
                          int node);

/**
 * Free the given memory.
 * @param memory_manager pointer to the memory manager
 * @param buffer pointer to the memory
 */
extern
void marcel_memory_free(marcel_memory_manager_t *memory_manager,
			void *buffer);

/**
 * @param memory_manager pointer to the memory manager
 * @param buffer pointer to the memory to be located
 * @param size size of the memory area to be located
 * @param node returns the location of the given memory
 * @return a negative value and set errno to EINVAL when address not found
 */
extern
int marcel_memory_locate(marcel_memory_manager_t *memory_manager,
                         void *buffer,
                         size_t size,
                         int *node);

/**
 * @param memory_manager pointer to the memory manager
 */
extern
void marcel_memory_print(marcel_memory_manager_t *memory_manager);

/**
 * @param stream
 * @param memory_manager pointer to the memory manager
 */
extern
void marcel_memory_fprint(FILE *stream, marcel_memory_manager_t *memory_manager);

/**
 * Indicates the migration cost for SIZE bits from node SOURCE to node DEST.
 * @param memory_manager pointer to the memory manager
 * @param source source node
 * @param dest destination node
 * @param size how many bits do we want to migrate
 * @param cost estimated cost of the migration
 */
extern
void marcel_memory_migration_cost(marcel_memory_manager_t *memory_manager,
                                  int source,
                                  int dest,
                                  size_t size,
                                  float *cost);

/**
 * Indicates the writing access cost for SIZE bits from node SOURCE to node DEST.
 * @param memory_manager pointer to the memory manager
 * @param source source node
 * @param dest destination node
 * @param size how many bits do we want to access
 * @param cost estimated cost of the access
 */
extern
void marcel_memory_writing_access_cost(marcel_memory_manager_t *memory_manager,
                                       int source,
                                       int dest,
                                       size_t size,
                                       float *cost);

/**
 * Indicates the reading access cost for SIZE bits from node SOURCE to node DEST.
 * @param memory_manager pointer to the memory manager
 * @param source source node
 * @param dest destination node
 * @param size how many bits do we want to access
 * @param cost estimated cost of the access
 */
extern
void marcel_memory_reading_access_cost(marcel_memory_manager_t *memory_manager,
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
int marcel_memory_sampling_of_memory_migration(marcel_memory_manager_t *memory_manager,
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
int marcel_memory_sampling_of_memory_access(marcel_memory_manager_t *memory_manager,
                                            unsigned long minsource,
                                            unsigned long maxsource,
                                            unsigned long mindest,
                                            unsigned long maxdest);

/**
 * Select the "best" node based on the given policy.
 * @param memory_manager pointer to the memory manager
 * @param policy selection policy
 * @param node returns the id of the node
 */
extern
int marcel_memory_select_node(marcel_memory_manager_t *memory_manager,
                              marcel_memory_node_selection_policy_t policy,
                              int *node);

/**
 * Migrate the pages to the specified node.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the buffer to be migrated
 * @param dest identifier of the destination node
 */
extern
int marcel_memory_migrate_pages(marcel_memory_manager_t *memory_manager,
                                void *buffer,
                                int dest);

/**
 * Mark the area to be migrated on next touch.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 */
extern
int marcel_memory_migrate_on_next_touch(marcel_memory_manager_t *memory_manager,
                                        void *buffer);

/**
 * Move the area to the specified node.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param node destination
 */
extern
int marcel_memory_migrate_on_node(marcel_memory_manager_t *memory_manager,
                                  void *buffer,
                                  int node);

/**
 * Attach the memory to the specified thread
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param size size of the memory area
 * @param owner thread
 * @param node
 */
extern
int marcel_memory_task_attach(marcel_memory_manager_t *memory_manager,
                              void *buffer,
                              size_t size,
                              marcel_t owner,
                              int *node);

/**
 * Unattach the memory to the specified thread
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param owner thread
 */
extern
int marcel_memory_task_unattach(marcel_memory_manager_t *memory_manager,
                                void *buffer,
                                marcel_t owner);

/**
 * Migrate to the given nodes all the memory areas attached to the specified thread
 * @param memory_manager pointer to the memory manager
 * @param owner thread
 * @param node
 */
extern
int marcel_memory_task_migrate_all(marcel_memory_manager_t *memory_manager,
                                   marcel_t owner,
                                   int node);

/**
 * Attach the memory to the specified bubble
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param size size of the memory area
 * @param owner bubble
 * @param node will contain the node id where the data is located
 */
extern
int marcel_memory_bubble_attach(marcel_memory_manager_t *memory_manager,
                                void *buffer,
                                size_t size,
                                marcel_bubble_t *owner,
                                int *node);

/**
 * Unattach the memory to the specified bubble
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 * @param owner bubble
 */
extern
int marcel_memory_bubble_unattach(marcel_memory_manager_t *memory_manager,
                                  void *buffer,
                                  marcel_bubble_t *owner);

/**
 * Migrate to the given nodes all the memory areas attached to the specified bubble
 * @param memory_manager pointer to the memory manager
 * @param owner thread
 * @param node
 */
extern
int marcel_memory_bubble_migrate_all(marcel_memory_manager_t *memory_manager,
                                     marcel_bubble_t *owner,
                                     int node);

/**
 * Unattach all the memory areas attached to the specified thread.
 * @param memory_manager pointer to the memory manager
 * @param owner thread
 */
extern
int marcel_memory_task_unattach_all(marcel_memory_manager_t *memory_manager,
                                    marcel_t owner);

/**
 * Indicates if huge pages are available on the system.
 */
extern
int marcel_memory_huge_pages_available(marcel_memory_manager_t *memory_manager);

/**
 * Checks the location of the memory area.
 */
extern
int marcel_memory_check_pages_location(marcel_memory_manager_t *memory_manager,
                                       void *buffer,
                                       size_t size,
                                       int node);

/**
 * Updates the location of the memory area.
 */
extern
int marcel_memory_update_pages_location(marcel_memory_manager_t *memory_manager,
                                        void *buffer,
                                        size_t size);

/**
 * Indicates the value of the statistic STAT for the node NODE
 */
extern
int marcel_memory_stats(marcel_memory_manager_t *memory_manager,
                        int node,
                        marcel_memory_stats_t stat,
                        unsigned long *value);

#section common
#endif /* MARCEL_MAMI_ENABLED */

/* @} */
