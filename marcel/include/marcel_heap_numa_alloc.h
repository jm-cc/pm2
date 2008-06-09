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

/** \defgroup marcel_heap Heap Allocator Interface
 *
 * This is the interface for memory management
 *
 * @{
 */

#section common
#ifdef MA__NUMA

//#ifndef __MAHEAP_NUMA_ALLOC_H
//#define __MAHEAP_NUMA_ALLOC_H

#include<stddef.h>

//#include "maheap_alloc.h"

//#section macros
//#define MARCEL_NBMAXNODES 512

#section structures
/** weights of pages */
enum pinfo_weight {
        /** */
	HIGH_WEIGHT,
        /** */
	MEDIUM_WEIGHT,
        /** */
	LOW_WEIGHT
};

/** mapping policies over numa nodes */
enum mem_policy {
        /** */
	CYCLIC,
        /** */
	LESS_LOADED,
        /** */
	SMALL_ACCESSED
};

/** structure used to describe the mapping of a set of pages over numa bloc */
struct pageinfo {
//        /** set to one when the structure is newly created */
//	int new_pinfo;				
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

#section types
typedef struct pageinfo ma_pinfo_t;

#section macros
//#define SIZE_OF_UL 8

/* Mark/Clear bits with given index */
//#define mark_touched(M,i)   (M[i / SIZE_OF_UL] |= 1 << (i % SIZE_OF_UL))
//#define cleartouched(M,i)  (M[i / SIZE_OF_UL] &= ~(1 << (i % SIZE_OF_UL)))
//#define page_is_touched(M,i) (M[i / SIZE_OF_UL] & (1 << (i % SIZE_OF_UL)))

#section functions
#ifdef LINUX_SYS

/**
 * Call ma_amalloc to allocate memory
 * bind heap if is not already binded
 */
void *ma_hmalloc(size_t size, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode,  ma_heap_t *heap);

/**
 * Realloc memory, call to ma_arealloc
 */
void *ma_hrealloc(void *ptr, size_t size);

/**
 * Call ma_acalloc to allocate memory
 * bind heap if is not already binded
 */
void *ma_hcalloc(size_t nmemb, size_t size, int policy, int weight, unsigned long *nodemask, unsigned long maxnode,  ma_heap_t *heap);

/**
 * free memory by calling ma_afree
 */
void ma_hfree(void *ptr);

/**
 * Attach memory to a heap
 * first a ma_hamalloc is done with a size of 0, then the created bloc is set with the mem_stat pointer and the stat_size of the bloc
 * Note that for the mbind to sucess it needs a static size aligned to page sizes 
 */
void ma_hattach_memory(void *ptr, size_t size, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode, ma_heap_t *heap);

/**
 * Detach a memory attached to a heap
 */
void ma_hdetach_memory(void *ptr, ma_heap_t *heap);

/**
 * Move touched  physical pages to numa paramter: mempolicy, weight and nodemask of each heap having the ppinfo numa parameter
 * Static memory attached to heap are also moved, however the mbind will only success if the static memory are aligned to pages
 */
int ma_hmove_memory(ma_pinfo_t *ppinfo, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode,  ma_heap_t *heap);

/**
 * Get pages information. Retrieve the numa information of (void*)ptr into ppinfo from heap
 * Works also if (void*)ptr is a static memory attached to heap
 */
void ma_hget_pinfo(void *ptr, ma_pinfo_t* ppinfo, ma_heap_t *heap);

/**
 * Get all pages information from heap
 * Iterate over the different numa information of a list of heap
 * To initiate the iteration ppinfo has to be null
 * ppinfo is allocated inside this method one time
 */
int ma_hnext_pinfo(ma_pinfo_t **ppinfo, ma_heap_t *heap);

/**
 * Check pages location 
 * Retrieve the number of pages touched for each numa nodes
 * Data are stored in ppinfo, and concerne each heap and static memory associated to numa information of ppinfo
 * To have this information the method call the syscall __NR_move_pages
 */
void ma_hupdate_memory_nodes(ma_pinfo_t *ppinfo, ma_heap_t *heap);

/**
 * Heap creation
 * Create a heap with a minimum size
 */
ma_heap_t* ma_hcreate_heap(void);

/**
 * Heap destruction
 * Delete a heap
 */
void ma_hdelete_heap(ma_heap_t *heap);

/**
 * merge two heaps
 */
void ma_hmerge_heap(ma_heap_t *hacc, ma_heap_t *h);

#endif /* LINUX_SYS */

//#endif
#section common
#endif

/* @} */
