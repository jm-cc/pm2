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

/** \addtogroup marcel_heap */
/* @{ */

//#ifndef __HEAP_ALLOC_H
//#define __HEAP_ALLOC_H

#section common
#ifdef MA__NUMA_MEMORY

#include<stddef.h>
#include<stdio.h>

#section functions
#ifdef LINUX_SYS
/**
 * Align (size_t)mem to a multiple of HMALLOC_ALIGNMENT
 */
size_t ma_memalign(size_t mem);
#endif /* LINUX_SYS */

#section macros
/* --- memory alignment --- */
#define SIZE_SIZE_T (sizeof(size_t))
#define HMALLOC_ALIGNMENT (2 * SIZE_SIZE_T)

/* --- zeroing memory --- */
#define MALLOC_ZERO(dest, nbytes)       memset(dest, (unsigned char)0, nbytes)
/* --- copying memory --- */
#define MALLOC_COPY(dest, src, nbytes)       memcpy(dest, src, nbytes)

/* --- manage bitmap --- */
typedef unsigned int binmap_t;
#define BITS_IN_BYTES 8
#define WORD_SIZE	(8*sizeof(unsigned long))
#define BITS_IN_WORDS (BITS_IN_BYTES*WORD_SIZE)
#define HEAP_BINMAP_SIZE 1048576
#define HEAP_BINMAP_MAX_PAGES (BITS_IN_WORDS*HEAP_BINMAP_SIZE)

/* Mark/Clear bits with given index */
#define markbit(M,i)       (M[i / BITS_IN_WORDS] |= 1 << (i % BITS_IN_WORDS))
#define clearbit(M,i)      (M[i / BITS_IN_WORDS] &= ~(1 << (i % BITS_IN_WORDS)))
#define bit_is_marked(M,i) (M[i / BITS_IN_WORDS] & (1 << (i % BITS_IN_WORDS)))

#section functions
#include<string.h>

#ifdef LINUX_SYS
/* Manage node mask */
static inline int mask_equal(unsigned long *a, unsigned long sa, unsigned long *b, unsigned long sb) {
        unsigned long i;
	if (sa != sb) return 0;
        for (i = 0; i < sa/WORD_SIZE; i++)
                if (a[i] != b[i])
                        return 0;
        return 1;
}

static inline int mask_isset(unsigned long *a, unsigned long sa, int node) {
	if ((unsigned int)node >= sa*8) return 0;
	return bit_is_marked(a,node);
}

static inline void mask_zero(unsigned long *a, unsigned long sa) {
        memset(a, 0, sa);
}

static inline void mask_set(unsigned long *a, int node) {
	markbit(a,node);
}

static inline void mask_clr(unsigned long *a, int node) {
	clearbit(a,node);
}

#endif /*LINUX_SYS */

#section structures
/* --- heap and used block structures --- */
/** used memory bloc definition inside heap*/
struct ub {
	/** */
	size_t size;
	/** */
	size_t prev_free_size;
	/** */
	void *data;
	/** */
	size_t stat_size;
	/** */
	struct heap *heap;
	/** */
	struct ub *prev;
	/** */
	struct ub *next;
};

#section types
typedef struct ub ma_ub_t;
/** heap structure type */
typedef struct heap ma_heap_t;
typedef struct malloc_stats ma_amalloc_stat_t;

#section macros
#define set_bloc(b,s,ps,h,pv,nx) {\
	b->size = s; \
	b->prev_free_size = ps; \
	b->heap = h; \
	b->prev = pv; \
	b->next = nx; \
	b->data = NULL; \
	b->stat_size = 0; \
}

#section structures
#depend "marcel_mutex.h[structures]"
/** heap definition */
struct heap {
        /** */
	struct ub *used;
        /** */
	ma_spinlock_t lock_heap;
        /** */
	struct heap *next_heap;
        /** */
	struct heap *next_same_heap;
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
	binmap_t bitmap[1]; /* avoid to be set as NULL, need to be the last field of the struct */
		//int pages[MA_HEAP_NBPAGES];
};

/** malloc statistics */
struct malloc_stats {
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

#section macros
#define HEAP_DYN_ALLOC 	0
#define HEAP_PAGE_ALLOC 1
#define HEAP_MINIMUM_PAGES 10
#define HEAP_UNSPECIFIED_POLICY -1
#define HEAP_UNSPECIFIED_WEIGHT -1
#define HEAP_ITERATOR_ID_UNDEFINED -1

#ifdef LINUX_SYS
#define HEAP_SIZE_T 	(ma_memalign(sizeof(ma_heap_t)))
#define BLOCK_SIZE_T 	(ma_memalign(sizeof(ma_ub_t)))
#endif /* LINUX_SYS */

#define IS_HEAP(h)			(h != NULL)
#define IS_HEAP_POLICY(h,p)		(h != NULL && h->alloc_policy == p)
#define HEAP_ADD_FREE_SIZE(sz,h) 	{ h->free_size += sz; h->used_size -= sz; }
#define HEAP_ADD_USED_SIZE(sz,h) 	{ h->free_size -= sz; h->used_size += sz; }
#define HEAP_GET_SIZE(h) 		( h->free_size + h->used_size )

#define MA_HEAP_NBPAGES 1048576

#section functions
#ifdef LINUX_SYS
/**
 * Return an adress pointing to a heap of size (size_t)size
 * This method is different to ma_acreate in the way that physical pages
 * are mapped to the numa bloc following the numa parameter: mempolicy, weight and nodemask
 * Size is aligned to pagesize.
 */
ma_heap_t *ma_acreatenuma(size_t size, int flag_heap, int policy, int weight, unsigned long *nodemask, unsigned long maxnode);

/**
 * Return an adress pointing to a heap of size (size_t)size.
 * This method call mmap to reserve memory.
 * In case of mmap failed or the initialization of the mutex failed the returning adress is NULL.
 * Size is aligned to pagesize.
 */
ma_heap_t *ma_acreate(size_t size, int flag_heap);

/**
 * This method call munmap to give back  memory adresses of each heap of the list starting at (ma_heap_t**)heap.
 * If the mutex is already lock by another thread this method has an undefined behavior (mainly the methode do nothing).
 * If the munmap call failed, the method do nothing and heap is set to NULL.
 */
void ma_adelete(ma_heap_t **heap);

/**
 * Returns true if the heap has no malloc set or memory attached
 */
int ma_is_empty_heap(ma_heap_t* heap);

/**
 * Mbind a heap and set its numa data: mempolicy, weight and nodemask
 */
int ma_amaparea(ma_heap_t* heap, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode);

//static inline unsigned long ma_at_cmpchg(volatile void *ptr, unsigned long old, unsigned long new, int size);

/**
 * Concat htgt at the end of hsrc
 * This method call an atomic cmp&swap method to link both lists
 */
void ma_aconcat_global_list(ma_heap_t *hsrc, ma_heap_t *htgt);

/**
 * Concat htgt at the end of hsrc for list of heaps with same numa inforamtion
 * This method call an atomic cmp&swap method to link both lists
 */
void ma_aconcat_local_list(ma_heap_t *hsrc, ma_heap_t *htgt);

/**
 * Return the heap of the list which has numa information: mempolicy, weight and nodemask
 * In case of heap is not found the call return null
 */
ma_heap_t* ma_aget_heap_from_list(int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode, ma_heap_t* heap);

/**
 * Allocate per pages in a heap.
 * TODO: If no enough contiguous page is found a new heap is created.
 * Allocated and free pages are managed by a bitmap vector.
 */
void *ma_apagealloc(int nb_pages, ma_heap_t *heap);

/**
 * Allocate memory into the (ma_heap_t*)heap memory area.
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
void *ma_amalloc(size_t size, ma_heap_t *heap);

/**
 * This method reserve a memory space of num_elts*elt_size in (void*)heap
 * The memory is set to zero
 */
void *ma_acalloc(size_t nmemb, size_t size, ma_heap_t* heap);

/**
 * ma_arealloc try to realloc a memory reserved area with a new size and return the adress of the new allocated memory.
 * If the new size is equal to zero, the ma_arealloc call ma_afree to remove the bloc from the list.
 * If the (void*)ptr memory reference is NULL, the ma_arealloc call ma_amalloc.
 * The reallocation try to find first to augment the reserved space of the bloc adressed by (void*)ptr.
 * If the space between its previous and next bloc is not enough, a new bloc is allocated.
 * Data are moved in case of the returning adress is different to (void*)ptr.
 */
void *ma_arealloc(void *ptr, size_t size, ma_heap_t* heap);

/**
 * Free the memory of (void*)ptr by calling ma_afree_heap.
 * Matching heap was stored in the used bloc referencing by (void*)ptr.
 */
void  ma_afree(void* ptr);

/**
 * A call to ma_afree_heap consists to remove the used bloc pointed by (void*)ptr
 * from the list of used bloc.
 * In case of a next bloc follow the removed bloc,
 * previous free size of next bloc is increased by the free space of the removed bloc.
 */
void  ma_afree_heap(void* ptr, ma_heap_t* heap);

/**
 * Free pages in a heap with an allocation per pages
 * Pages are set as free in the bitmap vector.
 */
void  ma_apagefree(void* ptr, int nb_pages, ma_heap_t *heap);

/**
 * Return a structure with information of the usage of the heap:
 * - free memory remaining
 * - used memory
 * - touch memory
 * - attached memory size
 * - number of heaps
 */
ma_amalloc_stat_t  ma_amallinfo(ma_heap_t* heap);

/**
 * Display the list of heap starting from (ma_heap_t*)heap
 */
void ma_print_list(const char* str, ma_heap_t* heap);

/**
 * Display the list of bloc starting from (ma_ub_t*)root
 */
void ma_print_heap(struct ub* root);

/* --- debug functions --- */

#ifdef PM2DEBUG
#define mdebug_heap_list(str,root) ma_print_list(str,root)
#else
#define	mdebug_heap_list(...)
#endif

#endif /* LINUX_SYS */

#section common
#endif /* MA__NUMA_MEMORY */

/* @} */
