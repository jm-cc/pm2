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

#include "marcel.h"

/** */
typedef struct memory_data_s {
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
} memory_data_t;

/** */
typedef struct memory_tree_s {
  /** */
  struct memory_tree_s *leftchild;
  /** */
  struct memory_tree_s *rightchild;
  /** */
  memory_data_t *data;
} memory_tree_t;

/** */
typedef struct memory_space_s {
  /** */
  void *start;
  /** */
  int nbpages;
  /** */
  struct memory_space_s *next;
} memory_space_t;

/** */
typedef struct memory_manager_s {
  /** */
  memory_tree_t *root;
  /** */
  memory_space_t **heaps;
  /** */
  marcel_spinlock_t lock;
  /** */
  int pagesize;
  /** */
  int initialpreallocatedpages;
} memory_manager_t;

/**
 * Initialises the memory manager.
 * @param memory_manager pointer to the memory manager
 * @param initialpreallocatedpages number of initial preallocated pages on each node
 */
void memory_manager_init(memory_manager_t *memory_manager,
			 int initialpreallocatedpages);

/**
 *
 */
void memory_manager_create_memory_data(memory_manager_t *memory_manager,
				       void **pageaddrs,
				       int nbpages,
				       size_t size,
				       int *nodes,
				       memory_data_t **memory_data);

/**
 *
 */
void memory_manager_delete_tree(memory_manager_t *memory_manager,
				memory_tree_t **memory_tree);

/**
 *
 */
void memory_manager_delete(memory_manager_t *memory_manager,
			   memory_tree_t **memory_tree,
			   void *buffer);

/**
 *
 */
void memory_manager_register(memory_manager_t *memory_manager,
			     memory_tree_t **memory_tree,
			     void **pageaddrs,
			     int nbpages,
			     size_t size,
			     int *nodes);

/**
 * Preallocates some memory (in number of pages) on the specified numa node.
 */
void memory_manager_preallocate(memory_manager_t *memory_manager,
				memory_space_t **space,
				int node);

/**
 * Allocates memory on a specific node. Size will be rounded up to the system page size.
 */
void* memory_manager_allocate_on_node(memory_manager_t *memory_manager,
				      size_t size,
				      int node);

/**
 *
 */
void* memory_manager_free_from_node(memory_manager_t *memory_manager,
				    void *buffer,
				    int nbpages,
				    int node);

/**
 * Allocates memory on the current node. Size will be rounded up to the system page size.
 */
void* memory_manager_malloc(memory_manager_t *memory_manager,
			    size_t size);

/**
 * Allocates memory on the current node. Size will be rounded up to the system page size.
 */
void* memory_manager_calloc(memory_manager_t *memory_manager,
			    size_t nmemb,
			    size_t size);

/**
 *
 */
void memory_manager_free(memory_manager_t *memory_manager,
			 void *buffer);

/**
 *
 */
void memory_manager_locate(memory_manager_t *memory_manager,
			   memory_tree_t *memory_tree,
			   void *address,
			   int *node);

/**
 *
 */
void memory_manager_print(memory_tree_t *memory_tree);
