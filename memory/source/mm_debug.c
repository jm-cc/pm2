/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#if defined(MM_MAMI_ENABLED) || defined(MM_HEAP_ENABLED)

#include "pm2_common.h"
#include "mm_debug.h"

debug_type_t debug_memory = NEW_DEBUG_TYPE("MEMORY: ", "memory-debug");
debug_type_t debug_memory_log = NEW_DEBUG_TYPE("MEMORY: ", "memory-log");
debug_type_t debug_memory_ilog = NEW_DEBUG_TYPE("MEMORY: ", "memory-ilog");
debug_type_t debug_memory_warn = NEW_DEBUG_TYPE("MEMORY: ", "memory-warn");

#if defined(MM_HEAP_ENABLED) && defined(PM2DEBUG)
void ma_print_list(const char* str, ma_heap_t* heap) {
	ma_heap_t* h;
	int count = 0;
  	unsigned int i, max_node = (unsigned int) (marcel_nbnodes - 1) ;

	if (debug_memory.show < PM2DEBUG_STDLEVEL) return;

	debug_printf(&debug_memory,"%s",str);
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
#endif /* MM_HEAP_ENABLED && PM2DEBUG */

#endif /* MM_MAMI_ENABLED || MM_HEAP_ENABLED */
