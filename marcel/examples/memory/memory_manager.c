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

#include <sys/mman.h>
#include <numaif.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory_manager.h"

#define printf marcel_printf

void memory_manager_init(memory_manager_t *memory_manager, int initialpreallocatedpages) {
  memory_manager->root = NULL;
  marcel_spin_init(&(memory_manager->lock), 0);
  memory_manager->pagesize = getpagesize();
  memory_manager->initialpreallocatedpages = initialpreallocatedpages;
  memory_manager_prealloc(memory_manager);
}

void memory_manager_create_memory_data(memory_manager_t *memory_manager,
				       void **pageaddrs, int nbpages, size_t size, int *nodes, memory_allocation_mode_t mode,
				       memory_data_t **memory_data) {
  int i, err;

  *memory_data = malloc(sizeof(memory_data_t));

  // Set the interval addresses and the length
  (*memory_data)->startaddress = pageaddrs[0];
  (*memory_data)->endaddress = pageaddrs[nbpages-1]+memory_manager->pagesize;
  (*memory_data)->size = size;
  (*memory_data)->allocation_mode = mode;

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

  // Display information
  for(i=0; i<(*memory_data)->nbpages; i++) {
    if ((*memory_data)->nodes[i] == -ENOENT)
      printf("  page #%d is not allocated\n", i);
    else
      printf("  page #%d is on node #%d\n", i, (*memory_data)->nodes[i]);
  }
}

void memory_manager_delete_tree(memory_manager_t *memory_manager, memory_tree_t **memory_tree) {
  if ((*memory_tree)->leftchild == NULL) {
    memory_tree_t *temp = (*memory_tree);
    (*memory_tree) = (*memory_tree)->rightchild;
    free(temp);
  }
  else if ((*memory_tree)->rightchild == NULL) {
    memory_tree_t *temp = *memory_tree;
    (*memory_tree) = (*memory_tree)->leftchild;
    free(temp);
  }
  else {
    // In-order predecessor (rightmost child of left subtree)
    // Node has two children - get max of left subtree
    memory_tree_t *temp = (*memory_tree)->leftchild; // get left node of the original node

    // find the rightmost child of the subtree of the left node
    while (temp->rightchild != NULL) {
      temp = temp->rightchild;
    }

    // copy the value from the in-order predecessor to the original node
    (*memory_tree)->data = temp->data;

    // then delete the predecessor
    memory_manager_delete_internal(memory_manager, &((*memory_tree)->leftchild), temp->data->pageaddrs[0]);
  }
}

void memory_manager_delete_internal(memory_manager_t *memory_manager, memory_tree_t **memory_tree, void *buffer) {
  if (*memory_tree!=NULL) {
    if (buffer == (*memory_tree)->data->pageaddrs[0]) {
      memory_data_t *data = (*memory_tree)->data;
      // Free memory
      if (data->allocation_mode == MEMORY_ALLOCATION_MALLOC)
	free(buffer);
      else if (data->allocation_mode == MEMORY_ALLOCATION_MMAP)
	munmap(buffer, data->size);
      else if (data->allocation_mode == MEMORY_ALLOCATION_PREALLOC)
	memory_manager_free_from_node(memory_manager, buffer, data->nbpages, data->nodes[0]);

      // Delete corresponding tree
      free(data->pageaddrs);
      free(data->nodes);
      free(data);
      memory_manager_delete_tree(memory_manager, memory_tree);
    }
    else if (buffer < (*memory_tree)->data->pageaddrs[0])
      memory_manager_delete_internal(memory_manager, &((*memory_tree)->leftchild), buffer);
    else
      memory_manager_delete_internal(memory_manager, &((*memory_tree)->rightchild), buffer);
  }
}

void memory_manager_add_internal(memory_manager_t *memory_manager, memory_tree_t **memory_tree,
				 void **pageaddrs, int nbpages, size_t size, int *nodes, memory_allocation_mode_t mode) {
  if (*memory_tree==NULL) {
    *memory_tree = malloc(sizeof(memory_tree_t));
    (*memory_tree)->leftchild = NULL;
    (*memory_tree)->rightchild = NULL;
    memory_manager_create_memory_data(memory_manager, pageaddrs, nbpages, size, nodes, mode, &((*memory_tree)->data));
  }
  else {
    if (pageaddrs[0] < (*memory_tree)->data->pageaddrs[0])
      memory_manager_add_internal(memory_manager, &((*memory_tree)->leftchild), pageaddrs, nbpages, size, nodes, mode);
    else
      memory_manager_add_internal(memory_manager, &((*memory_tree)->rightchild), pageaddrs, nbpages, size, nodes, mode);
  }
}

void memory_manager_add_with_pages(memory_manager_t *memory_manager,
				   void **pageaddrs, int nbpages, size_t size, int *nodes, memory_allocation_mode_t mode) {
  //printf("Adding [%p, %p]\n", pageaddrs[0], pageaddrs[0]+size);
  marcel_spin_lock(&(memory_manager->lock));
  memory_manager_add_internal(memory_manager, &(memory_manager->root), pageaddrs, nbpages, size, NULL, mode);
  marcel_spin_unlock(&(memory_manager->lock));
}

void memory_manager_add(memory_manager_t *memory_manager, void *address, size_t size, memory_allocation_mode_t mode) {
  int nbpages, i;
  void **pageaddrs;

  // Count the number of pages
  nbpages = size / memory_manager->pagesize;
  if (nbpages * memory_manager->pagesize != size) nbpages++;

  // Set the page addresses
  pageaddrs = malloc(nbpages * sizeof(void *));
  for(i=0; i<nbpages ; i++) pageaddrs[i] = address + i*memory_manager->pagesize;

  // Add memory
  memory_manager_add_with_pages(memory_manager, pageaddrs, nbpages, size, NULL, mode);

  free(pageaddrs);
}

void memory_manager_prealloc(memory_manager_t *memory_manager) {
  int node;
  size_t length;

  length = memory_manager->initialpreallocatedpages * memory_manager->pagesize;
  memory_manager->heaps = malloc(marcel_nbnodes * sizeof(memory_heap_t *));
  for(node=0 ; node<marcel_nbnodes ; node++) {
    unsigned long nodemask = (1<<node);
    void *buffer = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    mbind(buffer, length, MPOL_BIND, &nodemask, marcel_nbnodes+2, MPOL_MF_MOVE);
    memset(buffer, 0, length);

    memory_manager->heaps[node] = malloc(sizeof(memory_heap_t));
    memory_manager->heaps[node]->start = buffer;
    memory_manager->heaps[node]->available = malloc(sizeof(memory_space_t));
    memory_manager->heaps[node]->available->start = buffer;
    memory_manager->heaps[node]->available->nbpages = memory_manager->initialpreallocatedpages;
    memory_manager->heaps[node]->available->next = NULL;

    //printf("Preallocating %p for node #%d\n", buffer, node);
  }
}

void* memory_manager_allocate_on_node(memory_manager_t *memory_manager, size_t size, int node) {
  memory_heap_t *heap = memory_manager->heaps[node];
  memory_space_t *prev, *available = NULL;
  void *buffer;
  int nbpages;
  size_t realsize;

  // Round-up the size
  nbpages = size / memory_manager->pagesize;
  if (nbpages * memory_manager->pagesize != size) nbpages++;
  realsize = nbpages * memory_manager->pagesize;

  //printf("Requiring space of %d pages\n", nbpages);

  // Look for a space big enough
  prev = heap->available;
  available = heap->available;
  while (available != NULL) {
    //printf("Current space from %p with %d pages\n", available->start, available->nbpages);
    if (available->nbpages >= nbpages)
      break;
    prev = available;
    available = available->next;
  }
  if (available == NULL) {
    unsigned long nodemask = (1<<node);
    size_t length = memory_manager->initialpreallocatedpages * memory_manager->pagesize;
    void *ptr = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    mbind(ptr, length, MPOL_BIND, &nodemask, marcel_nbnodes+2, MPOL_MF_MOVE);
    memset(ptr, 0, length);

    available = malloc(sizeof(memory_space_t));
    available->start = ptr;
    available->nbpages = memory_manager->initialpreallocatedpages;

    prev->next = available;
  }

  buffer = available->start;
  if (nbpages == available->nbpages) {
    prev->next = available->next;
  }
  else {
    available->start += realsize;
    available->nbpages -= nbpages;
  }

  memory_manager_add(memory_manager, buffer, realsize, MEMORY_ALLOCATION_PREALLOC);

  //printf("Allocating %p on node #%d\n", buffer, node);
  return buffer;
}

void* memory_manager_free_from_node(memory_manager_t *memory_manager, void *buffer, int nbpages, int node) {
  memory_heap_t *heap = memory_manager->heaps[node];

  memory_space_t *available = malloc(sizeof(memory_space_t));
  //printf("Freeing space from %p with %d pages\n", buffer, nbpages);
  available->start = buffer;
  available->nbpages = nbpages;
  available->next = heap->available;
  heap->available = available;
}

void* memory_manager_malloc(memory_manager_t *memory_manager, size_t size) {
  void *ptr;

  ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  if (ptr == MAP_FAILED) {
    perror("mmap");
    exit(-1);
  }

  memory_manager_add(memory_manager, ptr, size, MEMORY_ALLOCATION_MMAP);

  return ptr;
}

void* memory_manager_calloc(memory_manager_t *memory_manager, size_t nmemb, size_t size) {
  void *ptr = calloc(nmemb, size);

  memory_manager_add(memory_manager, ptr, nmemb*size, MEMORY_ALLOCATION_MALLOC);

  return ptr;
}

void memory_manager_free(memory_manager_t *memory_manager, void *buffer) {
  //printf("Removing [%p]\n", buffer);
  marcel_spin_lock(&(memory_manager->lock));
  memory_manager_delete_internal(memory_manager, &(memory_manager->root), buffer);
  marcel_spin_unlock(&(memory_manager->lock));
}

void memory_manager_locate(memory_manager_t *memory_manager, memory_tree_t *memory_tree, void *address, int *node) {
  if (memory_tree==NULL) {
    // We did not find the address
    *node = -1;
  }
  else if (address < memory_tree->data->startaddress) {
    memory_manager_locate(memory_manager, memory_tree->leftchild, address, node);
  }
  else if (address > memory_tree->data->endaddress) {
    memory_manager_locate(memory_manager, memory_tree->rightchild, address, node);
  }
  else { // the address is stored on the current memory_data
    int offset = address - memory_tree->data->startaddress;
    *node = memory_tree->data->nodes[offset / memory_manager->pagesize];
  }
}

void memory_manager_print_aux(memory_tree_t *memory_tree, int indent) {
  if (memory_tree) {
    int x;
    memory_manager_print_aux(memory_tree->leftchild, indent+2);
    for(x=0 ; x<indent ; x++) printf(" ");
    printf("[%p, %p]\n", memory_tree->data->startaddress, memory_tree->data->endaddress);
    memory_manager_print_aux(memory_tree->rightchild, indent+2);
  }
}

void memory_manager_print(memory_tree_t *memory_tree) {
  memory_manager_print_aux(memory_tree, 0);
}

#define PAGES 2
memory_manager_t memory_manager;

any_t memory(any_t arg) {
  int *a, *b, *c, *d, *e;
  char *buffer, *buffer2, *buffer3;
  void *pageaddrs[PAGES];
  int i, node;

  a = malloc(100*sizeof(int));
  b = memory_manager_malloc(&memory_manager, 100*sizeof(int));
  c = memory_manager_malloc(&memory_manager, 100*sizeof(int));
  d = memory_manager_malloc(&memory_manager, 100*sizeof(int));
  e = memory_manager_malloc(&memory_manager, 100*sizeof(int));
  buffer = memory_manager_calloc(&memory_manager, 1, PAGES * memory_manager.pagesize);
  memory_manager_add(&memory_manager, a, 100*sizeof(int), MEMORY_ALLOCATION_MALLOC);

  buffer2 = malloc(PAGES * memory_manager.pagesize);
  memset(buffer2, 0, PAGES * memory_manager.pagesize);
  for(i=0; i<PAGES; i++) pageaddrs[i] = (buffer2 + i*memory_manager.pagesize);
  memory_manager_add_with_pages(&memory_manager, pageaddrs, PAGES, PAGES * memory_manager.pagesize, NULL, MEMORY_ALLOCATION_MALLOC);

  buffer3 = malloc(PAGES * memory_manager.pagesize);
  for(i=0; i<PAGES; i++) pageaddrs[i] = (buffer3 + i*memory_manager.pagesize);
  memory_manager_add_with_pages(&memory_manager, pageaddrs, PAGES, PAGES * memory_manager.pagesize, NULL, MEMORY_ALLOCATION_MALLOC);

  memory_manager_locate(&memory_manager, memory_manager.root, &(buffer[0]), &node);
  printf("[%d] Address %p is located on node %d\n", marcel_self()->id, &(buffer[0]), node);
  memory_manager_locate(&memory_manager, memory_manager.root, &(buffer3[10000]), &node);
  printf("[%d] Address %p is located on node %d\n", marcel_self()->id, &(buffer[10000]), node);

  memory_manager_free(&memory_manager, a);
  memory_manager_free(&memory_manager, c);
  memory_manager_free(&memory_manager, b);
  memory_manager_free(&memory_manager, d);
  memory_manager_free(&memory_manager, e);
  memory_manager_free(&memory_manager, buffer);
  memory_manager_free(&memory_manager, buffer2);
  memory_manager_free(&memory_manager, buffer3);
}

any_t memory2(any_t arg) {
  int *c;
  int i, node;

  memory_manager_print(memory_manager.root);

  c = malloc(sizeof(char));
  memory_manager_locate(&memory_manager, memory_manager.root, c, &node);
  printf("Address %p is located on node %d\n", c, node);
  free(c);

  {
    void **buffers;
    void **buffers2;
    int maxnode = marcel_nbnodes;
    buffers = malloc(maxnode * sizeof(void *));
    buffers2 = malloc(maxnode * sizeof(void *));
    for(i=1 ; i<=5 ; i++) {
      for(node=0 ; node<maxnode ; node++)
	buffers[node] = memory_manager_allocate_on_node(&memory_manager, i*memory_manager.pagesize, node);
      for(node=0 ; node<maxnode ; node++)
      	memory_manager_free(&memory_manager, buffers[node]);
    }
    for(node=0 ; node<maxnode ; node++) {
      buffers[node] = memory_manager_allocate_on_node(&memory_manager, memory_manager.pagesize, node);
      buffers2[node] = memory_manager_allocate_on_node(&memory_manager, memory_manager.pagesize, node);
    }
    for(node=0 ; node<maxnode ; node++) {
      memory_manager_free(&memory_manager, buffers[node]);
      memory_manager_free(&memory_manager, buffers2[node]);
    }
    free(buffers);
    free(buffers2);
  }
}


int marcel_main(int argc, char * argv[]) {
  marcel_t threads[2];
  marcel_attr_t attr;

  marcel_init(&argc,argv);
  memory_manager_init(&memory_manager, 1000);
  marcel_attr_init(&attr);

  // Start the 1st thread on the first VP
  marcel_attr_setid(&attr, 0);
  marcel_attr_settopo_level(&attr, &marcel_topo_vp_level[0]);
  marcel_create(&threads[0], &attr, memory, NULL);

  // Start the 2nd thread on the last VP
  marcel_attr_setid(&attr, 1);
  marcel_attr_settopo_level(&attr, &marcel_topo_vp_level[marcel_nbvps()-1]);
  marcel_create(&threads[1], &attr, memory, NULL);

  // Wait for the threads to complete
  marcel_join(threads[0], NULL);
  marcel_join(threads[1], NULL);

  // Start the thread on the last VP
  marcel_attr_setid(&attr, 1);
  marcel_attr_settopo_level(&attr, &marcel_topo_vp_level[marcel_nbvps()-1]);
  marcel_create(&threads[1], &attr, memory2, NULL);
  marcel_join(threads[1], NULL);

  // Finish marcel
  marcel_end();
}

// TODO: use memalign

