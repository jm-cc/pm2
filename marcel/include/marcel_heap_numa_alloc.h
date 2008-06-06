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
	HIGH_WEIGHT,
	MEDIUM_WEIGHT,
	LOW_WEIGHT
};

/** mapping policies over numa nodes */
enum mem_policy {
	CYCLIC,
	LESS_LOADED,
	SMALL_ACCESSED
};

/** structure used to describe the mapping of a set of pages over numa bloc */
struct pageinfo {
//	int new_pinfo;				/* set to one when the structure is newly created */
	int mempolicy;				/* mapping policy */
	int weight;				/* pages weights */
	unsigned long *nodemask;		/* numa nodes mask */
	unsigned long maxnode;			/* number of bits of mask */
	size_t size;
	int nb_touched[MARCEL_NBMAXNODES];	/* distribution of pages over numa nodes */
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

/** Memory allocators */
void *ma_hmalloc(size_t size, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode,  ma_heap_t *heap);
void *ma_hrealloc(void *ptr, size_t size);
void *ma_hcalloc(size_t nmemb, size_t size, int policy, int weight, unsigned long *nodemask, unsigned long maxnode,  ma_heap_t *heap);
void ma_hfree(void *ptr);

/** Attach and detach of memory area to heap */
void ma_hattach_memory(void *ptr, size_t size, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode, ma_heap_t *heap);
void ma_hdetach_memory(void *ptr, ma_heap_t *heap);

/** Memory migration */
int ma_hmove_memory(ma_pinfo_t *ppinfo, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode,  ma_heap_t *heap);

/** Get pages information */
void ma_hget_pinfo(void *ptr, ma_pinfo_t* ppinfo, ma_heap_t *heap);

/** Get all pages information from heap */
int ma_hnext_pinfo(ma_pinfo_t **ppinfo, ma_heap_t *heap);

/** Check pages location */
void ma_hupdate_memory_nodes(ma_pinfo_t *ppinfo, ma_heap_t *heap);

/** Heap creation */
ma_heap_t* ma_hcreate_heap(void);

/** Heap destruction */
void ma_hdelete_heap(ma_heap_t *heap);

/** merge two heaps */
void ma_hmerge_heap(ma_heap_t *hacc, ma_heap_t *h);

#endif /* LINUX_SYS */

//#endif
#section common
#endif

/* @} */
