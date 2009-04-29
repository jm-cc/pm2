/*
 * include/heap_alloc.h
 *
 * Definition of memory allocator inside a predefined heap
 * working for  NUMA architecture
 * and multithread environements
 *
 * Author: Martinasso Maxime
 *
 * (C) Copyright 2007 INRIA
 * Projet: MESCAL / ANR NUMASIS
 *
 */

/** \defgroup memory_heap Heap Allocator Interface
 *
 * This is the interface for memory management
 *
 * @{
 */

#ifdef MM_HEAP_ENABLED

#ifdef LINUX_SYS

#ifndef MM_HEAP_NUMA_ALLOC_H
#define MM_HEAP_NUMA_ALLOC_H

#include <stddef.h>

/** weights of pages */
enum heap_pinfo_weight {
  /** */
  HIGH_WEIGHT,
  /** */
  MEDIUM_WEIGHT,
  /** */
  LOW_WEIGHT
};

/** mapping policies over numa nodes */
enum heap_mem_policy {
  /** */
  CYCLIC,
  /** */
  LESS_LOADED,
  /** */
  SMALL_ACCESSED
};

/** structure used to describe the mapping of a set of pages over numa bloc */
struct heap_pageinfo {
  /** mapping policy */
  int mempolicy;
  /** pages weights */
  int weight;
  /** numa nodes mask */
  unsigned long *nodemask;
  /** number of bits of mask */
  unsigned long maxnode;
  /** */
  size_t size;
  /** distribution of pages over numa nodes */
  int nb_touched[MARCEL_NBMAXNODES];
 };

typedef struct heap_pageinfo heap_pinfo_t;

/**
 * Call heap_amalloc to allocate memory
 * bind heap if is not already binded
 */
void *heap_hmalloc(size_t size, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode,  heap_heap_t *heap);

/**
 * Realloc memory, call to heap_arealloc
 */
void *heap_hrealloc(void *ptr, size_t size);

/**
 * Call heap_acalloc to allocate memory
 * bind heap if is not already binded
 */
void *heap_hcalloc(size_t nmemb, size_t size, int policy, int weight, unsigned long *nodemask, unsigned long maxnode,  heap_heap_t *heap);

/**
 * free memory by calling heap_afree
 */
void heap_hfree(void *ptr);

/**
 * Attach memory to a heap
 * first a heap_hamalloc is done with a size of 0, then the created bloc is set with the mem_stat pointer and the stat_size of the bloc
 * Note that for the mbind to sucess it needs a static size aligned to page sizes
 */
void heap_hattach_memory(void *ptr, size_t size, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode, heap_heap_t *heap);

/**
 * Detach a memory attached to a heap
 */
void heap_hdetach_memory(void *ptr, heap_heap_t *heap);

/**
 * Move touched  physical pages to numa paramter: mempolicy, weight and nodemask of each heap having the ppinfo numa parameter
 * Static memory attached to heap are also moved, however the mbind will only success if the static memory are aligned to pages
 */
int heap_hmove_memory(heap_pinfo_t *ppinfo, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode,  heap_heap_t *heap);

/**
 * Get pages information. Retrieve the numa information of (void*)ptr into ppinfo from heap
 * Works also if (void*)ptr is a static memory attached to heap
 */
void heap_hget_pinfo(void *ptr, heap_pinfo_t* ppinfo, heap_heap_t *heap);

/**
 * Get all pages information from heap
 * Iterate over the different numa information of a list of heap
 * To initiate the iteration ppinfo has to be null
 * ppinfo is allocated inside this method one time
 */
int heap_hnext_pinfo(heap_pinfo_t **ppinfo, heap_heap_t *heap);

/**
 * Check pages location
 * Retrieve the number of pages touched for each numa nodes
 * Data are stored in ppinfo, and concerne each heap and static memory associated to numa information of ppinfo
 * To have this information the method call the syscall __NR_move_pages
 */
void heap_hupdate_memory_nodes(heap_pinfo_t *ppinfo, heap_heap_t *heap);

/**
 * Heap creation
 * Create a heap with a minimum size
 */
heap_heap_t* heap_hcreate_heap(void);

/**
 * Heap destruction
 * Delete a heap
 */
void heap_hdelete_heap(heap_heap_t *heap);

/**
 * merge two heaps
 * note: the iterator of (heap_heap_t*)h is freed
 */
void heap_hmerge_heap(heap_heap_t *hacc, heap_heap_t *h);

#endif /* MM_HEAP_NUMA_ALLOC_H */
#endif /* LINUX_SYS */
#endif /* MM_HEAP_ENABLED */

/* @} */
