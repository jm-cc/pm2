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

/** \brief Type of a memory manager */
typedef struct marcel_memory_manager_s  marcel_memory_manager_t;

/** \brief Node selection policy */
typedef int marcel_memory_node_selection_policy_t;
#define MARCEL_MEMORY_LEAST_LOADED_NODE      ((marcel_memory_node_selection_policy_t)0)

/** \brief Type of a memory status */
typedef int marcel_memory_status_t;
/** \brief Initial status: the memory has been allocated */
#define MARCEL_MEMORY_INITIAL_STATUS	  ((marcel_memory_status_t)0)
/** \brief The memory has been migrated following a next touch mark */
#define MARCEL_MEMORY_NEXT_TOUCHED_STATUS ((marcel_memory_status_t)1)

/** \brief Type of a memory location policy */
typedef int marcel_memory_membind_policy_t;
/** \brief The memory will be allocated based on the policy specified through the malloc functionality */
#define MARCEL_MEMORY_MEMBIND_POLICY_NONE              ((marcel_memory_membind_policy_t)0)
/** \brief The memory will be allocated on the given node */
#define MARCEL_MEMORY_MEMBIND_POLICY_SPECIFIC_NODE     ((marcel_memory_membind_policy_t)1)
/** \brief The memory will be allocated on the least loaded node */
#define MARCEL_MEMORY_MEMBIND_POLICY_LEAST_LOADED_NODE ((marcel_memory_membind_policy_t)2)

#section structures

#depend "linux_spinlock.h[types]"
#depend "marcel_spin.h[types]"
#include "tbx_pointers.h"

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

  /** \brief Page addresses of the memory area */
  void **pageaddrs;
  /** \brief Number of pages holding the memory area */
  int nbpages;
  /** \brief Nodes where the memory area is located */
  int *nodes;

  /** \brief Tag indicating the memory status */
  marcel_memory_status_t status;
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
  /** \brief Number of pages of the memory area */
  int nbpages;
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

/** \brief Structure of a memory manager */
struct marcel_memory_manager_s {
  /** \brief Memory migration costs from all the nodes to all the nodes */
  p_tbx_slist_t **migration_costs;
  /** \brief Reading access costs from all the nodes to all the nodes */
  marcel_access_cost_t **reading_access_costs;
  /** \brief Writing access costs from all the nodes to all the nodes */
  marcel_access_cost_t **writing_access_costs;
  /** \brief Tree containing all the allocated memory areas */
  marcel_memory_tree_t *root;
  /** \brief List of pre-allocated memory areas */
  marcel_memory_area_t **heaps;
  /** \brief Lock to manipulate the data */
  marcel_spinlock_t lock;
  /** \brief System page size */
  int pagesize;
  /** \brief Number of initially pre-allocated pages */
  int initially_preallocated_pages;
  /** \brief Cache line size */
  int cache_line_size;
  /** \brief Current membind policy */
  marcel_memory_membind_policy_t membind_policy;
  /** \brief Node for the membind policy */
  int membind_node;
};

#section marcel_functions

/*
 *
 */
void ma_memory_delete_tree(marcel_memory_manager_t *memory_manager,
			   marcel_memory_tree_t **memory_tree);

/*
 *
 */
void ma_memory_delete(marcel_memory_manager_t *memory_manager,
		      marcel_memory_tree_t **memory_tree,
		      void *buffer);

/*
 * Preallocates some memory (in number of pages) on the specified numa node.
 */
void ma_memory_preallocate(marcel_memory_manager_t *memory_manager,
			   marcel_memory_area_t **space,
			   int node);

/*
 * Deallocate the memory from the specified numa node.
 */
void ma_memory_deallocate(marcel_memory_manager_t *memory_manager,
			  marcel_memory_area_t **space,
			  int node);

/*
 *
 */
void ma_memory_free_from_node(marcel_memory_manager_t *memory_manager,
			      void *buffer,
			      int nbpages,
			      int node);

/*
 *
 */
void ma_memory_sampling_check_pages_location(void **pageaddrs,
                                             int pages,
                                             int node);

/*
 *
 */
void ma_memory_sampling_migrate_pages(void **pageaddrs,
                                      int pages,
                                      int *nodes,
                                      int *status);

/*
 *
 */
void ma_memory_load_model_for_memory_migration(marcel_memory_manager_t *memory_manager);

/*
 *
 */
void ma_memory_load_model_for_memory_access(marcel_memory_manager_t *memory_manager);

#section functions

/**
 * Initialises the memory manager.
 * @param memory_manager pointer to the memory manager
 * @param preallocatedpages number of initially preallocated pages on each node
 */
void marcel_memory_init(marcel_memory_manager_t *memory_manager,
			int preallocatedpages);

/**
 * Shutdowns the memory manager.
 * @param memory_manager pointer to the memory manager
 */
void marcel_memory_exit(marcel_memory_manager_t *memory_manager);

/**
 * Allocates memory on a specific node. Size will be rounded up to the system page size.
 * @param memory_manager pointer to the memory manager
 * @param size size of the required memory
 * @param node identifier of the node
 */
void* marcel_memory_allocate_on_node(marcel_memory_manager_t *memory_manager,
				     size_t size,
				     int node);

/**
 * Allocates memory on the current node. Size will be rounded up to the system page size.
 * @param memory_manager pointer to the memory manager
 * @param size size of the required memory
 */
void* marcel_memory_malloc(marcel_memory_manager_t *memory_manager,
			   size_t size);

/**
 * Allocates memory on the current node. Size will be rounded up to the system page size.
 * @param memory_manager pointer to the memory manager
 * @param nmemb number of elements to allocate
 * @param size size of a single element
 */
void* marcel_memory_calloc(marcel_memory_manager_t *memory_manager,
			   size_t nmemb,
			   size_t size);

/**
 *
 */
void marcel_memory_membind(marcel_memory_manager_t *memory_manager,
                           marcel_memory_membind_policy_t policy,
                           int node);

/**
 * Free the given memory.
 * @param memory_manager pointer to the memory manager
 * @param buffer pointer to the memory
 */
void marcel_memory_free(marcel_memory_manager_t *memory_manager,
			void *buffer);

/**
 * @param memory_manager pointer to the memory manager
 * @param address pointer to the memory to be located
 * @param node returns the location of the given memory, or -1 when not found
 */
int marcel_memory_locate(marcel_memory_manager_t *memory_manager,
                         void *address,
                         int *node);

/**
 * @param memory_manager pointer to the memory manager
 */
void marcel_memory_print(marcel_memory_manager_t *memory_manager);

/**
 * Indicates the migration cost for SIZE bits from node SOURCE to node DEST.
 * @param memory_manager pointer to the memory manager
 * @param source source node
 * @param dest destination node
 * @param size how many bits do we want to migrate
 * @param cost estimated cost of the migration
 */
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
void marcel_memory_reading_access_cost(marcel_memory_manager_t *memory_manager,
                                       int source,
                                       int dest,
                                       size_t size,
                                       float *cost);

/**
 * Performs the sampling for the memory migration between the specified nodes.
 * @param minsource
 * @param maxsource
 * @param mindest
 * @param maxdest
 * @param extended_mode
 */
void marcel_memory_sampling_of_memory_migration(unsigned long minsource,
                                                unsigned long maxsource,
                                                unsigned long mindest,
                                                unsigned long maxdest,
                                                int extended_mode);

/**
 * Performs the sampling for the memory access between all the nodes.
 * @param memory_manager pointer to the memory manager
 */
void marcel_memory_sampling_of_memory_access(marcel_memory_manager_t *memory_manager);

/**
 * Select the "best" node based on the given policy.
 * @param memory_manager pointer to the memory manager
 * @param policy selection policy
 * @param node returns the id of the node
 */
void marcel_memory_select_node(marcel_memory_manager_t *memory_manager,
                               marcel_memory_node_selection_policy_t policy,
                               int *node);

/**
 * Migrate the pages to the specified node.
 * @param memory_manager pointer to the memory manager
 */
int marcel_memory_migrate_pages(marcel_memory_manager_t *memory_manager,
                                void *buffer, size_t size, int dest);

/**
 * Mark the area to be migrated on next touch.
 * @param memory_manager pointer to the memory manager
 * @param buffer address of the memory area
 */
int marcel_memory_migrate_on_next_touch(marcel_memory_manager_t *memory_manager, void *buffer);

#section common
#endif /* MARCEL_MAMI_ENABLED */

/* @} */
