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

/** \defgroup marcel_memory Memory Interface
 *
 * This is the interface for memory management
 *
 * @{
 */

#section types
#depend "linux_spinlock.h[types]"
#depend "marcel_spin.h[types]"
/** */
typedef struct marcel_memory_data_s {
  /** */
  void *startaddress;
  /** */
  void *endaddress;
  /** */
  size_t size;

  /** */
  void **pageaddrs;
  /** */
  int nbpages;
  /** */
  int *nodes;
} marcel_memory_data_t;

/** */
typedef struct marcel_memory_tree_s {
  /** */
  struct marcel_memory_tree_s *leftchild;
  /** */
  struct marcel_memory_tree_s *rightchild;
  /** */
  marcel_memory_data_t *data;
} marcel_memory_tree_t;

/** */
typedef struct marcel_memory_space_s {
  /** */
  void *start;
  /** */
  int nbpages;
  /** */
  struct marcel_memory_space_s *next;
} marcel_memory_space_t;

/** */
typedef struct marcel_memory_manager_s {
  /** */
  marcel_memory_tree_t *root;
  /** */
  marcel_memory_space_t **heaps;
  /** */
  marcel_spinlock_t lock;
  /** */
  int pagesize;
  /** */
  int initialpreallocatedpages;
} marcel_memory_manager_t;

#section marcel_functions

/*
 *
 */
void ma_memory_init_memory_data(marcel_memory_manager_t *memory_manager,
				void **pageaddrs,
				int nbpages,
				size_t size,
				int *nodes,
				marcel_memory_data_t **memory_data);

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
 *
 */
void ma_memory_register(marcel_memory_manager_t *memory_manager,
			marcel_memory_tree_t **memory_tree,
			void **pageaddrs,
			int nbpages,
			size_t size,
			int *nodes);

/*
 * Preallocates some memory (in number of pages) on the specified numa node.
 */
void ma_memory_preallocate(marcel_memory_manager_t *memory_manager,
			   marcel_memory_space_t **space,
			   int node);

/*
 *
 */
void ma_memory_free_from_node(marcel_memory_manager_t *memory_manager,
			      void *buffer,
			      int nbpages,
			      int node);

#section functions

/**
 * Initialises the memory manager.
 * @param memory_manager pointer to the memory manager
 * @param initialpreallocatedpages number of initial preallocated pages on each node
 */
void marcel_memory_init(marcel_memory_manager_t *memory_manager,
			int initialpreallocatedpages);

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
void marcel_memory_locate(marcel_memory_manager_t *memory_manager,
			  marcel_memory_tree_t *memory_tree,
			  void *address,
			  int *node);

/**
 * @param memory_manager pointer to the memory manager
 */
void marcel_memory_print(marcel_memory_manager_t *memory_manager);

/* @} */
