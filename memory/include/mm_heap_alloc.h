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

/** \addtogroup memory_heap */
/* @{ */

#ifdef MM_HEAP_ENABLED

#ifdef LINUX_SYS

#ifndef MM_HEAP_ALLOC_H
#define MM_HEAP_ALLOC_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>

/**
 * Align (size_t)mem to a multiple of HMALLOC_ALIGNMENT
 */
size_t heap_memalign(size_t mem);

/* --- memory alignment --- */
#define HEAP_SIZE_SIZE_T (sizeof(size_t))
#define HEAP_HMALLOC_ALIGNMENT (2 * HEAP_SIZE_SIZE_T)

/* --- zeroing memory --- */
#define HEAP_MALLOC_ZERO(dest, nbytes)       memset(dest, (unsigned char)0, nbytes)
/* --- copying memory --- */
#define HEAP_MALLOC_COPY(dest, src, nbytes)       memcpy(dest, src, nbytes)

/* --- manage bitmap --- */
typedef unsigned int heap_binmap_t;
#define HEAP_BITS_IN_BYTES 8
#define HEAP_WORD_SIZE	(8*sizeof(unsigned long))
#define HEAP_BITS_IN_WORDS (HEAP_BITS_IN_BYTES*HEAP_WORD_SIZE)
#define HEAP_BINMAP_SIZE 1048576
#define HEAP_BINMAP_MAX_PAGES (HEAP_BITS_IN_WORDS*HEAP_BINMAP_SIZE)

/* Mark/Clear bits with given index */
#define heap_markbit(M,i)       (M[i / HEAP_BITS_IN_WORDS] |= 1 << (i % HEAP_BITS_IN_WORDS))
#define heap_clearbit(M,i)      (M[i / HEAP_BITS_IN_WORDS] &= ~(1 << (i % HEAP_BITS_IN_WORDS)))
#define heap_bit_is_marked(M,i) (M[i / HEAP_BITS_IN_WORDS] & (1 << (i % HEAP_BITS_IN_WORDS)))

/* Manage node mask */
static inline int heap_mask_equal(unsigned long *a, unsigned long sa, unsigned long *b, unsigned long sb) {
  unsigned long i;
  if (sa != sb) return 0;
  for (i = 0; i < sa/HEAP_WORD_SIZE; i++)
    if (a[i] != b[i])
      return 0;
  return 1;
}

static inline int heap_mask_isset(unsigned long *a, unsigned long sa, int node) {
  if ((unsigned int)node >= sa*8) return 0;
  return heap_bit_is_marked(a,node);
}

static inline void heap_mask_zero(unsigned long *a, unsigned long sa) {
  memset(a, 0, sa);
}

static inline void heap_mask_set(unsigned long *a, int node) {
  heap_markbit(a,node);
}

static inline void heap_mask_clr(unsigned long *a, int node) {
  heap_clearbit(a,node);
}

/* --- heap and used block structures --- */
/** used memory bloc definition inside heap*/
struct heap_ub {
  /** */
  size_t size;
  /** */
  size_t prev_free_size;
  /** */
  void *data;
  /** */
  size_t stat_size;
  /** */
  struct heap_heap *heap;
  /** */
  struct heap_ub *prev;
  /** */
  struct heap_ub *next;
};

typedef struct heap_ub heap_ub_t;
/** heap structure type */
typedef struct heap_heap heap_heap_t;
typedef struct heap_malloc_stats heap_amalloc_stat_t;

#define heap_set_bloc(b,s,ps,h,pv,nx) {\
    b->size = s;                  \
    b->prev_free_size = ps;       \
    b->heap = h;                  \
    b->prev = pv;                 \
    b->next = nx;                 \
    b->data = NULL;               \
    b->stat_size = 0;             \
}

/** heap definition */
struct heap_heap {
  /** */
  struct heap_ub *used;
  /** */
  marcel_spinlock_t lock_heap;
  /** */
  struct heap_heap *next_heap;
  /** */
  struct heap_heap *next_same_heap;
  /** */
  size_t free_size;
  /** */
  size_t used_size;
  /** */
  size_t touch_size;
  /** */
  size_t attached_size;
  /** */
  int nb_attach;
  /** */
  void* iterator;
  /** */
  int iterator_num;
  /** */
  int alloc_policy;
  /** */
  int mempolicy;
  /** */
  int weight;
  /** */
  unsigned long nodemask[MARCEL_NBMAXNODES];
  /** */
  unsigned long maxnode;
  /** */
  heap_binmap_t bitmap[1]; /* avoid to be set as NULL, need to be the last field of the struct */
};

/** malloc statistics */
struct heap_malloc_stats {
  /** */
  size_t free_size;
  /** */
  size_t used_size;
  /** */
  size_t touch_size;
  /** */
  size_t attached_size;
  /** */
  int npinfo;
};

#define HEAP_DYN_ALLOC 			0
#define HEAP_PAGE_ALLOC 		1
#define HEAP_MINIMUM_PAGES 		10
#define HEAP_UNSPECIFIED_POLICY 	-1
#define HEAP_UNSPECIFIED_WEIGHT 	-1
#define HEAP_ITERATOR_ID_UNDEFINED	-1

#define HEAP_SIZE_T 		(heap_memalign(sizeof(heap_heap_t)))
#define HEAP_BLOCK_SIZE_T 	(heap_memalign(sizeof(heap_ub_t)))

#define HEAP_IS_HEAP(h)			(h != NULL)
#define HEAP_IS_HEAP_POLICY(h,p)	(h != NULL && h->alloc_policy == p)
#define HEAP_ADD_FREE_SIZE(sz,h) 	{ h->free_size += sz; h->used_size -= sz; }
#define HEAP_ADD_USED_SIZE(sz,h) 	{ h->free_size -= sz; h->used_size += sz; }
#define HEAP_GET_SIZE(h) 		( h->free_size + h->used_size )

#define HEAP_NBPAGES 1048576

/**
 * Return an adress pointing to a heap of size (size_t)size
 * This method is different to heap_acreate in the way that physical pages
 * are mapped to the numa bloc following the numa parameter: mempolicy, weight and nodemask
 * Size is aligned to pagesize.
 */
heap_heap_t *heap_acreatenuma(size_t size, int flag_heap, int policy, int weight, unsigned long *nodemask, unsigned long maxnode);

/**
 * Return an adress pointing to a heap of size (size_t)size.
 * This method call mmap to reserve memory.
 * In case of mmap failed or the initialization of the mutex failed the returning adress is NULL.
 * Size is aligned to pagesize.
 */
heap_heap_t *heap_acreate(size_t size, int flag_heap);

/**
 * This method call munmap to give back  memory adresses of each heap of the list starting at (heap_heap_t**)heap.
 * If the mutex is already lock by another thread this method has an undefined behavior (mainly the methode do nothing).
 * If the munmap call failed, the method do nothing and heap is set to NULL.
 */
void heap_adelete(heap_heap_t **heap);

/**
 * Returns true if the heap has no malloc set or memory attached
 */
int heap_is_empty_heap(heap_heap_t* heap);

/**
 * Mbind a heap and set its numa data: mempolicy, weight and nodemask
 */
int heap_amaparea(heap_heap_t* heap, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode);

/**
 * Concat htgt at the end of hsrc
 * This method call an atomic cmp&swap method to link both lists
 */
void heap_aconcat_global_list(heap_heap_t *hsrc, heap_heap_t *htgt);

/**
 * Concat htgt at the end of hsrc for list of heaps with same numa inforamtion
 * This method call an atomic cmp&swap method to link both lists
 */
void heap_aconcat_local_list(heap_heap_t *hsrc, heap_heap_t *htgt);

/**
 * Return the heap of the list which has numa information: mempolicy, weight and nodemask
 * In case of heap is not found the call return null
 */
heap_heap_t* heap_aget_heap_from_list(int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode, heap_heap_t* heap);

/**
 * Allocate per pages in a heap.
 * TODO: If no enough contiguous page is found a new heap is created.
 * Allocated and free pages are managed by a bitmap vector.
 */
void *heap_apagealloc(int nb_pages, heap_heap_t *heap);

/**
 * Allocate memory into the (heap_heap_t*)heap memory area.
 * This allocation is structured by a double chained list of used bloc.
 * A used bloc is a structure including pointers to next and previous
 * bloc the current used size and the size, which it's free between this current
 * bloc and the previous bloc
 * This data structure are addressed inside the heap as follow:
 * <pre>
 *
 * 				|	heap			|
 * 				| ...(list of blocs)...		|
 * 				|	.......			|
 * 		new bloc ->	+-------------------------------+
 * 				| current size			|
 * 				+-------------------------------+
 * 				| previous size			|
 * 				+-------------------------------+
 * 				| adress of the current heap	|
 * 				+-------------------------------+
 *	 			| address of previous bloc	|
 *	 			+-------------------------------+
 *	 			| address of next bloc		|
 *	 			+-------------------------------+
 *	 			| address of static memory	|
 *	 			+-------------------------------+
 * 				| size of static memory		|
 * 	returned address -> 	+-------------------------------+
 * 				| user data ...			|
 * 				|				|
 * 				+-------------------------------+
 * 				|				|
 * 				|				|
 *
 * </pre>
 * The research of enough free space between blocs is done by a first-fit method.
 * If the heap has not enough memory to allocate the caller request , the method create a new heap
 * binded in cases of numa information are set
 * If the memory cannot ba allocated the returning adress to the caller is NULL.
 */
void *heap_amalloc(size_t size, heap_heap_t *heap);

/**
 * This method reserve a memory space of num_elts*elt_size in (void*)heap
 * The memory is set to zero
 */
void *heap_acalloc(size_t nmemb, size_t size, heap_heap_t* heap);

/**
 * heap_arealloc try to realloc a memory reserved area with a new size and return the adress of the new allocated memory.
 * If the new size is equal to zero, the heap_arealloc call heap_afree to remove the bloc from the list.
 * If the (void*)ptr memory reference is NULL, the heap_arealloc call heap_amalloc.
 * The reallocation try to find first to augment the reserved space of the bloc adressed by (void*)ptr.
 * If the space between its previous and next bloc is not enough, a new bloc is allocated.
 * Data are moved in case of the returning adress is different to (void*)ptr.
 */
void *heap_arealloc(void *ptr, size_t size, heap_heap_t* heap);

/**
 * Free the memory of (void*)ptr by calling heap_afree_heap.
 * Matching heap was stored in the used bloc referencing by (void*)ptr.
 */
void heap_afree(void* ptr);

/**
 * A call to heap_afree_heap consists to remove the used bloc pointed by (void*)ptr
 * from the list of used bloc.
 * In case of a next bloc follow the removed bloc,
 * previous free size of next bloc is increased by the free space of the removed bloc.
 */
void heap_afree_heap(void* ptr, heap_heap_t* heap);

/**
 * Free pages in a heap with an allocation per pages
 * Pages are set as free in the bitmap vector.
 */
void heap_apagefree(void* ptr, int nb_pages, heap_heap_t *heap);

/**
 * Return a structure with information of the usage of the heap:
 * - free memory remaining
 * - used memory
 * - touch memory
 * - attached memory size
 * - number of heaps
 */
heap_amalloc_stat_t heap_amallinfo(heap_heap_t* heap);

/**
 * Display the list of heap starting from (heap_heap_t*)heap
 */
void heap_print_list(const char* str, heap_heap_t* heap);

/**
 * Display the list of bloc starting from (heap_ub_t*)root
 */
void heap_print_heap(struct heap_ub* root);

#endif /* MM_HEAP_ALLOC_H */
#endif /* LINUX_SYS */
#endif /* MM_HEAP_ENABLED */

/* @} */
