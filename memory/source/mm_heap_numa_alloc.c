/*
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

#include <errno.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include "marcel.h"
#include "mm_heap_alloc.h"
#include "mm_heap_numa_alloc.h"
#include "mm_heap_numa.h"
#include "mm_debug.h"
#include "mm_helper.h"

void *heap_hmalloc(size_t size, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode, heap_heap_t *heap) {
  heap_heap_t* current_heap;

  //marcel_fprintf(stderr,"heap_hmalloc size=%ld at %p (%d,%d) numa=%ld\n", (unsigned long)size,heap,mempolicy,weight,*nodemask);
  mdebug_memory("heap_hmalloc size=%ld at %p (%d,%d) numa=%ld\n",(unsigned long)size,heap,mempolicy,weight,*nodemask);
  /* look for corresponding heap */
  current_heap = heap_aget_heap_from_list(mempolicy,weight,nodemask,maxnode,heap);
  //marcel_fprintf(stderr,"heap_hmalloc mempolicy %d, nodemask %ld, maxnode %ld, thread %p\n", mempolicy, *nodemask, maxnode, MARCEL_SELF);
  if (current_heap == NULL) {
    marcel_spin_lock(&heap->lock_heap);
    if (heap->mempolicy == HEAP_UNSPECIFIED_POLICY && heap->weight == HEAP_UNSPECIFIED_WEIGHT && heap->maxnode == 0) {
      heap_amaparea(heap,mempolicy,weight,nodemask,maxnode);
      marcel_spin_unlock(&heap->lock_heap);
      return heap_amalloc(size,heap);
    }
    else {
      /* new heap to create */
      marcel_spin_unlock(&heap->lock_heap);
      current_heap = heap_acreatenuma(2*size,HEAP_DYN_ALLOC,mempolicy,weight,nodemask,maxnode);
      heap_aconcat_global_list(heap,current_heap);
    }
  }
  return heap_amalloc(size,current_heap);
}

void *heap_hrealloc(void *ptr, size_t size) {
  heap_ub_t* current_bloc_used;
  heap_heap_t *current_heap;

  current_bloc_used = (heap_ub_t *)((char*)ptr-HEAP_BLOCK_SIZE_T);
  current_heap = current_bloc_used->heap;
  return heap_arealloc(ptr,size,current_heap);
}

void *heap_hcalloc(size_t nmemb, size_t size, int mempolicy, int weight, unsigned long* nodemask, unsigned long maxnode, heap_heap_t *heap) {
  heap_heap_t* current_heap;

  /* look for corresponding heap */
  current_heap = heap_aget_heap_from_list(mempolicy,weight,nodemask,maxnode,heap);

  if (current_heap == NULL) {
    marcel_spin_lock(&heap->lock_heap);
    if (size < HEAP_GET_SIZE(heap)+HEAP_BLOCK_SIZE_T && heap->mempolicy == HEAP_UNSPECIFIED_POLICY && heap->weight == HEAP_UNSPECIFIED_WEIGHT && heap->maxnode == 0) {
      heap_amaparea(heap,mempolicy,weight,nodemask,maxnode);
      marcel_spin_unlock(&heap->lock_heap);
      return heap_acalloc(nmemb,size,heap);
    }
    else {
      /* new heap to create */
      marcel_spin_unlock(&heap->lock_heap);
      current_heap = heap_acreatenuma(2*size,HEAP_DYN_ALLOC,mempolicy,weight,nodemask,maxnode);
      heap_aconcat_global_list(heap,current_heap);
      heap_aconcat_local_list(heap,current_heap);
    }
  }
  return heap_acalloc(nmemb,size,current_heap);
}

void heap_hfree(void *data) {
  heap_afree(data);
}

void heap_hattach_memory(void *ptr, size_t size, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode, heap_heap_t *heap) {
  heap_ub_t* current_bloc_used;
  heap_heap_t* current_heap;
  void* local_ptr;

  local_ptr = heap_hmalloc(0, mempolicy, weight, nodemask, maxnode, heap);
  current_bloc_used = (heap_ub_t *)((char*)local_ptr-HEAP_BLOCK_SIZE_T);
  current_heap = current_bloc_used->heap;;
  current_bloc_used->data = ptr;
  current_bloc_used->stat_size = size;
  marcel_spin_lock(&current_heap->lock_heap);
  current_heap->attached_size += size;
  current_heap->nb_attach++;
  heap_maparea(ptr,size,mempolicy,current_heap->nodemask,current_heap->maxnode);
  marcel_spin_unlock(&current_heap->lock_heap);
}

void heap_hdetach_memory(void *ptr, heap_heap_t *heap) {
  int found = 0;
  heap_ub_t* current_bloc_used;
  heap_heap_t* current_heap;

  current_heap = heap;
  while(HEAP_IS_HEAP(current_heap) && !found) {
    marcel_spin_lock(&current_heap->lock_heap);
    current_bloc_used = current_heap->used;
    while(current_bloc_used != NULL) {
      if (current_bloc_used->data == ptr) {
        current_heap->attached_size -= current_bloc_used->stat_size;
        current_heap->nb_attach--;
        found = 1;
        if (current_bloc_used->prev != NULL) {
          current_bloc_used->prev->next = current_bloc_used->next;
          if(current_bloc_used->next != NULL) {
            current_bloc_used->next->prev = current_bloc_used->prev;
            current_bloc_used->next->prev_free_size += HEAP_BLOCK_SIZE_T+current_bloc_used->prev_free_size;
          }
        }
        else {
          current_heap->used = current_bloc_used->next;
          if(current_bloc_used->next != NULL) {
            current_bloc_used->next->prev_free_size += HEAP_BLOCK_SIZE_T+current_bloc_used->prev_free_size;
            current_bloc_used->next->prev = NULL;
          }
        }
        break;
      }
      current_bloc_used = current_bloc_used->next;
    }
    marcel_spin_unlock(&current_heap->lock_heap);
    current_heap = current_heap->next_heap;
  }
}

int heap_hmove_memory(heap_pinfo_t *ppinfo, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode,  heap_heap_t *heap)  {
  heap_heap_t *current_heap, *next_same_heap, *match_heap;
  heap_ub_t* current_bloc_used;
  int nb_attach;

  /* look for corresponding heap */
  current_heap = heap_aget_heap_from_list(ppinfo->mempolicy,ppinfo->weight,ppinfo->nodemask,ppinfo->maxnode,heap);
  mdebug_memory("heap_hmove_memory (%d,%d) numa=%ld current_heap %p\n",ppinfo->mempolicy,ppinfo->weight,*ppinfo->nodemask,current_heap);
  if (current_heap == NULL) return 0;

  if (ppinfo->mempolicy == mempolicy && ppinfo->weight == weight && heap_mask_equal(ppinfo->nodemask,ppinfo->maxnode,nodemask,maxnode)) {
    return 0;
  }

  /* 	int i,j; */
  /* 	for (i=0;i<HEAP_HEAP_NBPAGES;i++) */
  /* 	{ */
  /* 		/\* ya pas moyen d'arranger ça autrement qu'en tour de boucle *\/  */
  /* 		j = 0; */
  /* 		while(1) */
  /* 		{ */
  /* 			if (bit_is_marked(nodemask,j)) */
  /* 				break; */
  /* 			j++; */
  /* 		} */
  /* 		current_heap->pages[i]= j; */
  /* 	} */

  mdebug_memory_list("heap_hmove_memory before\n",current_heap);

  /* look for a existing matching heap to link */
  match_heap = heap_aget_heap_from_list(mempolicy,weight,nodemask,maxnode,heap);

  /* map heap */
  next_same_heap = current_heap;
  while(HEAP_IS_HEAP(next_same_heap)) {
    marcel_spin_lock(&next_same_heap->lock_heap);
    if (heap_amaparea(next_same_heap, mempolicy, weight, nodemask, maxnode) != 0) {
      mdebug_memory("heap_amaparea failed\n");
    }
    nb_attach = next_same_heap->nb_attach;
    current_bloc_used = next_same_heap->used;
    while(current_bloc_used != NULL && nb_attach > 0) {
      if (current_bloc_used->data != NULL) {
        if (heap_maparea(current_bloc_used->data, current_bloc_used->stat_size, mempolicy, nodemask, maxnode) != 0) {
          mdebug_memory("heap_amaparea failed\n");
        }
        nb_attach--;
      }
      current_bloc_used = current_bloc_used->next;
    }
    marcel_spin_unlock(&next_same_heap->lock_heap);
    next_same_heap = next_same_heap->next_same_heap;
  }

  /* rebuild list */
  if(match_heap != NULL) {
    if (current_heap > match_heap) {
      heap_aconcat_local_list(match_heap,current_heap);
    }
    else {
      heap_aconcat_local_list(current_heap,match_heap);
    }
  }
  return 1;
}

void heap_hget_pinfo(void *ptr, heap_pinfo_t* ppinfo, heap_heap_t *heap) {
  heap_heap_t* current_heap;
  heap_ub_t* current_bloc_used;
  int i,found = 0;

  current_heap = heap;
  while(HEAP_IS_HEAP(current_heap)) {
    marcel_spin_lock(&current_heap->lock_heap);
    if ((unsigned long) ptr < (unsigned long)current_heap + HEAP_GET_SIZE(current_heap) && (unsigned long)ptr > (unsigned long)current_heap) {
      /* found: heap < data < heap+size */
      mdebug_memory("found %p in %p\n",ptr,current_heap);
      ppinfo->size = HEAP_GET_SIZE(current_heap);
      ppinfo->mempolicy = current_heap->mempolicy;
      ppinfo->weight = current_heap->weight;
      for (i = 0; i < current_heap->maxnode/HEAP_WORD_SIZE; ++i) {
        ppinfo->nodemask[i] = current_heap->nodemask[i];
      }
      ppinfo->maxnode = current_heap->maxnode;
      for(i = 0; i < marcel_nbnodes; ++i) {
        ppinfo->nb_touched[i] = 0;
      }
      found = 1;
      marcel_spin_unlock(&current_heap->lock_heap);
      break;
    }
    marcel_spin_unlock(&current_heap->lock_heap);
    current_heap = current_heap->next_heap;
  }

  if (!found) { /* not found as regular block check if it's an attached memory */
    current_heap = heap;
    while(current_heap != NULL && !found) {
      marcel_spin_lock(&current_heap->lock_heap);
      current_bloc_used = current_heap->used;
      while(current_bloc_used != NULL) {
        if (current_bloc_used->data == ptr) {
          ppinfo->mempolicy = current_heap->mempolicy;
          ppinfo->weight = current_heap->weight;
          for (i = 0; i < current_heap->maxnode/HEAP_WORD_SIZE; ++i) {
            ppinfo->nodemask[i] = current_heap->nodemask[i];
          }
          ppinfo->maxnode = current_heap->maxnode;
          for(i = 0; i < marcel_nbnodes; ++i) {
            ppinfo->nb_touched[i] = 0;
          }
          found = 1;
          break;
        }
        current_bloc_used = current_bloc_used->next;
      }
      marcel_spin_unlock(&current_heap->lock_heap);
      current_heap = current_heap->next_heap;
    }
  }
}

int heap_hnext_pinfo(heap_pinfo_t **ppinfo, heap_heap_t* heap) {
  heap_heap_t *current_heap, *current_same_heap;
  int i,iterator_num = HEAP_ITERATOR_ID_UNDEFINED;

  if (HEAP_IS_HEAP(heap)) {
    if (*ppinfo == NULL) {
      /* first call, allocate iterator */
      if (heap->iterator == NULL) {
        heap->iterator = heap_amalloc(sizeof(heap_pinfo_t)+heap->maxnode,heap);
      }
      *ppinfo = heap->iterator;
      (*ppinfo)->size = HEAP_GET_SIZE(heap);
      (*ppinfo)->mempolicy = HEAP_UNSPECIFIED_POLICY;
      (*ppinfo)->weight = HEAP_UNSPECIFIED_WEIGHT;
      (*ppinfo)->maxnode = 0;
      (*ppinfo)->nodemask =
        (unsigned long *) ((char *) heap->iterator + sizeof(heap_pinfo_t));

      /* reset iterator, some node may have changed */
      current_heap = heap;
      while (HEAP_IS_HEAP(current_heap)) {
        marcel_spin_lock(&current_heap->lock_heap);
        if (current_heap->iterator_num != HEAP_ITERATOR_ID_UNDEFINED) {
          current_heap->iterator_num = HEAP_ITERATOR_ID_UNDEFINED;
        }
        marcel_spin_unlock(&current_heap->lock_heap);
        current_heap = current_heap->next_heap;
      }
      mdebug_memory("heap_hnext_pinfo: first call, it=%d\n",iterator_num);
    }

    /* set iterator id list */
    current_heap = heap;
    while (HEAP_IS_HEAP(current_heap)) {
      if (current_heap->iterator_num == HEAP_ITERATOR_ID_UNDEFINED) {
        marcel_spin_lock(&current_heap->lock_heap);
        current_heap->iterator_num = iterator_num++;
        current_same_heap = current_heap;
        marcel_spin_unlock(&current_heap->lock_heap);
        while(HEAP_IS_HEAP(current_same_heap)) {
          marcel_spin_lock(&current_same_heap->lock_heap);
          current_same_heap->iterator_num = iterator_num;
          marcel_spin_unlock(&current_same_heap->lock_heap);
          current_same_heap = current_same_heap->next_same_heap;
        }
      }
      current_heap = current_heap->next_heap;
    }
    mdebug_memory_list("heap_hnext_pinfo list:\n",heap);

    /* goes to next heap higher iterator_num */
    current_heap = heap_aget_heap_from_list((*ppinfo)->mempolicy,(*ppinfo)->weight,(*ppinfo)->nodemask,(*ppinfo)->maxnode,heap);
    if (current_heap == NULL) {
      current_heap = heap;
      iterator_num = HEAP_ITERATOR_ID_UNDEFINED;
    }
    else {
      iterator_num = current_heap->iterator_num;
    }
    current_heap = heap;
    while (HEAP_IS_HEAP(current_heap)) {
      if (current_heap->iterator_num == iterator_num + 1) {
        mdebug_memory("next heap %p it=%d\n",current_heap,current_heap->iterator_num);
        break;
      }
      current_heap = current_heap->next_heap;
    }
    if (!HEAP_IS_HEAP(current_heap)) { /* no more pinfo */
      return 0;
    }
    // set ppinfo
    marcel_spin_lock(&current_heap->lock_heap);
    (*ppinfo)->size = HEAP_GET_SIZE(current_heap);
    (*ppinfo)->mempolicy = current_heap->mempolicy;
    (*ppinfo)->weight = current_heap->weight;
    for (i = 0; i < current_heap->maxnode/sizeof(unsigned long); ++i) {
      (*ppinfo)->nodemask[i] = current_heap->nodemask[i];
    }
    (*ppinfo)->maxnode = current_heap->maxnode;
    for(i = 0; i < marcel_nbnodes; ++i) {
      (*ppinfo)->nb_touched[i] = 0;
    }
    marcel_spin_unlock(&current_heap->lock_heap);
    return 1;
  }
  return -1;
}

void heap_hupdate_memory_nodes(heap_pinfo_t *ppinfo, heap_heap_t *heap) {
  heap_heap_t* current_heap;
  heap_ub_t* current_bloc_used;
  int i, nb_pages, pagesize, node, nb_attach;
  const void *addr[1];

  if (HEAP_IS_HEAP(heap)) {
    pagesize = getpagesize();
    for(i = 0; i < marcel_nbnodes; ++i) {
      ppinfo->nb_touched[i] = 0;
    }
    /* look for corresponding heap */
    current_heap = heap_aget_heap_from_list(ppinfo->mempolicy,ppinfo->weight,ppinfo->nodemask,ppinfo->maxnode,heap);

    if (HEAP_IS_HEAP(current_heap)) {
      while(HEAP_IS_HEAP(current_heap)) {
        marcel_spin_lock(&current_heap->lock_heap);
        nb_pages = HEAP_GET_SIZE(current_heap)/pagesize;

        for(i = 0; i < nb_pages; ++i) {
          /* if page is not touched */
          //if (current_heap->pages[i] == 0)
          //{
          addr[0] = (char *) current_heap + i*pagesize;
          //		addr[0] = (void*)current_heap+0;
          //	mdebug_memory("addr=%p size=%d*%d\n",addr[0],nb_pages,pagesize);
          node = -1;
          int ret = _mm_move_pages((void **)addr, 1, NULL, &node, MPOL_MF_MOVE);
          if (ret != 0)
            {
              perror("syscall6 error:");
              if (errno == ENOSYS)
                marcel_fprintf(stderr,"upgrade to kernel >= 2.6.18\n");
              else
                marcel_fprintf(stderr,"errno syscall6 = %d\n", errno);
            }
          if (node >= 0) {
            ppinfo->nb_touched[node]++;
            mdebug_memory("page touched %p node=%d\n",addr[0],node);
          }
          /* set the page touched */
          //current_heap->pages[i] = node;
          //}
          //else
          //{
          /* find the node in the touched page */
          //	ppinfo->nb_touched[current_heap->pages[i]]++;
          //}
        }
        if (current_heap->nb_attach > 0) {
          nb_attach = current_heap->nb_attach;
          current_bloc_used = current_heap->used;
          while(current_bloc_used != NULL && nb_attach > 0) {
            if (current_bloc_used->data != NULL) {
              nb_pages = current_bloc_used->stat_size/pagesize;
              for(i = 0; i < nb_pages; ++i) {

                /* if page is not touched */
                //if (current_heap->pages[i] == 0)
                //{
                addr[0] = (char *) current_bloc_used->data + i*pagesize;
                _mm_move_pages((void **)addr, 1, NULL, &node, MPOL_MF_MOVE);
                if (node >= 0) {
                  ppinfo->nb_touched[node]++;
                }

                /* set the page touched */
                //current_heap->pages[i] = node;
                //}
                //else
                //{
                /* find the node in the touched page */
                //	ppinfo->nb_touched[current_heap->pages[i]]++;
                //}
              }
              nb_attach--;
            }
            current_bloc_used = current_bloc_used->next;
          }
        }
        marcel_spin_unlock(&current_heap->lock_heap);
        current_heap = current_heap->next_same_heap;
      }
    }
  }
}

heap_heap_t* heap_hcreate_heap(void) {
  unsigned int pagesize = getpagesize();
  return heap_acreate(HEAP_MINIMUM_PAGES*pagesize, HEAP_DYN_ALLOC);
}

void heap_hdelete_heap(heap_heap_t *heap) {
  heap_adelete(&heap);
}

void heap_hmerge_heap(heap_heap_t *hacc, heap_heap_t *h) {
  heap_heap_t* current_heap;
  heap_heap_t* merging_heap;

  if (HEAP_IS_HEAP(hacc) && HEAP_IS_HEAP(h) && hacc != h) {
    current_heap = hacc;
    while (HEAP_IS_HEAP(current_heap)) {
      marcel_spin_lock(&current_heap->lock_heap);
      /* look for last numa heap */
      if (current_heap->next_same_heap == NULL) {
        /* look for matching heap */
        merging_heap = heap_aget_heap_from_list(current_heap->mempolicy,current_heap->weight,current_heap->nodemask,current_heap->maxnode,h);
        if (merging_heap != NULL) {
          heap_aconcat_local_list(current_heap,merging_heap);
        }
      }
      marcel_spin_unlock(&current_heap->lock_heap);
      current_heap = current_heap->next_heap;
    }
    heap_aconcat_global_list(hacc,h);
    if (h->iterator != NULL) {
      heap_afree(h->iterator);
    }
  }
}

#endif /* LINUX_SYS */
#endif /* MM_HEAP_ENABLED */
