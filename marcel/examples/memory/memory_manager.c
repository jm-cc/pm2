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

memory_tree_t *memory_root = NULL;
marcel_spinlock_t memory_lock;

void new_memory_with_pages(memory_node_t **memory_node, void **pageaddrs, int nbpages, size_t size, int *nodes) {
  int i, err;

  *memory_node = malloc(sizeof(memory_node_t));

  // Set the interval addresses and the length
  (*memory_node)->startaddress = pageaddrs[0];
  (*memory_node)->endaddress = pageaddrs[nbpages-1]+getpagesize();
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

void add_memory_internal(memory_tree_t **memory_tree, void **pageaddrs, int nbpages, size_t size, int *nodes) {
  if (*memory_tree==NULL) {
    *memory_tree = malloc(sizeof(memory_tree_t));
    (*memory_tree)->leftchild = NULL;
    (*memory_tree)->rightchild = NULL;
    new_memory_with_pages(&((*memory_tree)->data), pageaddrs, nbpages, size, nodes);
  }
  else {
    if (pageaddrs[0] < (*memory_tree)->data->pageaddrs[0])
      add_memory_internal(&((*memory_tree)->leftchild), pageaddrs, nbpages, size, nodes);
    else
      add_memory_internal(&((*memory_tree)->rightchild), pageaddrs, nbpages, size, nodes);
  }
}

void add_memory_with_pages(memory_tree_t **memory_tree, void **pageaddrs, int nbpages, size_t size, int *nodes) {
  marcel_spin_lock(&memory_lock);
  add_memory_internal(memory_tree, pageaddrs, nbpages, size, NULL);
  marcel_spin_unlock(&memory_lock);
}

void add_memory(memory_tree_t **memory_tree, void *address, size_t size) {
  int nbpages, i;
  void **pageaddrs;

  // Count the number of pages
  nbpages = size / getpagesize();
  if (nbpages * getpagesize() != size) nbpages++;

  // Set the page addresses
  pageaddrs = malloc(nbpages * sizeof(void *));
  for(i=0; i<nbpages ; i++) pageaddrs[i] = address + i*getpagesize();

  // Add memory
  add_memory_with_pages(memory_tree, pageaddrs, nbpages, size, NULL);
}

void locate_memory(memory_tree_t *memory_tree, void *address, int *node) {
  if (memory_tree==NULL) {
    // We did not find the address
    *node = -1;
  }
  else if (address < memory_tree->data->startaddress) {
    locate_memory(memory_tree->leftchild, address, node);
  }
  else if (address > memory_tree->data->endaddress) {
    locate_memory(memory_tree->rightchild, address, node);
  }
  else { // the address is stored on the current memory_node
    int offset = address - memory_tree->data->startaddress;
    *node = memory_tree->data->nodes[offset / getpagesize()];
  }
}

void print_memory(memory_tree_t *memory_tree) {
  if (memory_tree) {
    print_memory(memory_tree->leftchild);
    marcel_printf("[%p, %p]\n", memory_tree->data->startaddress, memory_tree->data->endaddress);
    print_memory(memory_tree->rightchild);
  }
}

#define PAGES 5
any_t memory(any_t arg) {
  int *a, *b, *c, *d, *e;
  char *buffer, *buffer2;
  void *pageaddrs[PAGES];
  int i, node;

  a = malloc(100*sizeof(int));
  b = malloc(100*sizeof(int));
  c = malloc(100*sizeof(int));
  d = malloc(100*sizeof(int));
  e = malloc(100*sizeof(int));

  add_memory(&memory_root, e, 100*sizeof(int));
  add_memory(&memory_root, b, 100*sizeof(int));
  add_memory(&memory_root, a, 100*sizeof(int));
  add_memory(&memory_root, c, 100*sizeof(int));
  add_memory(&memory_root, d, 100*sizeof(int));

  buffer = malloc(PAGES * getpagesize());
  memset(buffer, 0, PAGES * getpagesize());
  for(i=0; i<PAGES; i++) pageaddrs[i] = (buffer + i*getpagesize());
  add_memory_with_pages(&memory_root, pageaddrs, PAGES, PAGES * getpagesize(), NULL);

  buffer2 = malloc(PAGES * getpagesize());
  for(i=0; i<PAGES; i++) pageaddrs[i] = (buffer2 + i*getpagesize());
  add_memory_with_pages(&memory_root, pageaddrs, PAGES, PAGES * getpagesize(), NULL);

  locate_memory(memory_root, &(buffer[0]), &node);
  marcel_printf("[%d] Address %p is located on node %d\n", marcel_self()->id, &(buffer[0]), node);
  locate_memory(memory_root, &(buffer2[10000]), &node);
  marcel_printf("[%d] Address %p is located on node %d\n", marcel_self()->id, &(buffer[10000]), node);
}

int marcel_main(int argc, char * argv[]) {
  marcel_t threads[2];
  marcel_attr_t attr;
  char *buffer2;
  int node;

  marcel_init(&argc,argv);
  marcel_spin_init(&memory_lock, 0);
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

  print_memory(memory_root);

  buffer2 = malloc(sizeof(char));
  locate_memory(memory_root, buffer2, &node);
  marcel_printf("Address %p is located on node %d\n", buffer2, node);
}

// TODO: use memalign

