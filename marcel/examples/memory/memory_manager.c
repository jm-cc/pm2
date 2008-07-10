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
  int *nodes;
  unsigned long pagesize;
  void **pageaddrs;
  int nbpages;
} memory_t; 

typedef struct bt_memory_s {
  struct bt_memory_s *leftchild;
  struct bt_memory_s *rightchild;
  memory_t *data;
} bt_memory_t;

void new_memory(memory_t **memory, void *address, size_t size) {
  int i, err;

  *memory = malloc(sizeof(memory_t));

  // Count the number of pages
  (*memory)->pagesize = getpagesize();
  (*memory)->nbpages = size / (*memory)->pagesize;
  if ((*memory)->nbpages * (*memory)->pagesize != size) (*memory)->nbpages++;

  // Set the page addresses
  (*memory)->pageaddrs = malloc((*memory)->nbpages * sizeof(void *));
  for(i=0; i<(*memory)->nbpages ; i++)
    (*memory)->pageaddrs[i] = address + i*(*memory)->pagesize;

  // fill in the nodes
  (*memory)->nodes = malloc((*memory)->nbpages * sizeof(int));
  err = move_pages(0, (*memory)->nbpages, (*memory)->pageaddrs, NULL, (*memory)->nodes, 0);
  if (err < 0) {
    perror("move_pages");
    exit(-1);
  }

  // Display information
  for(i=0; i<(*memory)->nbpages; i++) {
    if ((*memory)->nodes[i] == -ENOENT)
      marcel_printf("  page #%d is not allocated\n", i);
    else
      marcel_printf("  page #%d is on node #%d\n", i, (*memory)->nodes[i]);
  }
}

void add_memory(bt_memory_t **memory_node, void *address, size_t size) {
  if (*memory_node==NULL) {
    *memory_node = malloc(sizeof(bt_memory_t));
    (*memory_node)->leftchild = NULL;
    (*memory_node)->rightchild = NULL;
    new_memory(&((*memory_node)->data), address, size);
  }
  else {
    if (address < (*memory_node)->data->pageaddrs[0])
      add_memory(&((*memory_node)->leftchild), address, size);
    else
      add_memory(&((*memory_node)->rightchild), address, size);
  }
}

void print_memory(bt_memory_t *memory_node) {
  if (memory_node) {
    print_memory(memory_node->leftchild);
    marcel_printf("%d\n", memory_node->data->pageaddrs[0]);
    print_memory(memory_node->rightchild);
  }
}

int marcel_main(int argc, char * argv[]) {
  bt_memory_t *memory_root = NULL;
  int *a, *b, *c, *d, *e;

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

  print_memory(memory_root);
}
