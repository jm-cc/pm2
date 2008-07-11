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
#include "marcel.h"

typedef struct memory_node_s {
  void *startaddress;
  void *endaddress;
  size_t size;

  void **pageaddrs;
  int nbpages;
  int *nodes;
} memory_node_t;

typedef struct memory_tree_s {
  struct memory_tree_s *leftchild;
  struct memory_tree_s *rightchild;
  memory_node_t *data;
} memory_tree_t;

typedef struct memory_manager_s {
  memory_tree_t *root;
  marcel_spinlock_t lock;
  int pagesize;
} memory_manager_t;

void memory_manager_init(memory_manager_t *memory_manager) {
  memory_manager->root = NULL;
  marcel_spin_init(&(memory_manager->lock), 0);
  memory_manager->pagesize = getpagesize();
}

void memory_manager_create_memory_node(memory_manager_t *memory_manager, void **pageaddrs, int nbpages, size_t size, int *nodes, memory_node_t **memory_node) {
  int i, err;

  *memory_node = malloc(sizeof(memory_node_t));

  // Set the interval addresses and the length
  (*memory_node)->startaddress = pageaddrs[0];
  (*memory_node)->endaddress = pageaddrs[nbpages-1]+memory_manager->pagesize;
  (*memory_node)->size = size;

  // Set the page addresses
  (*memory_node)->nbpages = nbpages;
  (*memory_node)->pageaddrs = malloc((*memory_node)->nbpages * sizeof(void *));
  memcpy((*memory_node)->pageaddrs, pageaddrs, nbpages*sizeof(void*));

  // fill in the nodes
  (*memory_node)->nodes = malloc((*memory_node)->nbpages * sizeof(int));
  if (nodes) {
    memcpy((*memory_node)->nodes, nodes, nbpages*sizeof(int));
  }
  else {
    err = move_pages(0, (*memory_node)->nbpages, (*memory_node)->pageaddrs, NULL, (*memory_node)->nodes, 0);
    if (err < 0) {
      if (errno == ENOSYS) {
        marcel_printf("Warning: Function not implemented. Assume the value 0\n");
      }
      else {
        perror("move_pages");
        exit(-1);
      }
    }
  }

  // Display information
  for(i=0; i<(*memory_node)->nbpages; i++) {
    if ((*memory_node)->nodes[i] == -ENOENT)
      marcel_printf("  page #%d is not allocated\n", i);
    else
      marcel_printf("  page #%d is on node #%d\n", i, (*memory_node)->nodes[i]);
  }
}

void memory_manager_add_internal(memory_manager_t *memory_manager, memory_tree_t **memory_tree, void **pageaddrs, int nbpages, size_t size, int *nodes) {
  if (*memory_tree==NULL) {
    *memory_tree = malloc(sizeof(memory_tree_t));
    (*memory_tree)->leftchild = NULL;
    (*memory_tree)->rightchild = NULL;
    memory_manager_create_memory_node(memory_manager, pageaddrs, nbpages, size, nodes, &((*memory_tree)->data));
  }
  else {
    if (pageaddrs[0] < (*memory_tree)->data->pageaddrs[0])
      memory_manager_add_internal(memory_manager, &((*memory_tree)->leftchild), pageaddrs, nbpages, size, nodes);
    else
      memory_manager_add_internal(memory_manager, &((*memory_tree)->rightchild), pageaddrs, nbpages, size, nodes);
  }
}

void memory_manager_add_with_pages(memory_manager_t *memory_manager, void **pageaddrs, int nbpages, size_t size, int *nodes) {
  marcel_spin_lock(&(memory_manager->lock));
  memory_manager_add_internal(memory_manager, &(memory_manager->root), pageaddrs, nbpages, size, NULL);
  marcel_spin_unlock(&(memory_manager->lock));
}

void memory_manager_add(memory_manager_t *memory_manager, void *address, size_t size) {
  int nbpages, i;
  void **pageaddrs;

  // Count the number of pages
  nbpages = size / memory_manager->pagesize;
  if (nbpages * memory_manager->pagesize != size) nbpages++;

  // Set the page addresses
  pageaddrs = malloc(nbpages * sizeof(void *));
  for(i=0; i<nbpages ; i++) pageaddrs[i] = address + i*memory_manager->pagesize;

  // Add memory
  memory_manager_add_with_pages(memory_manager, pageaddrs, nbpages, size, NULL);
}

void* memory_manager_malloc(memory_manager_t *memory_manager, size_t size) {
  void *ptr;

  ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  if (ptr == MAP_FAILED) {
    perror("mmap");
    exit(-1);
  }

  memory_manager_add(memory_manager, ptr, size);

  return ptr;
}

void* memory_manager_calloc(memory_manager_t *memory_manager, size_t nmemb, size_t size) {
  void *ptr = calloc(nmemb, size);

  memory_manager_add(memory_manager, ptr, nmemb*size);

  return ptr;
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
  else { // the address is stored on the current memory_node
    int offset = address - memory_tree->data->startaddress;
    *node = memory_tree->data->nodes[offset / memory_manager->pagesize];
  }
}

void memory_manager_print(memory_tree_t *memory_tree) {
  if (memory_tree) {
    memory_manager_print(memory_tree->leftchild);
    marcel_printf("[%p, %p]\n", memory_tree->data->startaddress, memory_tree->data->endaddress);
    memory_manager_print(memory_tree->rightchild);
  }
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
  memory_manager_add(&memory_manager, a, 100*sizeof(int));

  buffer2 = malloc(PAGES * memory_manager.pagesize);
  memset(buffer2, 0, PAGES * memory_manager.pagesize);
  for(i=0; i<PAGES; i++) pageaddrs[i] = (buffer2 + i*memory_manager.pagesize);
  memory_manager_add_with_pages(&memory_manager, pageaddrs, PAGES, PAGES * memory_manager.pagesize, NULL);

  buffer3 = malloc(PAGES * memory_manager.pagesize);
  for(i=0; i<PAGES; i++) pageaddrs[i] = (buffer3 + i*memory_manager.pagesize);
  memory_manager_add_with_pages(&memory_manager, pageaddrs, PAGES, PAGES * memory_manager.pagesize, NULL);

  memory_manager_locate(&memory_manager, memory_manager.root, &(buffer[0]), &node);
  marcel_printf("[%d] Address %p is located on node %d\n", marcel_self()->id, &(buffer[0]), node);
  memory_manager_locate(&memory_manager, memory_manager.root, &(buffer3[10000]), &node);
  marcel_printf("[%d] Address %p is located on node %d\n", marcel_self()->id, &(buffer[10000]), node);
}

int marcel_main(int argc, char * argv[]) {
  marcel_t threads[2];
  marcel_attr_t attr;
  char *buffer2;
  int node;

  marcel_init(&argc,argv);
  memory_manager_init(&memory_manager);
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

  memory_manager_print(memory_manager.root);

  buffer2 = malloc(sizeof(char));
  memory_manager_locate(&memory_manager, memory_manager.root, buffer2, &node);
  marcel_printf("Address %p is located on node %d\n", buffer2, node);
}

// TODO: use memalign

