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
#include "marcel_topology.h"

#include <numaif.h>
#include <errno.h>

typedef struct memory_s {
  void *startaddress;
  void *endaddress;
  size_t size;

  void **pageaddrs;
  int nbpages;
  int *nodes;
} memory_t; 

typedef struct bt_memory_s {
  struct bt_memory_s *leftchild;
  struct bt_memory_s *rightchild;
  memory_t *data;
} bt_memory_t;

void new_memory_with_pages(memory_t **memory, void **pageaddrs, int nbpages, int *nodes) {
  int i, err;

  *memory = malloc(sizeof(memory_t));

  // Set the interval addresses and the length
  (*memory)->startaddress = pageaddrs[0];
  (*memory)->endaddress = pageaddrs[nbpages-1]+getpagesize();
  (*memory)->size = nbpages * getpagesize();

  // Set the page addresses
  (*memory)->nbpages = nbpages;
  (*memory)->pageaddrs = malloc((*memory)->nbpages * sizeof(void *));
  memcpy((*memory)->pageaddrs, pageaddrs, nbpages*sizeof(void*));

  // fill in the nodes
  (*memory)->nodes = malloc((*memory)->nbpages * sizeof(int));
  if (nodes) {
    memcpy((*memory)->nodes, nodes, nbpages*sizeof(int));
  }
  else {
    err = move_pages(0, (*memory)->nbpages, (*memory)->pageaddrs, NULL, (*memory)->nodes, 0);
    if (err < 0) {
      perror("move_pages");
      exit(-1);
    }
  }

  // Display information
  for(i=0; i<(*memory)->nbpages; i++) {
    if ((*memory)->nodes[i] == -ENOENT)
      marcel_printf("  page #%d is not allocated\n", i);
    else
      marcel_printf("  page #%d is on node #%d\n", i, (*memory)->nodes[i]);
  }
}

void add_memory_with_pages(bt_memory_t **memory_node, void **pageaddrs, int nbpages, int *nodes) {
  if (*memory_node==NULL) {
    *memory_node = malloc(sizeof(bt_memory_t));
    (*memory_node)->leftchild = NULL;
    (*memory_node)->rightchild = NULL;
    new_memory_with_pages(&((*memory_node)->data), pageaddrs, nbpages, nodes);
  }
  else {
    if (pageaddrs[0] < (*memory_node)->data->pageaddrs[0])
      add_memory_with_pages(&((*memory_node)->leftchild), pageaddrs, nbpages, nodes);
    else
      add_memory_with_pages(&((*memory_node)->rightchild), pageaddrs, nbpages, nodes);
  }
}

void add_memory(bt_memory_t **memory_node, void *address, size_t size) {
  int nbpages, i;
  void **pageaddrs;

  // Count the number of pages
  nbpages = size / getpagesize();
  if (nbpages * getpagesize() != size) nbpages++;

  // Set the page addresses
  pageaddrs = malloc(nbpages * sizeof(void *));
  for(i=0; i<nbpages ; i++) pageaddrs[i] = address + i*getpagesize();

  // Add memory
  add_memory_with_pages(memory_node, pageaddrs, nbpages, NULL);
}

void locate_memory(bt_memory_t *memory_node, void *address, int *node) {
  if (memory_node==NULL) {
    // We did not find the address
    *node = -1;
  }
  else if (address < memory_node->data->startaddress) {
    locate_memory(memory_node->leftchild, address, node);
  }
  else if (address > memory_node->data->endaddress) {
    locate_memory(memory_node->rightchild, address, node);
  }
  else { // the address is stored on the current memory_node
    int offset = address - memory_node->data->startaddress;
    *node = memory_node->data->nodes[offset / getpagesize()];
  }
}

void print_memory(bt_memory_t *memory_node) {
  if (memory_node) {
    print_memory(memory_node->leftchild);
    marcel_printf("[%p, %p]\n", memory_node->data->startaddress, memory_node->data->endaddress);
    print_memory(memory_node->rightchild);
  }
}

#define PAGES 5

int marcel_main(int argc, char * argv[]) {
  bt_memory_t *memory_root = NULL;
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
