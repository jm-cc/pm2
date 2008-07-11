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

void new_memory_with_pages(memory_node_t **memory_node, void **pageaddrs, int nbpages, int *nodes) {
  int i, err;

  *memory_node = malloc(sizeof(memory_node_t));

  // Set the interval addresses and the length
  (*memory_node)->startaddress = pageaddrs[0];
  (*memory_node)->endaddress = pageaddrs[nbpages-1]+getpagesize();
  (*memory_node)->size = nbpages * getpagesize();

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

void add_memory_with_pages(memory_tree_t **memory_tree, void **pageaddrs, int nbpages, int *nodes) {
  if (*memory_tree==NULL) {
    *memory_tree = malloc(sizeof(memory_tree_t));
    (*memory_tree)->leftchild = NULL;
    (*memory_tree)->rightchild = NULL;
    new_memory_with_pages(&((*memory_tree)->data), pageaddrs, nbpages, nodes);
  }
  else {
    if (pageaddrs[0] < (*memory_tree)->data->pageaddrs[0])
      add_memory_with_pages(&((*memory_tree)->leftchild), pageaddrs, nbpages, nodes);
    else
      add_memory_with_pages(&((*memory_tree)->rightchild), pageaddrs, nbpages, nodes);
  }
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
  add_memory_with_pages(memory_tree, pageaddrs, nbpages, NULL);
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

int marcel_main(int argc, char * argv[]) {
  memory_tree_t *memory_root = NULL;
  int *a, *b, *c, *d, *e;
  char *buffer, *buffer2;
  void *pageaddrs[PAGES];
  int i, node;

  marcel_init(&argc,argv);

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
  for(i=0; i<PAGES; i++) pageaddrs[i] = (buffer + i*getpagesize());
  add_memory_with_pages(&memory_root, pageaddrs, PAGES, NULL);

  print_memory(memory_root);

  locate_memory(memory_root, &(buffer[0]), &node);
  marcel_printf("Address %p is located on node %d\n", &(buffer[0]), node);
  locate_memory(memory_root, &(buffer[10000]), &node);
  marcel_printf("Address %p is located on node %d\n", &(buffer[10000]), node);

  buffer2 = malloc(sizeof(char));
  locate_memory(memory_root, buffer2, &node);
  marcel_printf("Address %p is located on node %d\n", buffer2, node);
}
