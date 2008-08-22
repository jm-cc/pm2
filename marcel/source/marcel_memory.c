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

#include "marcel.h"
#include <errno.h>
#include <sys/mman.h>
#include <numaif.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void marcel_memory_init(marcel_memory_manager_t *memory_manager, int initialpreallocatedpages) {
  int node;

  LOG_IN();
  memory_manager->root = NULL;
  marcel_spin_init(&(memory_manager->lock), 0);
  memory_manager->pagesize = getpagesize();
  memory_manager->initialpreallocatedpages = initialpreallocatedpages;

  memory_manager->heaps = malloc(marcel_nbnodes * sizeof(marcel_memory_space_t *));
  for(node=0 ; node<marcel_nbnodes ; node++) {
    ma_memory_preallocate(memory_manager, &(memory_manager->heaps[node]), node);
    mdebug_heap("Preallocating %p for node #%d\n", memory_manager->heaps[node]->start, node);
  }

  LOG_OUT();
}

void ma_memory_init_memory_data(marcel_memory_manager_t *memory_manager,
				void **pageaddrs, int nbpages, size_t size, int *nodes,
				marcel_memory_data_t **memory_data) {
  int err;

  LOG_IN();

  *memory_data = malloc(sizeof(marcel_memory_data_t));

  // Set the interval addresses and the length
  (*memory_data)->startaddress = pageaddrs[0];
  (*memory_data)->endaddress = pageaddrs[nbpages-1]+memory_manager->pagesize;
  (*memory_data)->size = size;

  // Set the page addresses
  (*memory_data)->nbpages = nbpages;
  (*memory_data)->pageaddrs = malloc((*memory_data)->nbpages * sizeof(void *));
  memcpy((*memory_data)->pageaddrs, pageaddrs, nbpages*sizeof(void*));

  // fill in the nodes
  (*memory_data)->nodes = malloc((*memory_data)->nbpages * sizeof(int));
  if (nodes) {
    memcpy((*memory_data)->nodes, nodes, nbpages*sizeof(int));
  }
  else {
    err = move_pages(0, (*memory_data)->nbpages, (*memory_data)->pageaddrs, NULL, (*memory_data)->nodes, 0);
    if (err < 0) {
      if (errno == ENOSYS) {
	printf("Warning: Function not implemented. Assume the value 0\n");
      }
      else {
        perror("move_pages");
        exit(-1);
      }
    }
  }

#ifdef PM2DEBUG
 {
   int i;
   // Display information
   for(i=0; i<(*memory_data)->nbpages; i++) {
     if ((*memory_data)->nodes[i] == -ENOENT)
       mdebug_heap("  page #%d is not allocated\n", i);
     else
       mdebug_heap("  page #%d is on node #%d\n", i, (*memory_data)->nodes[i]);
   }
 }
#endif

  LOG_OUT();
}

void ma_memory_delete_tree(marcel_memory_manager_t *memory_manager, marcel_memory_tree_t **memory_tree) {
  LOG_IN();
  if ((*memory_tree)->leftchild == NULL) {
    //marcel_memory_tree_t *temp = (*memory_tree);
    (*memory_tree) = (*memory_tree)->rightchild;
    //free(temp);
  }
  else if ((*memory_tree)->rightchild == NULL) {
    //marcel_memory_tree_t *temp = *memory_tree;
    (*memory_tree) = (*memory_tree)->leftchild;
    //free(temp);
  }
  else {
    // In-order predecessor (rightmost child of left subtree)
    // Node has two children - get max of left subtree
    marcel_memory_tree_t *temp = (*memory_tree)->leftchild; // get left node of the original node

    // find the rightmost child of the subtree of the left node
    while (temp->rightchild != NULL) {
      temp = temp->rightchild;
    }

    // copy the value from the in-order predecessor to the original node
    (*memory_tree)->data = temp->data;

    // then delete the predecessor
    ma_memory_delete(memory_manager, &((*memory_tree)->leftchild), temp->data->pageaddrs[0]);
  }
  LOG_OUT();
}

void ma_memory_delete(marcel_memory_manager_t *memory_manager, marcel_memory_tree_t **memory_tree, void *buffer) {
  LOG_IN();
  if (*memory_tree!=NULL) {
    if (buffer == (*memory_tree)->data->pageaddrs[0]) {
      marcel_memory_data_t *data = (*memory_tree)->data;
      mdebug_heap("Removing [%p, %p]\n", (*memory_tree)->data->pageaddrs[0], (*memory_tree)->data->pageaddrs[0]+(*memory_tree)->data->size);
      // Free memory
      ma_memory_free_from_node(memory_manager, buffer, data->nbpages, data->nodes[0]);

      // Delete corresponding tree
//      free(data->pageaddrs);
//      free(data->nodes);
//      free(data);
      ma_memory_delete_tree(memory_manager, memory_tree);
    }
    else if (buffer < (*memory_tree)->data->pageaddrs[0])
      ma_memory_delete(memory_manager, &((*memory_tree)->leftchild), buffer);
    else
      ma_memory_delete(memory_manager, &((*memory_tree)->rightchild), buffer);
  }
  LOG_OUT();
}

void ma_memory_register(marcel_memory_manager_t *memory_manager, marcel_memory_tree_t **memory_tree,
			void **pageaddrs, int nbpages, size_t size, int *nodes) {
  LOG_IN();
  if (*memory_tree==NULL) {
    *memory_tree = malloc(sizeof(marcel_memory_tree_t));
    (*memory_tree)->leftchild = NULL;
    (*memory_tree)->rightchild = NULL;
    ma_memory_init_memory_data(memory_manager, pageaddrs, nbpages, size, nodes, &((*memory_tree)->data));
  }
  else {
    if (pageaddrs[0] < (*memory_tree)->data->pageaddrs[0])
      ma_memory_register(memory_manager, &((*memory_tree)->leftchild), pageaddrs, nbpages, size, nodes);
    else
      ma_memory_register(memory_manager, &((*memory_tree)->rightchild), pageaddrs, nbpages, size, nodes);
  }
  LOG_OUT();
}

void ma_memory_preallocate(marcel_memory_manager_t *memory_manager, marcel_memory_space_t **space, int node) {
  unsigned long nodemask;
  size_t length;
  void *buffer;

  nodemask = (1<<node);
  length = memory_manager->initialpreallocatedpages * memory_manager->pagesize;

  buffer = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  mbind(buffer, length, MPOL_BIND, &nodemask, marcel_nbnodes+2, MPOL_MF_MOVE);
  memset(buffer, 0, length);

  (*space) = malloc(sizeof(marcel_memory_space_t));
  (*space)->start = buffer;
  (*space)->nbpages = memory_manager->initialpreallocatedpages;
  (*space)->next = NULL;
}

void* marcel_memory_allocate_on_node(marcel_memory_manager_t *memory_manager, size_t size, int node) {
  marcel_memory_space_t *heap = memory_manager->heaps[node];
  marcel_memory_space_t *prev = NULL;
  void *buffer;
  int i, nbpages;
  size_t realsize;
  void **pageaddrs;
  int *nodes;

  LOG_IN();

  marcel_spin_lock(&(memory_manager->lock));

  // Round-up the size
  nbpages = size / memory_manager->pagesize;
  if (nbpages * memory_manager->pagesize != size) nbpages++;
  realsize = nbpages * memory_manager->pagesize;

  mdebug_heap("Requiring space of %d pages from heap %p on node %d\n", nbpages, heap, node);

  // Look for a space big enough
  prev = heap;
  while (heap != NULL) {
    mdebug_heap("Current space from %p with %d pages\n", heap->start, heap->nbpages);
    if (heap->nbpages >= nbpages)
      break;
    prev = heap;
    heap = heap->next;
  }
  if (heap == NULL) {
    mdebug_heap("not enough space, let's allocate some more\n");
    ma_memory_preallocate(memory_manager, &heap, node);

    if (prev == NULL) {
      memory_manager->heaps[node] = heap;
      prev = heap;
    }
    else {
      prev->next = heap;
    }
  }

  buffer = heap->start;
  if (nbpages == heap->nbpages) {
    if (prev == heap)
      memory_manager->heaps[node] = NULL;
    else
      prev->next = heap->next;
  }
  else {
    heap->start += realsize;
    heap->nbpages -= nbpages;
  }

  // Set the page addresses and the node location
  pageaddrs = malloc(nbpages * sizeof(void *));
  for(i=0; i<nbpages ; i++) pageaddrs[i] = buffer + i*memory_manager->pagesize;
  nodes = malloc(nbpages * sizeof(int));
  for(i=0; i<nbpages ; i++) nodes[i] = node;

  // Register memory
  mdebug_heap("Registering [%p, %p]\n", pageaddrs[0], pageaddrs[0]+size);
  ma_memory_register(memory_manager, &(memory_manager->root), pageaddrs, nbpages, size, nodes);

  free(pageaddrs);
  free(nodes);

  marcel_spin_unlock(&(memory_manager->lock));
  mdebug_heap("Allocating %p on node #%d\n", buffer, node);
  LOG_OUT();
  return buffer;
}

void ma_memory_free_from_node(marcel_memory_manager_t *memory_manager, void *buffer, int nbpages, int node) {
  marcel_memory_space_t *available;

  LOG_IN();

  mdebug_heap("Freeing space from %p with %d pages\n", buffer, nbpages);
  available = malloc(sizeof(marcel_memory_space_t));
  available->start = buffer;
  available->nbpages = nbpages;
  available->next = memory_manager->heaps[node];
  memory_manager->heaps[node] = available;

  LOG_OUT();
}

void* marcel_memory_malloc(marcel_memory_manager_t *memory_manager, size_t size) {
  int numanode;
  void *ptr;

  LOG_IN();

  numanode = marcel_current_node();
  if (tbx_unlikely(numanode == -1)) numanode=0;
  ptr = marcel_memory_allocate_on_node(memory_manager, size, numanode);

  LOG_OUT();
  return ptr;
}

void* marcel_memory_calloc(marcel_memory_manager_t *memory_manager, size_t nmemb, size_t size) {
  int numanode;
  void *ptr;

  LOG_IN();

  numanode = marcel_current_node();
  if (tbx_unlikely(numanode == -1)) numanode=0;
  ptr = marcel_memory_allocate_on_node(memory_manager, nmemb*size, numanode);

  LOG_OUT();
  return ptr;
}

void marcel_memory_free(marcel_memory_manager_t *memory_manager, void *buffer) {
  LOG_IN();
  mdebug_heap("Freeing [%p]\n", buffer);
  marcel_spin_lock(&(memory_manager->lock));
  ma_memory_delete(memory_manager, &(memory_manager->root), buffer);
  marcel_spin_unlock(&(memory_manager->lock));
  LOG_OUT();
}

void ma_memory_locate(marcel_memory_manager_t *memory_manager, marcel_memory_tree_t *memory_tree, void *address, int *node) {
 LOG_IN();
  if (memory_tree==NULL) {
    // We did not find the address
    *node = -1;
  }
  else if (address < memory_tree->data->startaddress) {
    ma_memory_locate(memory_manager, memory_tree->leftchild, address, node);
  }
  else if (address > memory_tree->data->endaddress) {
    ma_memory_locate(memory_manager, memory_tree->rightchild, address, node);
  }
  else { // the address is stored on the current memory_data
    int offset = address - memory_tree->data->startaddress;
    *node = memory_tree->data->nodes[offset / memory_manager->pagesize];
  }
  LOG_OUT();
}

void marcel_memory_locate(marcel_memory_manager_t *memory_manager, void *address, int *node) {
  ma_memory_locate(memory_manager, memory_manager->root, address, node);
}

void ma_memory_print(marcel_memory_tree_t *memory_tree, int indent) {
  if (memory_tree) {
    int x;
    ma_memory_print(memory_tree->leftchild, indent+2);
    for(x=0 ; x<indent ; x++) printf(" ");
    printf("[%p, %p]\n", memory_tree->data->startaddress, memory_tree->data->endaddress);
    ma_memory_print(memory_tree->rightchild, indent+2);
  }
}

void marcel_memory_print(marcel_memory_manager_t *memory_manager) {
  LOG_IN();
  mdebug_heap("******************** TREE BEGIN *********************************\n");
  ma_memory_print(memory_manager->root, 0);
  mdebug_heap("******************** TREE END *********************************\n");
  LOG_OUT();
}


