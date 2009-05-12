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

#ifdef MM_HEAP_ENABLED

#ifdef LINUX_SYS

#define MARCEL_INTERNAL_INCLUDE /* Only needed for pm2_compareexchange */

#include <errno.h>
#include <stdarg.h>
#include <sys/mman.h>
#include "marcel.h"
#include "mm_heap_numa.h"
#include "mm_heap_alloc.h"
#include "mm_debug.h"

size_t heap_memalign(size_t mem) {
  unsigned long nb;
  if (mem == 0) return 0;
  if (mem < HEAP_HMALLOC_ALIGNMENT) {
    return HEAP_HMALLOC_ALIGNMENT;
  }
  /* look if is not already a multiple of HMALLOC_ALIGNMENT */
  if (mem % HEAP_HMALLOC_ALIGNMENT == 0) {
    return mem;
  }
  nb = mem/HEAP_HMALLOC_ALIGNMENT;
  return (nb+1) * HEAP_HMALLOC_ALIGNMENT;
}

/**
 * Atomic compare and swap
 */
#define heap_at_cmpchg(p,o,n,s) pm2_compareexchange(p,o,n,s)

heap_heap_t* heap_acreatenuma(size_t size, int alloc_policy, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode) {
  heap_heap_t *heap;

  heap = heap_acreate(size,alloc_policy);
  if (mempolicy != HEAP_UNSPECIFIED_POLICY && weight != HEAP_UNSPECIFIED_WEIGHT && maxnode != 0) {
    marcel_spin_lock(&heap->lock_heap);
    heap_amaparea(heap, mempolicy, weight, nodemask, maxnode);
    marcel_spin_unlock(&heap->lock_heap);
  }
  return heap;
}

heap_heap_t* heap_acreate(size_t size, int alloc_policy) {
  heap_heap_t* heap;
  unsigned int pagesize, i; //nb_pages;

  pagesize = getpagesize () ;
  if (size < HEAP_MINIMUM_PAGES*pagesize) {
    size = HEAP_MINIMUM_PAGES*pagesize;
  } else {
    size = heap_memalign(size);
  }

  /* align to pagesize */
  if ((size % pagesize) != 0) {
    size = ((size / pagesize)+1)*pagesize ;
  }
  if (alloc_policy == HEAP_PAGE_ALLOC) {
    size += pagesize;
  }

  heap = (heap_heap_t *) mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  if (heap == MAP_FAILED) {
    heap = NULL;
    perror("mmap failed in acreate\n");
    mdebug_memory("heap_acreate: mmap failed\n");
  }
  else {
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
    }
    else {
      heap->free_size = size - pagesize;
      heap->used_size = pagesize;
      heap->touch_size = pagesize;
      for(i = 0; i < HEAP_BINMAP_SIZE; ++i) {
        heap_clearbit(heap->bitmap,i);
      }
    }

    //for(i = 0; i < HEAP_HEAP_NBPAGES; ++i) {
    //	heap->pages[i] = 0;
    //}

    marcel_spin_init(&heap->lock_heap, MARCEL_PROCESS_SHARED);

    /* 	if (marcel_spin_lock_init(&heap->lock_heap) != 0) { */
    /* 			heap = NULL; */
    /* 			perror("Initilization of the mutex failed in acreate\n"); */
    /* 			mdebug_memory("heap_acreate: mutex_init failed\n"); */
    /* 		} */
  }
  mdebug_memory("heap_acreate: heap created at %p size=%zd\n",heap,size);
  return heap;
}

void heap_adelete(heap_heap_t **heap) {
  size_t size_heap;
  int failed = 0;
  heap_heap_t* next_heap;
  heap_heap_t* next_heap_tmp;

  next_heap = *heap;
  if (HEAP_IS_HEAP(next_heap)) {
    while(next_heap != NULL) {
      marcel_spin_lock(&next_heap->lock_heap);
      size_heap = HEAP_GET_SIZE(next_heap);
      next_heap_tmp = next_heap->next_heap;
      marcel_spin_unlock(&next_heap->lock_heap);
      //if (marcel_mutex_destroy(&next_heap->lock_heap) == 0) {
      /* if mutex destroy success mainly no other thread has locked the mutex */
      if (munmap(next_heap,size_heap) != 0) {
        mdebug_memory("heap_adelete: freearea error addr=%p size=%d\n",next_heap, (int)size_heap);
      }
      mdebug_memory("heap_adelete: succeed\n");
      //} else {
      /* may destroy an locked mutex: undefined behavior */
      //mdebug_memory("heap_adelete: may destroy a locked mutex: undefined behavior\n");
      //failed = 1;
      //}
      next_heap = next_heap_tmp;
    }
    if (!failed) *heap = NULL;
  }
}

int heap_is_empty_heap(heap_heap_t* heap) {
  heap_heap_t* current_heap;

  current_heap = heap;
  while(HEAP_IS_HEAP(current_heap)) {
    if (current_heap->used != NULL) {
      return 0;
    }
    current_heap = current_heap->next_heap;
  }
  return 1;
}

int heap_amaparea(heap_heap_t* heap, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode) {
  int err;
  int i;
  //fprintf(stderr,"heap_amaparea nodemask %ld maxnode %ld thread %p\n", *nodemask, maxnode, MARCEL_SELF);
  err = heap_maparea(heap,HEAP_GET_SIZE(heap),mempolicy, nodemask, maxnode);
  if (err == 0) {
    heap->mempolicy = mempolicy;
    heap->weight = weight;
    heap-> maxnode = maxnode;
    for (i = 0; i < heap->maxnode/HEAP_WORD_SIZE; ++i) {
      heap->nodemask[i] = nodemask[i];
    }
  }
  return err;
}

void heap_aconcat_global_list(heap_heap_t *hsrc, heap_heap_t *htgt) {
  heap_heap_t *current_heap;
  int valid = 0;

  if (HEAP_IS_HEAP(hsrc) && HEAP_IS_HEAP(htgt) && hsrc != htgt) {
    current_heap = hsrc;
    while(!valid) {
      while(HEAP_IS_HEAP(current_heap->next_heap)) {
        current_heap = current_heap->next_heap;
      }
      if(heap_at_cmpchg((volatile void*)&current_heap->next_heap, (unsigned long)NULL, (unsigned long)htgt,sizeof(*(&current_heap->next_heap))) == (int)0) {
        valid = 1;
        mdebug_memory("heap_aconcat_global link %p with %p\n",hsrc,htgt);
        //mdebug_memory_list("heap_aconcat_global\n",hsrc);
      }
    }
  }
}

void heap_aconcat_local_list(heap_heap_t *hsrc, heap_heap_t *htgt) {
  heap_heap_t *current_heap;
  int valid = 0;

  if (HEAP_IS_HEAP(hsrc) && HEAP_IS_HEAP(htgt) && hsrc != htgt) {
    current_heap = hsrc;
    while(!valid) {
      while(HEAP_IS_HEAP(current_heap->next_same_heap)) {
        current_heap = current_heap->next_same_heap;
      }
      if(heap_at_cmpchg((volatile void*)&current_heap->next_same_heap, (unsigned long)NULL, (unsigned long)htgt,sizeof(*(&current_heap->next_same_heap))) == (int)0) {
        valid = 1;
        mdebug_memory("heap_aconcat_local link %p with %p\n",hsrc,htgt);
        //mdebug_memory_list("heap_aconcat_local\n",hsrc);
      }
    }
  }
}

heap_heap_t* heap_aget_heap_from_list(int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode, heap_heap_t* heap) {
  heap_heap_t *current_heap;

  mdebug_memory("heap_aget_heap_from_list from %p (%d,%d) numa=%ld\n",heap,mempolicy,weight,*nodemask);
  current_heap = heap;
  while (HEAP_IS_HEAP(current_heap)) {
    marcel_spin_lock(&current_heap->lock_heap);
    //	mdebug_memory("[check %d = %d && %d = %d && %d = %d]",*nodemask,*current_heap->nodemask,maxnode, current_heap->maxnode,mempolicy,current_heap->mempolicy,weight,current_heap->weight);
    if (heap_mask_equal(nodemask,maxnode,current_heap->nodemask,current_heap->maxnode) && mempolicy == current_heap->mempolicy && weight == current_heap->weight) {
      marcel_spin_unlock(&current_heap->lock_heap);
      mdebug_memory(" found: %p\n",current_heap);
      return current_heap;
    }
    marcel_spin_unlock(&current_heap->lock_heap);
    current_heap = current_heap->next_heap;
  }
  mdebug_memory(" not found\n");
  return NULL;
}

void *heap_apagealloc(int nb_pages, heap_heap_t *heap) {
  unsigned int c, i;
  unsigned int pagesize = getpagesize () ;
  void* ptr;

  if (!HEAP_IS_HEAP_POLICY(heap,HEAP_PAGE_ALLOC)) {
    mdebug_memory("heap_apagealloc: bad heap\n");
    return NULL;
  }
  marcel_spin_lock(&heap->lock_heap);
  c = 0;
  i = 1;
  while(i < HEAP_BINMAP_MAX_PAGES) {
    if (heap_bit_is_marked(heap->bitmap,i)) {
      c = 0;
    }
    else {
      c++;
    }
    i++;
    if (c == nb_pages) break;
  }
  mdebug_memory("i = %d c = %d bitmap=%p max=%d\n",i,c,heap->bitmap,(int)HEAP_BINMAP_MAX_PAGES);
  if (c < nb_pages) {
    /* TODO add a new heap */
    mdebug_memory("heap_apagealloc: not enough pages available\n");
    marcel_spin_unlock(&heap->lock_heap);
    return NULL;
  }

  i -= nb_pages;
  ptr = (char*)heap + i * pagesize;

  HEAP_ADD_USED_SIZE(nb_pages*pagesize,heap);
  if (heap->touch_size/pagesize < i+nb_pages) {
    heap->touch_size =  (char*)ptr+nb_pages*pagesize-(char*)heap;
  }
  for(c = i;c < i + nb_pages; c++) {
    heap_markbit(heap->bitmap,c);
  }

  marcel_spin_unlock(&heap->lock_heap);
  return ptr;
}

void *heap_amalloc(size_t size, heap_heap_t* heap) {
  heap_ub_t *current_bloc_used, *current_bloc_used_prev, *temp_bloc_used;
  heap_heap_t *temp_heap, *next_same_heap, *prev_next_same_heap;
  void *ptr;

  size = heap_memalign(size);

  mdebug_memory("heap_amalloc size=%d\n",(int)size);
  if (!HEAP_IS_HEAP_POLICY(heap,HEAP_DYN_ALLOC)) {
    mdebug_memory("bad heap\n");
    return NULL;
  }
  next_same_heap = prev_next_same_heap = heap;
	/* check for heap with enough remaining free space */
  while(HEAP_IS_HEAP(next_same_heap)) {
    marcel_spin_lock(&next_same_heap->lock_heap);
    if (next_same_heap->free_size > size + HEAP_BLOCK_SIZE_T) {
      marcel_spin_unlock(&next_same_heap->lock_heap);
      break;
    }
    marcel_spin_unlock(&next_same_heap->lock_heap);
    prev_next_same_heap = next_same_heap;
    next_same_heap = next_same_heap->next_same_heap;
  }
  if (!HEAP_IS_HEAP(next_same_heap)) {
    /* not enough space found in heap list */
    /* create a new heap */
    mdebug_memory("create new heap from %p\n",prev_next_same_heap);
    mdebug_memory("heap_amalloc mempolicy %d, nodemask %ld, maxnode %ld, thread %p\n", heap->mempolicy, *heap->nodemask, heap->maxnode, MARCEL_SELF);
    temp_heap = heap_acreatenuma(2*size, HEAP_DYN_ALLOC, heap->mempolicy, heap->weight, heap->nodemask, heap->maxnode);
    /* link heap */
    heap_aconcat_global_list(heap,temp_heap);
    heap_aconcat_local_list(prev_next_same_heap,temp_heap);
    return heap_amalloc(size,temp_heap);
  }
  /* enough space in heap */
  while(HEAP_IS_HEAP(next_same_heap)) {
    marcel_spin_lock(&next_same_heap->lock_heap);
    if (next_same_heap->used == NULL) { /* first used bloc in list of current heap */
      mdebug_memory("first bloc of list\n");
      next_same_heap->used = (heap_ub_t*)((char*)next_same_heap + HEAP_SIZE_T);

      heap_set_bloc(next_same_heap->used, size, 0, next_same_heap, NULL, NULL);

      HEAP_ADD_USED_SIZE(size+HEAP_BLOCK_SIZE_T,next_same_heap);
      next_same_heap->touch_size = size + HEAP_BLOCK_SIZE_T;

      marcel_spin_unlock(&next_same_heap->lock_heap);
      mdebug_memory_list("->",next_same_heap);
      return (void*)((char*)(next_same_heap->used) + HEAP_BLOCK_SIZE_T);
    }

    /* find first free block: first fit */
    current_bloc_used_prev = current_bloc_used = next_same_heap->used;
    while(current_bloc_used != NULL && current_bloc_used->prev_free_size < size + HEAP_BLOCK_SIZE_T) {
      current_bloc_used_prev = current_bloc_used;
      current_bloc_used = current_bloc_used->next;
    }
    //mdebug_memory(" %p %p ok\n",current_bloc_used_prev,current_bloc_used);

    if (current_bloc_used == NULL) { /* not enough free space between used bloc */
      mdebug_memory("no free space between used bloc found\n");

      /* goes to offset of the last used bloc */
      current_bloc_used = current_bloc_used_prev;

      /* check for enough remaining contigous free space */
      if ( (char*)next_same_heap+HEAP_GET_SIZE(next_same_heap) - ((char*)current_bloc_used+HEAP_BLOCK_SIZE_T+current_bloc_used->size) < size + HEAP_BLOCK_SIZE_T) {
        /* goes to next heap */
        marcel_spin_unlock(&next_same_heap->lock_heap);
        if (HEAP_IS_HEAP(next_same_heap->next_same_heap)) {
          next_same_heap = next_same_heap->next_same_heap;
          continue;
        }
        else {
          /* no bloc was found in each heap and no heap has enough contiguous space remaining */
          /* we need to create a new heap */
          mdebug_memory("create new heap from %p\n",next_same_heap);
          temp_heap = heap_acreatenuma(2*size, HEAP_DYN_ALLOC, heap->mempolicy, heap->weight, heap->nodemask, heap->maxnode);
          heap_aconcat_global_list(heap,temp_heap);
          heap_aconcat_local_list(next_same_heap,temp_heap);
          return heap_amalloc(size,temp_heap);
        }
      }

      /* create new used bloc */
      mdebug_memory("heap found\n");
      current_bloc_used->next = (heap_ub_t*)((char*)current_bloc_used+HEAP_BLOCK_SIZE_T+current_bloc_used->size);
      heap_set_bloc(current_bloc_used->next, size, 0, next_same_heap, current_bloc_used, NULL);
      next_same_heap->touch_size = (unsigned long)current_bloc_used->next + size + HEAP_BLOCK_SIZE_T - (unsigned long)next_same_heap;
      HEAP_ADD_USED_SIZE(size+HEAP_BLOCK_SIZE_T,next_same_heap);

      ptr = (char*)(current_bloc_used->next) + HEAP_BLOCK_SIZE_T;
    }
    else { /* free space found between block */
      mdebug_memory("free space between used bloc found\n");

      if(current_bloc_used->prev != NULL) {
        /* goes to offset of the last used bloc */
        temp_bloc_used = current_bloc_used;
        current_bloc_used = (heap_ub_t*)((char*)current_bloc_used_prev+HEAP_BLOCK_SIZE_T+current_bloc_used_prev->size);
        heap_set_bloc(current_bloc_used, size, 0, next_same_heap, current_bloc_used_prev, temp_bloc_used);
        ptr = (char*)(current_bloc_used) + HEAP_BLOCK_SIZE_T;
        current_bloc_used_prev->next = current_bloc_used;
        if (current_bloc_used->next != NULL) {
          current_bloc_used->next->prev_free_size -= (size + HEAP_BLOCK_SIZE_T);
          current_bloc_used->next->prev = current_bloc_used;
        }
      }
      else {
        /* used bloc at begining */
        temp_bloc_used = current_bloc_used;
        current_bloc_used = (heap_ub_t*)((char *)next_same_heap+HEAP_SIZE_T);
        next_same_heap->used = current_bloc_used;
        heap_set_bloc(current_bloc_used,size, 0, next_same_heap, NULL, temp_bloc_used);
        ptr = (char*)(current_bloc_used) + HEAP_BLOCK_SIZE_T;
        if (current_bloc_used->next != NULL) {
          current_bloc_used->next->prev_free_size -= (size + HEAP_BLOCK_SIZE_T);
          current_bloc_used->next->prev = current_bloc_used;
        }
      }
      HEAP_ADD_USED_SIZE(size+HEAP_BLOCK_SIZE_T,next_same_heap);
    }
    marcel_spin_unlock(&next_same_heap->lock_heap);
    return ptr;
  }
  return NULL;
}

void *heap_acalloc(size_t nmemb, size_t size, heap_heap_t* heap) {
  void* ptr;
  size_t local_size = heap_memalign(nmemb*size);

  /* amalloc */
  mdebug_memory("heap_acalloc: call to amalloc\n");
  ptr = heap_amalloc(local_size,heap);
  if (ptr == NULL) {
    mdebug_memory("heap_acalloc: call to heap_amalloc failed\n");
    return NULL;
  }
  /* zeroing memory: only memory area specified */
  HEAP_MALLOC_ZERO(ptr,local_size);

  return ptr;
}

void *heap_arealloc(void *ptr, size_t size, heap_heap_t* heap) {
  heap_ub_t *current_bloc_used;
  //heap_ub_t *current_bloc_used_prev;
  //heap_ub_t *temp_bloc_used;
  void* new_ptr;

  size = heap_memalign(size);
  if (!HEAP_IS_HEAP_POLICY(heap,HEAP_DYN_ALLOC)) {
    mdebug_memory("heap_arealloc: bad heap\n");
    return NULL;
  }

  if (ptr == NULL) { /* call amalloc */
    ptr = heap_amalloc(size,heap);
    if (ptr == NULL) {
      mdebug_memory("heap_arealloc: call to amalloc failed\n");
      return NULL;
    }
    return ptr;
  }
  if (size == 0) { /* afree ptr */
    mdebug_memory("heap_arealloc: size = 0 free memory\n");
    heap_afree_heap(ptr,heap);
    return NULL;
  }

  marcel_spin_lock(&heap->lock_heap);
  current_bloc_used = (heap_ub_t *)((char*)ptr-HEAP_BLOCK_SIZE_T);
  if (current_bloc_used->heap == heap) {
    if (current_bloc_used->size == size) { /* do nothing */
      mdebug_memory("heap_arealloc: nothing to do\n");
      marcel_spin_unlock(&heap->lock_heap);
      return ptr;
    }

    if (current_bloc_used->size > size) { /* reduce bloc size */
      mdebug_memory("heap_arealloc: reduce bloc\n");
      if (current_bloc_used->next != NULL) {
        current_bloc_used->next->prev_free_size += current_bloc_used->size - size;
      }
      current_bloc_used->size = size;
      HEAP_ADD_FREE_SIZE(current_bloc_used->size - size,heap);
      marcel_spin_unlock(&heap->lock_heap);
      return ptr;
    }

    /* size area need to be increased */
    /* first check enough space after bloc then if not call amalloc, move data and free ptr */

    if (current_bloc_used->next != NULL && current_bloc_used->next->prev_free_size >= size - current_bloc_used->size) {
      /* enough space after bloc and not last bloc */
      mdebug_memory("heap_arealloc: enough space after bloc not last bloc\n");
      current_bloc_used->next->prev_free_size -= (size - current_bloc_used->size);
      HEAP_ADD_USED_SIZE(size - current_bloc_used->size,heap);
      current_bloc_used->size = size;
      marcel_spin_unlock(&heap->lock_heap);
      return ptr;
    }
    if (current_bloc_used->next == NULL  && (char*)heap+HEAP_SIZE_T+HEAP_GET_SIZE(heap) > (char*)current_bloc_used+HEAP_BLOCK_SIZE_T+size) {
      /* last block and enough remaining space */
      mdebug_memory("heap_arealloc: enough space after bloc and last bloc\n");
      HEAP_ADD_USED_SIZE(size - current_bloc_used->size,heap);
      current_bloc_used->size = size;
      marcel_spin_unlock(&heap->lock_heap);
      return ptr;
    }
  }

  /* make a malloc */
  marcel_spin_unlock(&heap->lock_heap);
  new_ptr = heap_amalloc(size,heap);
  if (new_ptr != NULL) {
    /* move data */
    HEAP_MALLOC_COPY(new_ptr,ptr,size);
    /* make a free */
    heap_afree_heap(ptr,heap);
    return new_ptr;
  }
  return ptr;
}

void heap_afree(void *ptr) {
  heap_ub_t *current_bloc_used;
  if (ptr != NULL) {
    current_bloc_used = (heap_ub_t *)((char*)ptr-HEAP_BLOCK_SIZE_T);
    mdebug_memory("afree: heap=%p\n",current_bloc_used->heap);
    heap_afree_heap(ptr,current_bloc_used->heap);
  }
}

void heap_afree_heap(void *ptr, heap_heap_t* heap) {
  heap_ub_t *current_bloc_used;
  size_t size;

  if (ptr != NULL && HEAP_IS_HEAP_POLICY(heap,HEAP_DYN_ALLOC)) {
    marcel_spin_lock(&heap->lock_heap);
    current_bloc_used = (heap_ub_t *)((char*)ptr-HEAP_BLOCK_SIZE_T);
    size = current_bloc_used->size;
    if (current_bloc_used->prev != NULL) {
      current_bloc_used->prev->next = current_bloc_used->next;
      if(current_bloc_used->next != NULL) {
        current_bloc_used->next->prev = current_bloc_used->prev;
        current_bloc_used->next->prev_free_size += current_bloc_used->size+HEAP_BLOCK_SIZE_T+current_bloc_used->prev_free_size;
      }
    }
    else {
      heap->used = current_bloc_used->next;
      if (current_bloc_used->next != NULL) {
        current_bloc_used->next->prev_free_size += current_bloc_used->size+HEAP_BLOCK_SIZE_T+current_bloc_used->prev_free_size;
        current_bloc_used->next->prev = NULL;
      }
    }
    HEAP_ADD_FREE_SIZE(size+HEAP_BLOCK_SIZE_T,heap);
    marcel_spin_unlock(&heap->lock_heap);
    mdebug_memory_list("heap_afree_heap\n",heap);
  }
  else {
    /* argument = NULL: undefined behavior */
    mdebug_memory("heap_afree_heap: passing NULL as argument caused undefined behavior\n");
  }
}

void heap_apagefree(void* ptr, int nb_pages, heap_heap_t *heap) {
  unsigned int i,c;
  unsigned int pagesize;

  if (ptr != NULL && HEAP_IS_HEAP_POLICY(heap,HEAP_PAGE_ALLOC)) {
    marcel_spin_lock(&heap->lock_heap);
    pagesize = getpagesize () ;
    i = ((unsigned long)ptr-(unsigned long)heap)/pagesize;
    mdebug_memory("i = %d\n",i);
    for(c = i;c < i + nb_pages; c++) {
      heap_clearbit(heap->bitmap,c);
    }
    HEAP_ADD_FREE_SIZE(nb_pages*pagesize,heap);
    marcel_spin_unlock(&heap->lock_heap);
  }
}

heap_amalloc_stat_t heap_amallinfo(heap_heap_t* heap) {
  heap_heap_t * next_heap;
  heap_amalloc_stat_t stats;

  stats.free_size = 0;
  stats.used_size = 0;
  stats.touch_size = 0;
  stats.attached_size = 0;
  stats.npinfo = 0;

  next_heap = heap;
  while(HEAP_IS_HEAP(next_heap)) {
    marcel_spin_lock(&next_heap->lock_heap);
    stats.free_size += next_heap->free_size;
    stats.used_size += next_heap->used_size;
    stats.touch_size += next_heap->touch_size;
    stats.attached_size += next_heap->attached_size;
    stats.npinfo++;
    marcel_spin_unlock(&next_heap->lock_heap);
    next_heap = next_heap->next_heap;
  }
  mdebug_memory_list("heap_amallinfo:\n",heap);
  return stats;
}

void heap_print_list(const char* str, heap_heap_t* heap) {
  heap_heap_t* h;
  int count = 0;
  unsigned int i, max_node = (unsigned int) (marcel_nbnodes - 1) ;

  if (debug_memory.show < PM2DEBUG_STDLEVEL) return;

  debug_printf(&debug_memory, "%s", str);
  h = heap;
  while (HEAP_IS_HEAP(h)) {
    marcel_spin_lock(&h->lock_heap);
    marcel_fprintf(stderr, "heap #%d %p (it=%d) (%d,%d) numa=",count,h,h->iterator_num,h->mempolicy,h->weight);
    for(i = 0; i < h->maxnode/HEAP_WORD_SIZE; i++) {
      printf("%ld|",h->nodemask[i]);
    }
    for(i = 0; i < max_node+1; ++i) {
      if(heap_mask_isset(h->nodemask,h->maxnode,i)) {
        marcel_fprintf(stderr, "%d-",i);
      }
    }
    marcel_fprintf(stderr, " : next_same=%p|",h->next_same_heap);
    marcel_fprintf(stderr, " : next=%p|",h->next_heap);
    marcel_fprintf(stderr, " : %d : ",(int)HEAP_GET_SIZE(h));
    heap_print_heap(h->used);
    count++;
    marcel_spin_unlock(&h->lock_heap);
    h = h->next_heap;
  }
}

void heap_print_heap(struct heap_ub* root) {
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

#endif /* LINUX_SYS */
#endif /* MM_HEAP_ENABLED */