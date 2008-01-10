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

//#ifndef __HEAP_ALLOC_H
//#define __HEAP_ALLOC_H

#section common
#ifdef MA__NUMA

#include<stddef.h>
#include<stdio.h>

#section functions
#ifdef LINUX_SYS
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
/* used memory bloc definition inside heap*/
struct ub {
	size_t size;
	size_t prev_free_size;
	void *data;
	size_t stat_size;
	struct heap *heap;
	struct ub *prev;
	struct ub *next;
};

#section types
typedef struct ub ma_ub_t;
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
/* heap definition */
#depend "marcel_mutex.h[structures]"
struct heap {
	struct ub *used;
	ma_spinlock_t lock_heap; 
	struct heap *next_heap;
	struct heap *next_same_heap;
	size_t free_size;
	size_t used_size;
	size_t touch_size;
	size_t attached_size;
	int nb_attach;
	void* iterator;
	int iterator_num;
	int alloc_policy;
	int mempolicy;
	int weight;
	unsigned long nodemask[MARCEL_NBMAXNODES];
	unsigned long maxnode;
	binmap_t bitmap[1]; /* avoid to be set as NULL, need to be the last field of the struct */
		//int pages[MA_HEAP_NBPAGES];
};

/* malloc statistics */
struct malloc_stats {
	size_t free_size;
	size_t used_size;
	size_t touch_size;
	size_t attached_size;
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
/* --- function prototypes --- */
ma_heap_t *ma_acreatenuma(size_t size, int flag_heap, int policy, int weight, unsigned long *nodemask, unsigned long maxnode);
ma_heap_t *ma_acreate(size_t size, int flag_heap);
void  ma_adelete(ma_heap_t **heap);
int ma_is_empty_heap(ma_heap_t* heap);
int ma_amaparea(ma_heap_t* heap, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode);

//static inline unsigned long ma_at_cmpchg(volatile void *ptr, unsigned long old, unsigned long new, int size);
void ma_aconcat_global_list(ma_heap_t *hsrc, ma_heap_t *htgt);
void ma_aconcat_local_list(ma_heap_t *hsrc, ma_heap_t *htgt);
ma_heap_t* ma_aget_heap_from_list(int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode, ma_heap_t* heap);

void *ma_apagealloc(int nb_pages, ma_heap_t *heap);
void *ma_amalloc(size_t size, ma_heap_t *heap);
void *ma_acalloc(size_t nmemb, size_t size, ma_heap_t* heap);
void *ma_arealloc(void *ptr, size_t size, ma_heap_t* heap);

void  ma_afree(void* ptr);
void  ma_afree_heap(void* ptr, ma_heap_t* heap);
void  ma_apagefree(void* ptr, int nb_pages, ma_heap_t *heap);

ma_amalloc_stat_t  ma_amallinfo(ma_heap_t* heap);

void ma_print_list(const char* str, ma_heap_t* heap);
void ma_print_heap(struct ub* root);

/* --- debug functions --- */

//#define HEAP_DEBUG

#ifdef HEAP_DEBUG

#define DEBUG_LIST(str,root) \
 		ma_print_list(str,root)
#else
#define	DEBUG_LIST(...) 
#endif

#ifdef HEAP_DEBUG
#define	DEBUG_PRINT(...) \
      fprintf(stderr,__VA_ARGS__)
#else
#define	DEBUG_PRINT(...) 
#endif

#endif /* LINUX_SYS */

#section common
#endif

