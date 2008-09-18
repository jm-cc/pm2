/*
 * src/maheap_numa_alloc.c
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
#include <sys/mman.h>
#include <sys/syscall.h>

#include "marcel.h"

#ifdef MA__NUMA_MEMORY
#include <numaif.h>

#ifdef LINUX_SYS
#ifndef __NR_move_pages

#ifdef X86_64_ARCH
#define __NR_move_pages 279
#elif IA64_ARCH
#define __NR_move_pages 1276
#elif X86_ARCH
#define __NR_move_pages 317
#elif PPC_ARCH
#define __NR_move_pages 301
#elif PPC64_ARCH
#define __NR_move_pages 301
#endif

#endif /* __NR_move_pages */

void *ma_hmalloc(size_t size, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode, ma_heap_t *heap) {
	ma_heap_t* current_heap;
	//marcel_fprintf(stderr,"ma_hmalloc size=%ld at %p (%d,%d) numa=%ld\n", (unsigned long)size,heap,mempolicy,weight,*nodemask);
	mdebug_heap("ma_hmalloc size=%ld at %p (%d,%d) numa=%ld\n",(unsigned long)size,heap,mempolicy,weight,*nodemask);
	/* look for corresponding heap */
	current_heap = ma_aget_heap_from_list(mempolicy,weight,nodemask,maxnode,heap);
	//marcel_fprintf(stderr,"ma_hmalloc mempolicy %d, nodemask %ld, maxnode %ld, thread %p\n", mempolicy, *nodemask, maxnode, MARCEL_SELF);
	if (current_heap == NULL) {
		ma_spin_lock(&heap->lock_heap);
		if (heap->mempolicy == HEAP_UNSPECIFIED_POLICY && heap->weight == HEAP_UNSPECIFIED_WEIGHT && heap->maxnode == 0) {
			ma_amaparea(heap,mempolicy,weight,nodemask,maxnode);
			ma_spin_unlock(&heap->lock_heap);
			return ma_amalloc(size,heap);
		} else {
			/* new heap to create */
			ma_spin_unlock(&heap->lock_heap);
			current_heap = ma_acreatenuma(2*size,HEAP_DYN_ALLOC,mempolicy,weight,nodemask,maxnode);
			ma_aconcat_global_list(heap,current_heap);
		}
	}
	return ma_amalloc(size,current_heap);
}

void *ma_hrealloc(void *ptr, size_t size) {
	ma_ub_t* current_bloc_used;
	ma_heap_t *current_heap;

	current_bloc_used = (ma_ub_t *)((char*)ptr-BLOCK_SIZE_T);
	current_heap = current_bloc_used->heap;
	return ma_arealloc(ptr,size,current_heap);
}

void *ma_hcalloc(size_t nmemb, size_t size, int mempolicy, int weight, unsigned long* nodemask, unsigned long maxnode, ma_heap_t *heap) {
	ma_heap_t* current_heap;

	/* look for corresponding heap */
	current_heap = ma_aget_heap_from_list(mempolicy,weight,nodemask,maxnode,heap);

	if (current_heap == NULL) {
		ma_spin_lock(&heap->lock_heap);
		if (size < HEAP_GET_SIZE(heap)+BLOCK_SIZE_T && heap->mempolicy == HEAP_UNSPECIFIED_POLICY && heap->weight == HEAP_UNSPECIFIED_WEIGHT && heap->maxnode == 0) {
			ma_amaparea(heap,mempolicy,weight,nodemask,maxnode);
			ma_spin_unlock(&heap->lock_heap);
			return ma_acalloc(nmemb,size,heap);
		} else {
			/* new heap to create */
			ma_spin_unlock(&heap->lock_heap);
			current_heap = ma_acreatenuma(2*size,HEAP_DYN_ALLOC,mempolicy,weight,nodemask,maxnode);
			ma_aconcat_global_list(heap,current_heap);
			ma_aconcat_local_list(heap,current_heap);
		}
	}
	return ma_acalloc(nmemb,size,current_heap);
}

void ma_hfree(void *data) {
	ma_afree(data);
}

void ma_hattach_memory(void *ptr, size_t size, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode, ma_heap_t *heap) {
	ma_ub_t* current_bloc_used;
	ma_heap_t* current_heap;
	void* local_ptr;

	local_ptr = ma_hmalloc(0, mempolicy, weight, nodemask, maxnode, heap);
	current_bloc_used = (ma_ub_t *)((char*)local_ptr-BLOCK_SIZE_T);
	current_heap = current_bloc_used->heap;;
	current_bloc_used->data = ptr;
	current_bloc_used->stat_size = size;
	ma_spin_lock(&current_heap->lock_heap);
	current_heap->attached_size += size;
	current_heap->nb_attach++;
	ma_maparea(ptr,size,mempolicy,current_heap->nodemask,current_heap->maxnode);
	ma_spin_unlock(&current_heap->lock_heap);
}

void ma_hdetach_memory(void *ptr, ma_heap_t *heap) {
	int found = 0;
	ma_ub_t* current_bloc_used;
	ma_heap_t* current_heap;

	current_heap = heap;
	while(IS_HEAP(current_heap) && !found) {
		ma_spin_lock(&current_heap->lock_heap);
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
						current_bloc_used->next->prev_free_size += BLOCK_SIZE_T+current_bloc_used->prev_free_size;
					}
				} else {
					current_heap->used = current_bloc_used->next;
					if(current_bloc_used->next != NULL) {
						current_bloc_used->next->prev_free_size += BLOCK_SIZE_T+current_bloc_used->prev_free_size;
						current_bloc_used->next->prev = NULL;
					}
				}
				break;
			}
			current_bloc_used = current_bloc_used->next;
		}
		ma_spin_unlock(&current_heap->lock_heap);
		current_heap = current_heap->next_heap;
	}
}

int ma_hmove_memory(ma_pinfo_t *ppinfo, int mempolicy, int weight, unsigned long *nodemask, unsigned long maxnode,  ma_heap_t *heap)  {
	ma_heap_t *current_heap, *next_same_heap, *match_heap;
	ma_ub_t* current_bloc_used;
	int nb_attach;

	/* look for corresponding heap */
	current_heap = ma_aget_heap_from_list(ppinfo->mempolicy,ppinfo->weight,ppinfo->nodemask,ppinfo->maxnode,heap);
	mdebug_heap("ma_hmove_memory (%d,%d) numa=%ld current_heap %p\n",ppinfo->mempolicy,ppinfo->weight,*ppinfo->nodemask,current_heap);
	if (current_heap == NULL) return 0;

	if (ppinfo->mempolicy == mempolicy && ppinfo->weight == weight && mask_equal(ppinfo->nodemask,ppinfo->maxnode,nodemask,maxnode)) {
		return 0;
	}

	/* 	int i,j; */
	/* 	for (i=0;i<MA_HEAP_NBPAGES;i++) */
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

	mdebug_heap_list("ma_hmove_memory before\n",current_heap);

	/* look for a existing matching heap to link */
	match_heap = ma_aget_heap_from_list(mempolicy,weight,nodemask,maxnode,heap);

	/* map heap */
	next_same_heap = current_heap;
	while(IS_HEAP(next_same_heap)) {
		ma_spin_lock(&next_same_heap->lock_heap);
		if (ma_amaparea(next_same_heap, mempolicy, weight, nodemask, maxnode) != 0) {
			mdebug_heap("ma_amaparea failed\n");
		}
		nb_attach = next_same_heap->nb_attach;
		current_bloc_used = next_same_heap->used;
		while(current_bloc_used != NULL && nb_attach > 0) {
			if (current_bloc_used->data != NULL) {
				if (ma_maparea(current_bloc_used->data, current_bloc_used->stat_size, mempolicy, nodemask, maxnode) != 0) {
					mdebug_heap("ma_amaparea failed\n");
				}
				nb_attach--;
			}
			current_bloc_used = current_bloc_used->next;
		}
		ma_spin_unlock(&next_same_heap->lock_heap);
		next_same_heap = next_same_heap->next_same_heap;
	}

	/* rebuild list */
	if(match_heap != NULL) {
		if (current_heap > match_heap) {
			ma_aconcat_local_list(match_heap,current_heap);
		} else {
			ma_aconcat_local_list(current_heap,match_heap);
		}
	}
	return 1;
}

void ma_hget_pinfo(void *ptr, ma_pinfo_t* ppinfo, ma_heap_t *heap) {
	ma_heap_t* current_heap;
	ma_ub_t* current_bloc_used;
	int i,found = 0;
	current_heap = heap;
	while(IS_HEAP(current_heap)) {
		ma_spin_lock(&current_heap->lock_heap);
		if ((unsigned long) ptr < (unsigned long)current_heap + HEAP_GET_SIZE(current_heap) && (unsigned long)ptr > (unsigned long)current_heap) {
			/* found: heap < data < heap+size */
			mdebug_heap("found %p in %p\n",ptr,current_heap);
			ppinfo->size = HEAP_GET_SIZE(current_heap);
			ppinfo->mempolicy = current_heap->mempolicy;
			ppinfo->weight = current_heap->weight;
			for (i = 0; i < current_heap->maxnode/WORD_SIZE; ++i) {
				ppinfo->nodemask[i] = current_heap->nodemask[i];
			}
			ppinfo->maxnode = current_heap->maxnode;
			for(i = 0; i < marcel_nbnodes; ++i) {
				ppinfo->nb_touched[i] = 0;
			}
			found = 1;
			ma_spin_unlock(&current_heap->lock_heap);
			break;
		}
		ma_spin_unlock(&current_heap->lock_heap);
		current_heap = current_heap->next_heap;
	}

	if (!found) { /* not found as regular block check if it's an attached memory */
		current_heap = heap;
		while(current_heap != NULL && !found) {
			ma_spin_lock(&current_heap->lock_heap);
			current_bloc_used = current_heap->used;
			while(current_bloc_used != NULL) {
				if (current_bloc_used->data == ptr) {
					ppinfo->mempolicy = current_heap->mempolicy;
					ppinfo->weight = current_heap->weight;
					for (i = 0; i < current_heap->maxnode/WORD_SIZE; ++i) {
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
			ma_spin_unlock(&current_heap->lock_heap);
			current_heap = current_heap->next_heap;
		}
	}
}

int ma_hnext_pinfo(ma_pinfo_t **ppinfo, ma_heap_t* heap) {
	ma_heap_t *current_heap, *current_same_heap;
	int i,iterator_num = HEAP_ITERATOR_ID_UNDEFINED;

	if (IS_HEAP(heap)) {
		if (*ppinfo == NULL) {
			/* first call, allocate iterator */
			if (heap->iterator == NULL) {
				heap->iterator = ma_amalloc(sizeof(ma_pinfo_t)+heap->maxnode,heap);
			}
			*ppinfo = heap->iterator;
			(*ppinfo)->size = HEAP_GET_SIZE(heap);
			(*ppinfo)->mempolicy = HEAP_UNSPECIFIED_POLICY;
			(*ppinfo)->weight = HEAP_UNSPECIFIED_WEIGHT;
			(*ppinfo)->maxnode = 0;
			(*ppinfo)->nodemask = heap->iterator+sizeof(ma_pinfo_t);

			/* reset iterator, some node may have changed */
			current_heap = heap;
			while (IS_HEAP(current_heap)) {
				ma_spin_lock(&current_heap->lock_heap);
				if (current_heap->iterator_num != HEAP_ITERATOR_ID_UNDEFINED) {
					current_heap->iterator_num = HEAP_ITERATOR_ID_UNDEFINED;
				}
				ma_spin_unlock(&current_heap->lock_heap);
				current_heap = current_heap->next_heap;
			}
			mdebug_heap("ma_hnext_pinfo: first call, it=%d\n",iterator_num);
		}

		/* set iterator id list */
		current_heap = heap;
		while (IS_HEAP(current_heap)) {
			if (current_heap->iterator_num == HEAP_ITERATOR_ID_UNDEFINED) {
				ma_spin_lock(&current_heap->lock_heap);
				current_heap->iterator_num = iterator_num++;
				current_same_heap = current_heap;
				ma_spin_unlock(&current_heap->lock_heap);
				while(IS_HEAP(current_same_heap)) {
					ma_spin_lock(&current_same_heap->lock_heap);
					current_same_heap->iterator_num = iterator_num;
					ma_spin_unlock(&current_same_heap->lock_heap);
					current_same_heap = current_same_heap->next_same_heap;
				}
			}
			current_heap = current_heap->next_heap;
		}
		mdebug_heap_list("ma_hnext_pinfo list:\n",heap);

		/* goes to next heap higher iterator_num */
		current_heap = ma_aget_heap_from_list((*ppinfo)->mempolicy,(*ppinfo)->weight,(*ppinfo)->nodemask,(*ppinfo)->maxnode,heap);
		if (current_heap == NULL) {
			current_heap = heap;
			iterator_num = HEAP_ITERATOR_ID_UNDEFINED;
		} else {
			iterator_num = current_heap->iterator_num;
		}
		current_heap = heap;
		while (IS_HEAP(current_heap)) {
			if (current_heap->iterator_num == iterator_num + 1) {
				mdebug_heap("next heap %p it=%d\n",current_heap,current_heap->iterator_num);
				break;
			}
			current_heap = current_heap->next_heap;
		}
		if (!IS_HEAP(current_heap)) { /* no more pinfo */
			return 0;
		}
		// set ppinfo
		ma_spin_lock(&current_heap->lock_heap);
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
		ma_spin_unlock(&current_heap->lock_heap);
		return 1;
	}
	return -1;
}

void ma_hupdate_memory_nodes(ma_pinfo_t *ppinfo, ma_heap_t *heap) {
	ma_heap_t* current_heap;
	ma_ub_t* current_bloc_used;
	int i, nb_pages, pagesize, node, nb_attach;
	const void *addr[1];

	if (IS_HEAP(heap)) {
		pagesize = getpagesize();
		for(i = 0; i < marcel_nbnodes; ++i) {
			ppinfo->nb_touched[i] = 0;
		}
		/* look for corresponding heap */
		current_heap = ma_aget_heap_from_list(ppinfo->mempolicy,ppinfo->weight,ppinfo->nodemask,ppinfo->maxnode,heap);

		if (IS_HEAP(current_heap)) {
			while(IS_HEAP(current_heap)) {
				ma_spin_lock(&current_heap->lock_heap);
				nb_pages = HEAP_GET_SIZE(current_heap)/pagesize;

				for(i = 0; i < nb_pages; ++i) {
					/* if page is not touched */
					//if (current_heap->pages[i] == 0)
					//{
					addr[0] = (void*)current_heap+i*pagesize;
					//		addr[0] = (void*)current_heap+0;
					//	mdebug_heap("addr=%p size=%d*%d\n",addr[0],nb_pages,pagesize);
					node = -1;
#ifndef __NR_move_pages
#warning __NR_move_pages unknown
					int ret = -ENOSYS;
#elif defined (X86_64_ARCH) && defined (X86_ARCH)
					extern int syscall6();
					int ret = syscall6(__NR_move_pages, 0, 1, addr, NULL, &node, MPOL_MF_MOVE);
					//	int ret = syscall6(__NR_move_pages, 0, nb_pages, addr, NULL, ppinfo->nb_touched, MPOL_MF_MOVE);
#else
					int ret = syscall(__NR_move_pages, 0, 1, addr, NULL, &node, MPOL_MF_MOVE);
#endif
					//move_pages(0,1,addr,NULL,&node,MPOL_MF_MOVE);
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
						mdebug_heap("page touched %p node=%d\n",addr[0],node);
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
								addr[0] = (void*)current_bloc_used->data+i*pagesize;
								//	addr[0] = (void*)current_bloc_used->data;
#ifndef __NR_move_pages
#warning __NR_move_pages unknown
#else
								syscall(__NR_move_pages, 0, 1, addr, NULL, &node, MPOL_MF_MOVE);
								//	syscall(__NR_move_pages, 0, nb_pages, addr, NULL, ppinfo->nb_touched, MPOL_MF_MOVE);
								//	mdebug_heap("i=%d node=%d\n",i,node);
								//move_pages(0,1,addr,NULL,&node,MPOL_MF_MOVE);
#endif
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
				ma_spin_unlock(&current_heap->lock_heap);
				current_heap = current_heap->next_same_heap;
			}
		}
	}
}

ma_heap_t* ma_hcreate_heap(void) {
	unsigned int pagesize = getpagesize();
	return ma_acreate(HEAP_MINIMUM_PAGES*pagesize, HEAP_DYN_ALLOC);
}

void ma_hdelete_heap(ma_heap_t *heap) {
	ma_adelete(&heap);
}

void ma_hmerge_heap(ma_heap_t *hacc, ma_heap_t *h) {
	ma_heap_t* current_heap;
	ma_heap_t* merging_heap;

	if (IS_HEAP(hacc) && IS_HEAP(h) && hacc != h) {
		current_heap = hacc;
		while (IS_HEAP(current_heap)) {
			ma_spin_lock(&current_heap->lock_heap);
			/* look for last numa heap */
			if (current_heap->next_same_heap == NULL) {
				/* look for matching heap */
				merging_heap = ma_aget_heap_from_list(current_heap->mempolicy,current_heap->weight,current_heap->nodemask,current_heap->maxnode,h);
				if (merging_heap != NULL) {
					ma_aconcat_local_list(current_heap,merging_heap);
				}
			}
			ma_spin_unlock(&current_heap->lock_heap);
			current_heap = current_heap->next_heap;
		}
		ma_aconcat_global_list(hacc,h);
		if (h->iterator != NULL) {
			ma_afree(h->iterator);
		}
	}
}

#endif /* LINUX_SYS */

#endif /* MA__NUMA_MEMORY */
