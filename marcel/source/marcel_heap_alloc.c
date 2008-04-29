/*
 * src/heap_alloc.c
 *
 * Definition of memory allocator inside a predefined heap
 * working for  NUMA architecture and multithread environements.
 * 
 * Author: Martinasso Maxime
 *
 * (C) Copyright 2007 INRIA 
 * Projet: MESCAL / ANR NUMASIS
 *
 */

#include <errno.h>
#include <stdarg.h>

/* used for atomic cmp&swap */
//#include <asm/atomic.h>
//#include <asm/system.h>

/* used for mmap */
#include <sys/mman.h>

#include "marcel.h"

#ifdef MA__NUMA

#ifdef LINUX_SYS
#include <numaif.h>
#include <numa.h>


#ifdef HEAP_DEBUG
/* --- print_list ---
 ** Display the list of heap starting from (ma_heap_t*)heap
 */
void ma_print_list(const char* str, ma_heap_t* heap) {
	ma_heap_t* h;
	int count = 0;
  	unsigned int i, max_node = (unsigned int) (marcel_nbnodes - 1) ;

	printf("%s",str);
	h = heap;
	while (IS_HEAP(h)) {
		ma_spin_lock(&h->lock_heap);
		printf("heap #%d %p (it=%d) (%d,%d) numa=",count,h,h->iterator_num,h->mempolicy,h->weight);
		for(i = 0; i < h->maxnode/WORD_SIZE; i++) {
			printf("%ld|",h->nodemask[i]);
		}
		for(i = 0; i < max_node+1; ++i) {
			if(mask_isset(h->nodemask,h->maxnode,i)) {
				printf("%d-",i);
			}
		}
		printf(" : next_same=%p|",h->next_same_heap);
		printf(" : next=%p|",h->next_heap);
		printf(" : %d : ",(int)HEAP_GET_SIZE(h));
		ma_print_heap(h->used);
		count++;
		ma_spin_unlock(&h->lock_heap);
		h = h->next_heap;
	}
}
/* --- print heap ---
 * Display the list of bloc starting from (ma_ub_t*)root
 */
void ma_print_heap(struct ub* root) {
	int count = 0;
	while(root != NULL) {
		printf("[%d| %p (u:%zu,f:%zu,s:%zu) (p:%p,n:%p)",count, root, root->size, root->prev_free_size, root->stat_size, root->prev, root->next);
		if (root->data != NULL) {
			printf("S:%p",root->data);
		} 
		printf("]");
		root = root->next;
		count++;
	}
	if (count != 0) {
		printf("\n");
	} else {
		printf("Empty\n");
	}	
}
#endif /* HEAP_DEBUG */

/* --- memalign ---
 * Align (size_t)mem to a multiple of HMALLOC_ALIGNMENT
 */
size_t ma_memalign(size_t mem) {
	unsigned long nb;
	if (mem == 0) return 0;
	if (mem < HMALLOC_ALIGNMENT) {
		return HMALLOC_ALIGNMENT;
	}
	/* look if is not already a multiple of HMALLOC_ALIGNMENT */
	if (mem % HMALLOC_ALIGNMENT == 0) {
		return mem;
	}
	nb = mem/HMALLOC_ALIGNMENT;
    	return (nb+1) * HMALLOC_ALIGNMENT;
}

/* --- ma_at_cmpchg
 * Atomic compare and swap
 */
#define ma_at_cmpchg(p,o,n,s) pm2_compareexchange(p,o,n,s)
#if 0
static inline unsigned long ma_at_cmpchg(volatile void *ptr, unsigned long old,unsigned long new, int size) {
	unsigned long prev;
	switch (size) {
	case 1:
		__asm__ __volatile__(LOCK_PREFIX "cmpxchgb %b1,%2"
				     : "=a"(prev)
				     : "q"(new), "m"(*__xg(ptr)), "0"(old)
				     : "memory");
		return prev;
	case 2:
		__asm__ __volatile__(LOCK_PREFIX "cmpxchgw %w1,%2"
				     : "=a"(prev)
				     : "r"(new), "m"(*__xg(ptr)), "0"(old)
				     : "memory");
		return prev;
	case 4:
		__asm__ __volatile__(LOCK_PREFIX "cmpxchgl %k1,%2"
				     : "=a"(prev)
				     : "r"(new), "m"(*__xg(ptr)), "0"(old)
				     : "memory");
		return prev;
	case 8:
		__asm__ __volatile__(LOCK_PREFIX "cmpxchgq %1,%2"
				     : "=a"(prev)
				     : "r"(new), "m"(*__xg(ptr)), "0"(old)
				     : "memory");
		return prev;
	}
	return old;
}
#endif

/* --- ma_amaparea ---
 * Mbind a heap and set its numa data: mempolicy, weight and nodemask
 */
int ma_amaparea(ma_heap_t* heap, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode) {
	int err;
	int i;
	//fprintf(stderr,"ma_amaparea nodemask %ld maxnode %ld thread %p\n", *nodemask, maxnode, MARCEL_SELF);
	err = ma_maparea(heap,HEAP_GET_SIZE(heap),mempolicy, nodemask, maxnode);
	if (err == 0) {
		heap->mempolicy = mempolicy;
		heap->weight = weight;
		heap-> maxnode = maxnode;
		for (i = 0; i < heap->maxnode/WORD_SIZE; ++i) {		
			heap->nodemask[i] = nodemask[i];
		}	
	}
	return err;
}


/* --- ma_acreate ---
 * Return an adress pointing to a heap of size (size_t)size.
 * This method call mmap to reserve memory.
 * In case of mmap failed or the initialization of the mutex failed the returning adress is NULL.
 * Size is aligned to pagesize.
 */
ma_heap_t* ma_acreate(size_t size, int alloc_policy) {
	ma_heap_t* heap;	
	unsigned int pagesize, i; //nb_pages;

	pagesize = getpagesize () ;
	if (size < HEAP_MINIMUM_PAGES*pagesize) {
		size = HEAP_MINIMUM_PAGES*pagesize;
	} else {
		size = ma_memalign(size);
	}

	/* align to pagesize */
  	if ((size % pagesize) != 0) {
		size = ((size / pagesize)+1)*pagesize ;
	}
	if (alloc_policy == HEAP_PAGE_ALLOC) {
		size += pagesize;
	}

	heap = (ma_heap_t *) mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	if (heap == MAP_FAILED) {
		heap = NULL;
		perror("mmap failed in acreate\n");
		DEBUG_PRINT("ma_acreate: mmap failed\n");
	} else {
		heap->mempolicy = HEAP_UNSPECIFIED_POLICY;
		heap->weight = HEAP_UNSPECIFIED_WEIGHT;
		heap->iterator = NULL;
		heap->iterator_num = HEAP_ITERATOR_ID_UNDEFINED;
		heap->nb_attach = 0;
		heap->maxnode = 0;
		heap->used = NULL;
		heap->next_heap = NULL;
		heap->next_same_heap = NULL;
		heap->attached_size = 0;
		heap->alloc_policy = alloc_policy;
		if (alloc_policy == HEAP_DYN_ALLOC) {
			heap->free_size = size - HEAP_SIZE_T;
			heap->used_size = HEAP_SIZE_T;
			heap->touch_size = HEAP_SIZE_T;
		} else {
			heap->free_size = size - pagesize;
			heap->used_size = pagesize;
			heap->touch_size = pagesize;
			for(i = 0; i < HEAP_BINMAP_SIZE; ++i) {
				clearbit(heap->bitmap,i);
			}
		}
		
		//for(i = 0; i < MA_HEAP_NBPAGES; ++i) {
		//	heap->pages[i] = 0;
		//}

		ma_spin_lock_init(&heap->lock_heap);

	/* 	if (ma_spin_lock_init(&heap->lock_heap) != 0) { */
/* 			heap = NULL; */
/* 			perror("Initilization of the mutex failed in acreate\n"); */
/* 			DEBUG_PRINT("ma_acreate: mutex_init failed\n"); */
/* 		} */
	}
	DEBUG_PRINT("ma_acreate: heap created at %p size=%ld\n",heap,size);
	return heap;
}

/* --- acreatenuma ---
 * Return an adress pointing to a heap of size (size_t)size
 * This method is different to ma_acreate in the way that physical pages
 * are mapped to the numa bloc following the numa parameter: mempolicy, weight and nodemask
 * Size is aligned to pagesize.
 */
ma_heap_t* ma_acreatenuma(size_t size, int alloc_policy, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode) {
	ma_heap_t *heap;

	heap = ma_acreate(size,alloc_policy);
	if (mempolicy != HEAP_UNSPECIFIED_POLICY && weight != HEAP_UNSPECIFIED_WEIGHT && maxnode != 0) {
		ma_spin_lock(&heap->lock_heap);
		ma_amaparea(heap, mempolicy, weight, nodemask, maxnode);
		ma_spin_unlock(&heap->lock_heap);
	}
	return heap;
}

/* --- ma_adelete --- 
 * This method call munmap to give back  memory adresses of each heap of the list starting at (ma_heap_t**)heap.
 * If the mutex is already lock by another thread this method has an undefined behavior (mainly the methode do nothing).
 * If the munmap call failed, the method do nothing and heap is set to NULL.
 */
void ma_adelete(ma_heap_t **heap) {
	size_t size_heap;
	int failed = 0;
	ma_heap_t* next_heap;
	ma_heap_t* next_heap_tmp;

	next_heap = *heap;
	if (IS_HEAP(next_heap)) {
		while(next_heap != NULL) {
			ma_spin_lock(&next_heap->lock_heap);
			size_heap = HEAP_GET_SIZE(next_heap);
			next_heap_tmp = next_heap->next_heap;
			ma_spin_unlock(&next_heap->lock_heap);
			//if (marcel_mutex_destroy(&next_heap->lock_heap) == 0) {
				/* if mutex destroy success mainly no other thread has locked the mutex */
			if (munmap(next_heap,size_heap) != 0) {
				DEBUG_PRINT("ma_adelete: freearea error addr=%p size=%d\n",next_heap, (int)size_heap);
			}
			DEBUG_PRINT("ma_adelete: succeed\n");
				//} else {
				/* may destroy an locked mutex: undefined behavior */
				//DEBUG_PRINT("ma_adelete: may destroy a locked mutex: undefined behavior\n");
				//failed = 1;
				//}
			next_heap = next_heap_tmp;
		}
		if (!failed) *heap = NULL;
	}
}

/* --- ma_is_empty_heap ---
 * Returns true if the heap has no malloc set or memory attached
 */
int ma_is_empty_heap(ma_heap_t* heap) {
	ma_heap_t* current_heap;

	current_heap = heap;
	while(IS_HEAP(current_heap)) {
		if (current_heap->used != NULL) {
			return 0;
		}
		current_heap = current_heap->next_heap;
	}
	return 1;
}

/* --- ma_aconcat_global_list ---
 * Concat htgt at the end of hsrc
 * This method call an atomic cmp&swap method to link both lists
 */
void ma_aconcat_global_list(ma_heap_t *hsrc, ma_heap_t *htgt) {
	ma_heap_t *current_heap;
	int valid = 0;

	if (IS_HEAP(hsrc) && IS_HEAP(htgt) && hsrc != htgt) {
		current_heap = hsrc;
		while(!valid) {
			while(IS_HEAP(current_heap->next_heap)) {
				current_heap = current_heap->next_heap;
			}
			if(ma_at_cmpchg((volatile void*)&current_heap->next_heap, (unsigned long)NULL, (unsigned long)htgt,sizeof(*(&current_heap->next_heap))) == (int)0) {
				valid = 1;
				DEBUG_PRINT("ma_aconcat_global link %p with %p\n",hsrc,htgt);
				//DEBUG_LIST("ma_aconcat_global\n",hsrc);
			}
		}
	}
}

/* --- ma_aconcat_local_list ---
 * Concat htgt at the end of hsrc for list of heaps with same numa inforamtion
 * This method call an atomic cmp&swap method to link both lists
 */
void ma_aconcat_local_list(ma_heap_t *hsrc, ma_heap_t *htgt) {
	ma_heap_t *current_heap;
	int valid = 0;

	if (IS_HEAP(hsrc) && IS_HEAP(htgt) && hsrc != htgt) {
		current_heap = hsrc;
		while(!valid) {
			while(IS_HEAP(current_heap->next_same_heap)) {
				current_heap = current_heap->next_same_heap;
			}
			if(ma_at_cmpchg((volatile void*)&current_heap->next_same_heap, (unsigned long)NULL, (unsigned long)htgt,sizeof(*(&current_heap->next_same_heap))) == (int)0) {
				valid = 1;
				DEBUG_PRINT("ma_aconcat_local link %p with %p\n",hsrc,htgt);
				//DEBUG_LIST("ma_aconcat_local\n",hsrc);
			}
		}
	}
}

/* --- ma_amalloc --- 
 * Allocate memory into the (ma_heap_t*)heap memory area.
 * This allocation is structured by a double chained list of used bloc.
 * A used bloc is a structure including pointers to next and previous
 * bloc the current used size and the size, which it's free between this current
 * bloc and the previous bloc
 * This data structure are addressed inside the heap as follow:
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
 * The research of enough free space between blocs is done by a first-fit method.
 * If the heap has not enough memory to allocate the caller request , the method create a new heap
 * binded in cases of numa information are set
 * If the memory cannot ba allocated the returning adress to the caller is NULL.
 */
void *ma_amalloc(size_t size, ma_heap_t* heap) {
	ma_ub_t *current_bloc_used, *current_bloc_used_prev, *temp_bloc_used;
	ma_heap_t *temp_heap, *next_same_heap, *prev_next_same_heap;
	void *ptr;

	size = ma_memalign(size);

	DEBUG_PRINT("ma_amalloc size=%d ",(int)size);
	if (!IS_HEAP_POLICY(heap,HEAP_DYN_ALLOC)) {
		DEBUG_PRINT("bad heap\n");
		return NULL;	
	}
	next_same_heap = prev_next_same_heap = heap;
	/* check for heap with enough remaining free space */
	while(IS_HEAP(next_same_heap)) {
		ma_spin_lock(&next_same_heap->lock_heap);
		if (next_same_heap->free_size > size + BLOCK_SIZE_T) {
			ma_spin_unlock(&next_same_heap->lock_heap);
			break;
		}
		ma_spin_unlock(&next_same_heap->lock_heap);
		prev_next_same_heap = next_same_heap;
		next_same_heap = next_same_heap->next_same_heap;
	}
	if (!IS_HEAP(next_same_heap)) {
		/* not enough space found in heap list */
		/* create a new heap */
		DEBUG_PRINT("create new heap from %p\n",prev_next_same_heap);
		//fprintf(stderr,"ma_amalloc mempolicy %d, nodemask %ld, maxnode %ld, thread %p\n", heap->mempolicy, *heap->nodemask, heap->maxnode, MARCEL_SELF);
		temp_heap = ma_acreatenuma(2*size, HEAP_DYN_ALLOC, heap->mempolicy, heap->weight, heap->nodemask, heap->maxnode);
		/* link heap */
		ma_aconcat_global_list(heap,temp_heap);
		ma_aconcat_local_list(prev_next_same_heap,temp_heap);
		return ma_amalloc(size,temp_heap);
	}
	/* enough space in heap */	
	while(IS_HEAP(next_same_heap)) {
		
		ma_spin_lock(&next_same_heap->lock_heap);

		if (next_same_heap->used == NULL) { /* first used bloc in list of current heap */
			DEBUG_PRINT("first bloc of list\n");
			next_same_heap->used = (ma_ub_t*)((char*)next_same_heap + HEAP_SIZE_T);
		
			set_bloc(next_same_heap->used, size, 0, next_same_heap, NULL, NULL);
		
			HEAP_ADD_USED_SIZE(size+BLOCK_SIZE_T,next_same_heap);
			next_same_heap->touch_size = size + BLOCK_SIZE_T;
	
			ma_spin_unlock(&next_same_heap->lock_heap);
			DEBUG_LIST("->",next_same_heap);
			return (void*)((char*)(next_same_heap->used) + BLOCK_SIZE_T);
		}
	
		
		/* find first free block: first fit */
		current_bloc_used_prev = current_bloc_used = next_same_heap->used;
		while(current_bloc_used != NULL && current_bloc_used->prev_free_size < size + BLOCK_SIZE_T) {
			current_bloc_used_prev = current_bloc_used;
			current_bloc_used = current_bloc_used->next;
		}
		//DEBUG_PRINT(" %p %p ok\n",current_bloc_used_prev,current_bloc_used);

		if (current_bloc_used == NULL) { /* not enough free space between used bloc */
			DEBUG_PRINT("no free space between used bloc found ");
		
			/* goes to offset of the last used bloc */
			current_bloc_used = current_bloc_used_prev;

			/* check for enough remaining contigous free space */
			if ( (char*)next_same_heap+HEAP_GET_SIZE(next_same_heap) - ((char*)current_bloc_used+BLOCK_SIZE_T+current_bloc_used->size) < size + BLOCK_SIZE_T) { 
				/* goes to next heap */
				ma_spin_unlock(&next_same_heap->lock_heap);
				if (IS_HEAP(next_same_heap->next_same_heap)) {
					next_same_heap = next_same_heap->next_same_heap;
					continue; 
				} else {
					/* no bloc was found in each heap and no heap has enough contiguous space remaining */
					/* we need to create a new heap */
					DEBUG_PRINT("create new heap from %p\n",next_same_heap);
					temp_heap = ma_acreatenuma(2*size, HEAP_DYN_ALLOC, heap->mempolicy, heap->weight, heap->nodemask, heap->maxnode);
					ma_aconcat_global_list(heap,temp_heap);
					ma_aconcat_local_list(next_same_heap,temp_heap);
					return ma_amalloc(size,temp_heap);
				}
			}

			/* create new used bloc */
			DEBUG_PRINT("heap found\n");
			current_bloc_used->next = (ma_ub_t*)((char*)current_bloc_used+BLOCK_SIZE_T+current_bloc_used->size);
			set_bloc(current_bloc_used->next, size, 0, next_same_heap, current_bloc_used, NULL);
			next_same_heap->touch_size = (unsigned long)current_bloc_used->next + size + BLOCK_SIZE_T - (unsigned long)next_same_heap;
			HEAP_ADD_USED_SIZE(size+BLOCK_SIZE_T,next_same_heap);
		
			ptr = (char*)(current_bloc_used->next) + BLOCK_SIZE_T;
	
		} else { /* free space found between block */
			DEBUG_PRINT("free space between used bloc found\n");
		
			if(current_bloc_used->prev != NULL) {
				/* goes to offset of the last used bloc */
				temp_bloc_used = current_bloc_used;
				current_bloc_used = (ma_ub_t*)((char*)current_bloc_used_prev+BLOCK_SIZE_T+current_bloc_used_prev->size);
				set_bloc(current_bloc_used, size, 0, next_same_heap, current_bloc_used_prev, temp_bloc_used);
				ptr = (char*)(current_bloc_used) + BLOCK_SIZE_T;
				current_bloc_used_prev->next = current_bloc_used;
				if (current_bloc_used->next != NULL) {
					current_bloc_used->next->prev_free_size -= (size + BLOCK_SIZE_T);
					current_bloc_used->next->prev = current_bloc_used;
				}
			} else {
				/* used bloc at begining */
				temp_bloc_used = current_bloc_used;
				current_bloc_used = (ma_ub_t*)((char *)next_same_heap+HEAP_SIZE_T);
				next_same_heap->used = current_bloc_used;
				set_bloc(current_bloc_used,size, 0, next_same_heap, NULL, temp_bloc_used);
				ptr = (char*)(current_bloc_used) + BLOCK_SIZE_T;
				if (current_bloc_used->next != NULL) {
					current_bloc_used->next->prev_free_size -= (size + BLOCK_SIZE_T);
					current_bloc_used->next->prev = current_bloc_used;
				}
			}
			HEAP_ADD_USED_SIZE(size+BLOCK_SIZE_T,next_same_heap);
		}
		ma_spin_unlock(&next_same_heap->lock_heap);
		return ptr;
	}
	return NULL;
}

/* --- ma_acalloc ---
 * This method reserve a memory space of num_elts*elt_size in (void*)heap
 * The memory is set to zero
 */
void *ma_acalloc(size_t nmemb, size_t size, ma_heap_t* heap) {
	void* ptr;
	size_t local_size = ma_memalign(nmemb*size);

	/* amalloc */
	DEBUG_PRINT("ma_acalloc: call to amalloc\n");
	ptr = ma_amalloc(local_size,heap);
	if (ptr == NULL) {
		DEBUG_PRINT("ma_acalloc: call to ma_amalloc failed\n");
		return NULL;
	}
	/* zeroing memory: only memory area specified */
	MALLOC_ZERO(ptr,local_size);	
	
	return ptr;
}

/* --- ma_arealloc ---
 * ma_arealloc try to realloc a memory reserved area with a new size and return the adress of the new allocated memory.
 * If the new size is equal to zero, the ma_arealloc call ma_afree to remove the bloc from the list.
 * If the (void*)ptr memory reference is NULL, the ma_arealloc call ma_amalloc.
 * The reallocation try to find first to augment the reserved space of the bloc adressed by (void*)ptr.
 * If the space between its previous and next bloc is not enough, a new bloc is allocated.
 * Data are moved in case of the returning adress is different to (void*)ptr.
 */
void *ma_arealloc(void *ptr, size_t size, ma_heap_t* heap) {
	ma_ub_t *current_bloc_used;
	//ma_ub_t *current_bloc_used_prev;
	//ma_ub_t *temp_bloc_used;
	void* new_ptr;
	
	size = ma_memalign(size);

	if (!IS_HEAP_POLICY(heap,HEAP_DYN_ALLOC)) {
		DEBUG_PRINT("ma_arealloc: bad heap\n");
		return NULL;	
	}

	if (ptr == NULL) { /* call amalloc */
		ptr = ma_amalloc(size,heap);
		if (ptr == NULL) {
			DEBUG_PRINT("ma_arealloc: call to amalloc failed\n");
			return NULL;
		}
		return ptr;
	}
	if (size == 0) { /* afree ptr */
		DEBUG_PRINT("ma_arealloc: size = 0 free memory\n");
		ma_afree_heap(ptr,heap);
		return NULL;
	}

	ma_spin_lock(&heap->lock_heap);
	
	current_bloc_used = (ma_ub_t *)((char*)ptr-BLOCK_SIZE_T);
	if (current_bloc_used->heap == heap) {
		if (current_bloc_used->size == size) { /* do nothing */
			DEBUG_PRINT("ma_arealloc: nothing to do\n");
			ma_spin_unlock(&heap->lock_heap);
			return ptr;
		}

		if (current_bloc_used->size > size) { /* reduce bloc size */
			DEBUG_PRINT("ma_arealloc: reduce bloc\n");
			if (current_bloc_used->next != NULL) {
				current_bloc_used->next->prev_free_size += current_bloc_used->size - size;
			}
			current_bloc_used->size = size;
			HEAP_ADD_FREE_SIZE(current_bloc_used->size - size,heap);
			ma_spin_unlock(&heap->lock_heap);
			return ptr;
		}	
	
		/* size area need to be increased */
		/* first check enough space after bloc then if not call amalloc, move data and free ptr */ 

		if (current_bloc_used->next != NULL && current_bloc_used->next->prev_free_size >= size - current_bloc_used->size) {
			/* enough space after bloc and not last bloc */ 
			DEBUG_PRINT("ma_arealloc: enough space after bloc not last bloc\n");
			current_bloc_used->next->prev_free_size -= (size - current_bloc_used->size); 
			HEAP_ADD_USED_SIZE(size - current_bloc_used->size,heap);
			current_bloc_used->size = size;
			ma_spin_unlock(&heap->lock_heap);
			return ptr;
		}
		if (current_bloc_used->next == NULL  && (char*)heap+HEAP_SIZE_T+HEAP_GET_SIZE(heap) > (char*)current_bloc_used+BLOCK_SIZE_T+size) {
			/* last block and enough remaining space */
			DEBUG_PRINT("ma_arealloc: enough space after bloc and last bloc\n");
			HEAP_ADD_USED_SIZE(size - current_bloc_used->size,heap);
			current_bloc_used->size = size;
			ma_spin_unlock(&heap->lock_heap);
			return ptr;
		}	
	}

	/* make a malloc */
	ma_spin_unlock(&heap->lock_heap);
	new_ptr = ma_amalloc(size,heap);
	if (new_ptr != NULL) {
		/* move data */
		MALLOC_COPY(new_ptr,ptr,size);	
		/* make a free */
		ma_afree_heap(ptr,heap);
		return new_ptr;
	}
	return ptr;
}

/* --- ma_apagealloc ---
 * Allocate per pages in a heap.
 * TODO: If no enough contiguous page is found a new heap is created.
 * Allocated and free pages are managed by a bitmap vector.
 */
void *ma_apagealloc(int nb_pages, ma_heap_t *heap) {
	unsigned int c, i;
	unsigned int pagesize = getpagesize () ;
	void* ptr;

	if (!IS_HEAP_POLICY(heap,HEAP_PAGE_ALLOC)) {
		DEBUG_PRINT("ma_apagealloc: bad heap\n");
		return NULL;	
	}
	ma_spin_lock(&heap->lock_heap);
	c = 0;
	i = 1;
	while(i < HEAP_BINMAP_MAX_PAGES) {
		if (bit_is_marked(heap->bitmap,i)) {
			c = 0;
		} else {
			c++;
		}
		i++;
		if (c == nb_pages) break;
	}
	DEBUG_PRINT("i = %d c = %d bitmap=%d max=%d\n",i,c,(int)heap->bitmap,(int)HEAP_BINMAP_MAX_PAGES);
	if (c < nb_pages) {
		/* TODO add a new heap */
		DEBUG_PRINT("ma_apagealloc: not enough pages available\n");
		ma_spin_unlock(&heap->lock_heap);
		return NULL;	
	}
	
	i -= nb_pages;
	ptr = (char*)heap + i * pagesize;

	HEAP_ADD_USED_SIZE(nb_pages*pagesize,heap);
	if (heap->touch_size/pagesize < i+nb_pages) { 
		heap->touch_size =  (char*)ptr+nb_pages*pagesize-(char*)heap;
	}
	for(c = i;c < i + nb_pages; c++) {
		markbit(heap->bitmap,c);
	}

	ma_spin_unlock(&heap->lock_heap);
	return ptr;

}


/* --- ma_apagefree ---
 * Free pages in a heap with an allocation per pages
 * Pages are set as free in the bitmap vector.
 */
void ma_apagefree(void* ptr, int nb_pages, ma_heap_t *heap) {
	unsigned int i,c;
	unsigned int pagesize;

	if (ptr != NULL && IS_HEAP_POLICY(heap,HEAP_PAGE_ALLOC)) {
		ma_spin_lock(&heap->lock_heap);
		pagesize = getpagesize () ;
		i = ((unsigned long)ptr-(unsigned long)heap)/pagesize;
		DEBUG_PRINT("i = %d\n",i);
		for(c = i;c < i + nb_pages; c++) {
			clearbit(heap->bitmap,c);
		}
		HEAP_ADD_FREE_SIZE(nb_pages*pagesize,heap);
		ma_spin_unlock(&heap->lock_heap);
	}
}

/* --- ma_afree ---
 * Free the memory of (void*)ptr by calling ma_afree_heap.
 * Matching heap was stored in the used bloc referencing by (void*)ptr.
 */
void ma_afree(void *ptr) {
	ma_ub_t *current_bloc_used;
	if (ptr != NULL) {
		current_bloc_used = (ma_ub_t *)((char*)ptr-BLOCK_SIZE_T);
		DEBUG_PRINT("afree: heap=%p\n",current_bloc_used->heap);
		ma_afree_heap(ptr,current_bloc_used->heap);		
	}
}

/* --- ma_afree_heap --- 
 * A call to ma_afree_heap consists to remove the used bloc pointed by (void*)ptr
 * from the list of used bloc.
 * In case of a next bloc follow the removed bloc,
 * previous free size of next bloc is increased by the free space of the removed bloc.
 */
void ma_afree_heap(void *ptr, ma_heap_t* heap) {
	ma_ub_t *current_bloc_used;
	size_t size;
	
	if (ptr != NULL && IS_HEAP_POLICY(heap,HEAP_DYN_ALLOC)) {
		ma_spin_lock(&heap->lock_heap);
		current_bloc_used = (ma_ub_t *)((char*)ptr-BLOCK_SIZE_T);
		size = current_bloc_used->size;
		if (current_bloc_used->prev != NULL) {
			current_bloc_used->prev->next = current_bloc_used->next;
			if(current_bloc_used->next != NULL) {
				current_bloc_used->next->prev = current_bloc_used->prev;
				current_bloc_used->next->prev_free_size += current_bloc_used->size+BLOCK_SIZE_T+current_bloc_used->prev_free_size;
			}
		} else {
			heap->used = current_bloc_used->next;
			if(current_bloc_used->next != NULL) {
				current_bloc_used->next->prev_free_size += current_bloc_used->size+BLOCK_SIZE_T+current_bloc_used->prev_free_size;
				current_bloc_used->next->prev = NULL;
			}
		}
		HEAP_ADD_FREE_SIZE(size+BLOCK_SIZE_T,heap);
		ma_spin_unlock(&heap->lock_heap);
		DEBUG_LIST("ma_afree_heap\n",heap);
	} else {
		/* argument = NULL: undefined behavior */
		DEBUG_PRINT("ma_afree_heap: passing NULL as argument caused undefined behavior\n");
	}
}

/* --- ma_amallinfo --- 
 * Return a structure with information of the usage of the heap:
 * - free memory remaining
 * - used memory
 * - touch memory
 * - attached memory size
 * - number of heaps
 */
ma_amalloc_stat_t ma_amallinfo(ma_heap_t* heap) {
	ma_heap_t * next_heap;
	ma_amalloc_stat_t stats;
	stats.free_size = 0;
	stats.used_size = 0;
	stats.touch_size = 0;
	stats.attached_size = 0;
	stats.npinfo = 0;
	
	next_heap = heap;
	while(IS_HEAP(next_heap)) {
		ma_spin_lock(&next_heap->lock_heap);
		stats.free_size += next_heap->free_size;
		stats.used_size += next_heap->used_size;
		stats.touch_size += next_heap->touch_size;
		stats.attached_size += next_heap->attached_size;
		stats.npinfo++;
		ma_spin_unlock(&next_heap->lock_heap);
		next_heap = next_heap->next_heap;
	}
	DEBUG_LIST("ma_amallinfo:\n",heap);
	return stats;
}

/* --- ma_aget_heap_from_list ---
 * Return the heap of the list which has numa information: mempolicy, weight and nodemask
 * In case of heap is not found the call return null
 */
ma_heap_t* ma_aget_heap_from_list(int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode, ma_heap_t* heap) {
	ma_heap_t *current_heap;

	DEBUG_PRINT("ma_aget_heap_from_list from %p (%d,%d) numa=%ld ",heap,mempolicy,weight,*nodemask);
	current_heap = heap;
	while (IS_HEAP(current_heap)) {
		ma_spin_lock(&current_heap->lock_heap);
	//	DEBUG_PRINT("[check %d = %d && %d = %d && %d = %d]",*nodemask,*current_heap->nodemask,maxnode, current_heap->maxnode,mempolicy,current_heap->mempolicy,weight,current_heap->weight);
		if (mask_equal(nodemask,maxnode,current_heap->nodemask,current_heap->maxnode) && mempolicy == current_heap->mempolicy && weight == current_heap->weight) {
			ma_spin_unlock(&current_heap->lock_heap);
			DEBUG_PRINT(" found: %p\n",current_heap);
			return current_heap;
		}
		ma_spin_unlock(&current_heap->lock_heap);
		current_heap = current_heap->next_heap;
	}
	DEBUG_PRINT(" not found\n");
	return NULL;
}

#endif /* LINUX_SYS */
#endif
