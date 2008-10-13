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

#ifdef MARCEL_MAMI_ENABLED

#include "marcel.h"
#include <errno.h>
#include <numaif.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern long move_pages(int pid, unsigned long count,
                       void **pages, const int *nodes, int *status, int flags);

void marcel_memory_init(marcel_memory_manager_t *memory_manager, int preallocatedpages) {
  int node, dest;

  LOG_IN();
  memory_manager->root = NULL;
  marcel_spin_init(&(memory_manager->lock), 0);
  memory_manager->pagesize = getpagesize();
  memory_manager->initially_preallocated_pages = preallocatedpages;
  memory_manager->cache_line_size = 64;
  memory_manager->membind_policy = MARCEL_MEMORY_MEMBIND_POLICY_NONE;

  // Preallocate memory on each node
  memory_manager->heaps = malloc(marcel_nbnodes * sizeof(marcel_memory_area_t *));
  for(node=0 ; node<marcel_nbnodes ; node++) {
    ma_memory_preallocate(memory_manager, &(memory_manager->heaps[node]), node);
    mdebug_heap("Preallocating %p for node #%d\n", memory_manager->heaps[node]->start, node);
  }

  // Load the model for the migration costs
  memory_manager->migration_costs = (p_tbx_slist_t **) malloc(marcel_nbnodes * sizeof(p_tbx_slist_t *));
  for(node=0 ; node<marcel_nbnodes ; node++) {
    memory_manager->migration_costs[node] = (p_tbx_slist_t *) malloc(marcel_nbnodes * sizeof(p_tbx_slist_t));
    for(dest=0 ; dest<marcel_nbnodes ; dest++) {
      memory_manager->migration_costs[node][dest] = tbx_slist_nil();
    }
  }
  ma_memory_load_model_for_memory_migration(memory_manager);

  // Load the model for the access costs
  memory_manager->writing_access_costs = (marcel_access_cost_t **) malloc(marcel_nbnodes * sizeof(marcel_access_cost_t *));
  for(node=0 ; node<marcel_nbnodes ; node++) {
    memory_manager->writing_access_costs[node] = (marcel_access_cost_t *) malloc(marcel_nbnodes * sizeof(marcel_access_cost_t));
  }
  memory_manager->reading_access_costs = (marcel_access_cost_t **) malloc(marcel_nbnodes * sizeof(marcel_access_cost_t *));
  for(node=0 ; node<marcel_nbnodes ; node++) {
    memory_manager->reading_access_costs[node] = (marcel_access_cost_t *) malloc(marcel_nbnodes * sizeof(marcel_access_cost_t));
  }
  ma_memory_load_model_for_memory_access(memory_manager);

#ifdef PM2DEBUG
  if (marcel_heap_debug.show > PM2DEBUG_STDLEVEL) {
    for(node=0 ; node<marcel_nbnodes ; node++) {
      for(dest=0 ; dest<marcel_nbnodes ; dest++) {
        p_tbx_slist_t migration_costs = memory_manager->migration_costs[node][dest];
        if (!tbx_slist_is_nil(migration_costs)) {
          tbx_slist_ref_to_head(migration_costs);
          do {
            marcel_memory_migration_cost_t *object = NULL;
            object = tbx_slist_ref_get(migration_costs);

            marcel_printf("[%d:%d] [%ld:%ld] %f %f %f\n", node, dest, (long)object->size_min, (long)object->size_max, object->slope, object->intercept, object->correlation);
          } while (tbx_slist_ref_forward(migration_costs));
        }
      }
    }

    for(node=0 ; node<marcel_nbnodes ; node++) {
      for(dest=0 ; dest<marcel_nbnodes ; dest++) {
        marcel_access_cost_t wcost = memory_manager->writing_access_costs[node][dest];
        marcel_access_cost_t rcost = memory_manager->writing_access_costs[node][dest];
        marcel_printf("[%d:%d] %f %f\n", node, dest, wcost.cost, rcost.cost);
      }
    }
  }
#endif /* PM2DEBUG */

  LOG_OUT();
}

static
void ma_memory_sampling_free(p_tbx_slist_t migration_costs) {
  LOG_IN();
  while (tbx_slist_is_nil(migration_costs) == tbx_false) {
    marcel_memory_migration_cost_t *ptr = tbx_slist_extract(migration_costs);
    free(ptr);
  }
  tbx_slist_clear(migration_costs);
  tbx_slist_free(migration_costs);
  LOG_OUT();
}


void marcel_memory_exit(marcel_memory_manager_t *memory_manager) {
  int node, dest;

  LOG_IN();

  for(node=0 ; node<marcel_nbnodes ; node++) {
    ma_memory_deallocate(memory_manager, &(memory_manager->heaps[node]), node);
  }
  free(memory_manager->heaps);

  for(node=0 ; node<marcel_nbnodes ; node++) {
    for(dest=0 ; dest<marcel_nbnodes ; dest++) {
      ma_memory_sampling_free(memory_manager->migration_costs[node][dest]);
    }
    free(memory_manager->migration_costs[node]);
  }
  free(memory_manager->migration_costs);

  LOG_OUT();
}

static
void ma_memory_init_memory_data(marcel_memory_manager_t *memory_manager,
                                void **pageaddrs, int nbpages, size_t size, int *nodes, int protection,
                                marcel_memory_data_t **memory_data) {
  int err;

  LOG_IN();

  *memory_data = malloc(sizeof(marcel_memory_data_t));

  // Set the interval addresses and the length
  (*memory_data)->startaddress = pageaddrs[0];
  (*memory_data)->endaddress = pageaddrs[nbpages-1]+memory_manager->pagesize;
  (*memory_data)->size = size;
  (*memory_data)->status = MARCEL_MEMORY_INITIAL_STATUS;
  (*memory_data)->protection = protection;

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
	marcel_printf("Warning: Function not implemented. Assume the value 0\n");
      }
      else {
        perror("move_pages");
        exit(-1);
      }
    }
  }
  mdebug_heap("Location of the new pages: %d\n", (*memory_data)->nodes[0]);

//#ifdef PM2DEBUG
// {
//   int i;
//   // Display information
//   for(i=0; i<(*memory_data)->nbpages; i++) {
//     if ((*memory_data)->nodes[i] == -ENOENT)
//       mdebug_heap("  page #%d is not allocated\n", i);
//     else
//       mdebug_heap("  page #%d is on node #%d\n", i, (*memory_data)->nodes[i]);
//   }
// }
//#endif

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

static
void ma_memory_register(marcel_memory_manager_t *memory_manager, marcel_memory_tree_t **memory_tree,
			void **pageaddrs, int nbpages, size_t size, int *nodes, int protection) {
  LOG_IN();
  if (*memory_tree==NULL) {
    *memory_tree = malloc(sizeof(marcel_memory_tree_t));
    (*memory_tree)->leftchild = NULL;
    (*memory_tree)->rightchild = NULL;
    ma_memory_init_memory_data(memory_manager, pageaddrs, nbpages, size, nodes, protection, &((*memory_tree)->data));
  }
  else {
    if (pageaddrs[0] < (*memory_tree)->data->pageaddrs[0])
      ma_memory_register(memory_manager, &((*memory_tree)->leftchild), pageaddrs, nbpages, size, nodes, protection);
    else
      ma_memory_register(memory_manager, &((*memory_tree)->rightchild), pageaddrs, nbpages, size, nodes, protection);
  }
  LOG_OUT();
}

void ma_memory_preallocate(marcel_memory_manager_t *memory_manager, marcel_memory_area_t **space, int node) {
  unsigned long nodemask;
  size_t length;
  void *buffer;
  int err;

  LOG_IN();

  nodemask = (1<<node);
  length = memory_manager->initially_preallocated_pages * memory_manager->pagesize;

  buffer = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  err = mbind(buffer, length, MPOL_BIND, &nodemask, marcel_nbnodes+2, MPOL_MF_MOVE);
  if (err < 0) {
    perror("mbind");
  }
  buffer = memset(buffer, 0, length);

  (*space) = malloc(sizeof(marcel_memory_area_t));
  (*space)->start = buffer;
  (*space)->nbpages = memory_manager->initially_preallocated_pages;
  (*space)->protection = PROT_READ|PROT_WRITE;
  (*space)->next = NULL;

  LOG_OUT();
}

void ma_memory_deallocate(marcel_memory_manager_t *memory_manager, marcel_memory_area_t **space, int node) {
  marcel_memory_area_t *ptr;

  LOG_IN();
  ptr  = (*space);
  while (ptr != NULL) {
    mdebug_heap("Unmapping memory area from %p\n", ptr->start);
    munmap(ptr->start, ptr->nbpages * memory_manager->pagesize);
    ptr = ptr->next;
  }
  LOG_OUT();
}

void* marcel_memory_allocate_on_node(marcel_memory_manager_t *memory_manager, size_t size, int node) {
  marcel_memory_area_t *heap = memory_manager->heaps[node];
  marcel_memory_area_t *prev = NULL;
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
  ma_memory_register(memory_manager, &(memory_manager->root), pageaddrs, nbpages, size, nodes, heap->protection);

  free(pageaddrs);
  free(nodes);

  marcel_spin_unlock(&(memory_manager->lock));
  mdebug_heap("Allocating %p on node #%d\n", buffer, node);
  LOG_OUT();
  return buffer;
}

void ma_memory_free_from_node(marcel_memory_manager_t *memory_manager, void *buffer, int nbpages, int node) {
  marcel_memory_area_t *available;

  LOG_IN();

  mdebug_heap("Freeing space from %p with %d pages\n", buffer, nbpages);
  available = malloc(sizeof(marcel_memory_area_t));
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

  if (memory_manager->membind_policy == MARCEL_MEMORY_MEMBIND_POLICY_NONE) {
    numanode = marcel_current_node();
    if (tbx_unlikely(numanode == -1)) numanode=0;
  }
  else if (memory_manager->membind_policy == MARCEL_MEMORY_MEMBIND_POLICY_SPECIFIC_NODE) {
    numanode = memory_manager->membind_node;
  }
  else if (memory_manager->membind_policy == MARCEL_MEMORY_MEMBIND_POLICY_LEAST_LOADED_NODE) {
    marcel_memory_select_node(memory_manager, MARCEL_MEMORY_LEAST_LOADED_NODE, &numanode);
  }

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

void marcel_memory_membind(marcel_memory_manager_t *memory_manager,
                           marcel_memory_membind_policy_t policy,
                           int node) {
  LOG_IN();
  mdebug_heap("Set the current membind policy to %d (node %d)\n", policy, node);
  memory_manager->membind_policy = policy;
  memory_manager->membind_node = node;
  LOG_OUT();
}

void marcel_memory_free(marcel_memory_manager_t *memory_manager, void *buffer) {
  LOG_IN();
  mdebug_heap("Freeing [%p]\n", buffer);
  marcel_spin_lock(&(memory_manager->lock));
  ma_memory_delete(memory_manager, &(memory_manager->root), buffer);
  marcel_spin_unlock(&(memory_manager->lock));
  LOG_OUT();
}

static
int ma_memory_locate(marcel_memory_manager_t *memory_manager, marcel_memory_tree_t *memory_tree, void *address, int *node, marcel_memory_data_t **data) {
 LOG_IN();
  if (memory_tree==NULL) {
    // We did not find the address
    *node = -1;
    errno = ENOENT;
    return -errno;
  }
  else if (address >= memory_tree->data->startaddress && address < memory_tree->data->endaddress) {
    // the address is stored on the current memory_data
    int offset = address - memory_tree->data->startaddress;
    mdebug_heap("Found address %p in [%p:%p]\n", address, memory_tree->data->startaddress, memory_tree->data->endaddress);
    *node = memory_tree->data->nodes[offset / memory_manager->pagesize];
    mdebug_heap("Address %p is located on node %d\n", address, *node);
    if (data) *data = memory_tree->data;
    return 0;
  }
  else if (address <= memory_tree->data->startaddress) {
    return ma_memory_locate(memory_manager, memory_tree->leftchild, address, node, data);
  }
  else if (address >= memory_tree->data->endaddress) {
    return ma_memory_locate(memory_manager, memory_tree->rightchild, address, node, data);
  }
  else {
    *node = -1;
    errno = ENOENT;
    return -errno;
  }
  LOG_OUT();
}

int marcel_memory_locate(marcel_memory_manager_t *memory_manager, void *address, int *node) {
  marcel_memory_data_t *data = NULL;
  return ma_memory_locate(memory_manager, memory_manager->root, address, node, &data);
}

static
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

void marcel_memory_migration_cost(marcel_memory_manager_t *memory_manager,
                                  int source,
                                  int dest,
                                  size_t size,
                                  float *cost) {
  p_tbx_slist_t migration_costs;

  LOG_IN();
  *cost = -1;
  migration_costs = memory_manager->migration_costs[source][dest];
  if (!tbx_slist_is_nil(migration_costs)) {
    tbx_slist_ref_to_head(migration_costs);
    do {
      marcel_memory_migration_cost_t *object = NULL;
      object = tbx_slist_ref_get(migration_costs);

      if (size >= object->size_min && size <= object->size_max) {
	*cost = (object->slope * size) + object->intercept;
	break;
      }
    } while (tbx_slist_ref_forward(migration_costs));
  }
  LOG_OUT();
}

void marcel_memory_writing_access_cost(marcel_memory_manager_t *memory_manager,
                                       int source,
                                       int dest,
                                       size_t size,
                                       float *cost) {
  LOG_IN();
  marcel_access_cost_t access_cost = memory_manager->writing_access_costs[source][dest];
  *cost = (size/memory_manager->cache_line_size) * access_cost.cost;
  LOG_OUT();
}

void marcel_memory_reading_access_cost(marcel_memory_manager_t *memory_manager,
                                       int source,
                                       int dest,
                                       size_t size,
                                       float *cost) {
  LOG_IN();
  marcel_access_cost_t access_cost = memory_manager->reading_access_costs[source][dest];
  *cost = ((float)size/(float)memory_manager->cache_line_size) * access_cost.cost;
  LOG_OUT();
}

static
void ma_memory_get_free_space(marcel_memory_manager_t *memory_manager,
                              int node,
                              int *space) {
  marcel_memory_area_t *heap = memory_manager->heaps[node];
  LOG_IN();
  *space = 0;
  while (heap != NULL) {
    *space += heap->nbpages;
    heap = heap->next;
  }
  LOG_OUT();
}

void marcel_memory_select_node(marcel_memory_manager_t *memory_manager,
                               marcel_memory_node_selection_policy_t policy,
                               int *node) {
  LOG_IN();
  marcel_spin_lock(&(memory_manager->lock));

  if (policy == MARCEL_MEMORY_LEAST_LOADED_NODE) {
    int i, space, maxspace;
    maxspace = 0;
    for(i=0 ; i<marcel_nbnodes ; i++) {
      ma_memory_get_free_space(memory_manager, i, &space);
      if (space > maxspace) {
        maxspace = space;
        *node = i;
      }
    }
  }

  marcel_spin_unlock(&(memory_manager->lock));
  LOG_OUT();
}

static
int ma_memory_migrate_pages(marcel_memory_manager_t *memory_manager, void *buffer, marcel_memory_data_t *data, int source, int dest) {
  int i, *dests, *status;

  LOG_IN();
  if (source == -1) {
    mdebug_heap("The address %p is not managed by MAMI.\n", buffer);
    errno = ENOENT;
    return -errno;
  }
  else if (source == dest) {
    mdebug_heap("The address %p is already located at the required node.\n", buffer);
    errno = EALREADY;
    return -errno;
  }

  mdebug_heap("Migrating %d page(s) to node #%d\n", data->nbpages, dest);
  dests = (int *) malloc(data->nbpages * sizeof(int));
  status = (int *) malloc(data->nbpages * sizeof(int));
  for(i=0 ; i<data->nbpages ; i++) dests[i] = dest;
  ma_memory_sampling_migrate_pages(data->pageaddrs, data->nbpages, dests, status);
#ifdef PM2DEBUG
  ma_memory_sampling_check_pages_location(data->pageaddrs, data->nbpages, dest);
#endif /* PM2DEBUG */
  for(i=0 ; i<data->nbpages ; i++) data->nodes[i] = dest;

  LOG_OUT();
  return 0;
}

int marcel_memory_migrate_pages(marcel_memory_manager_t *memory_manager,
                                void *buffer, int dest) {
  int source, ret;
  marcel_memory_data_t *data;

  LOG_IN();
  marcel_spin_lock(&(memory_manager->lock));
  ma_memory_locate(memory_manager, memory_manager->root, buffer, &source, &data);
  ret = ma_memory_migrate_pages(memory_manager, buffer, data, source, dest);
  marcel_spin_unlock(&(memory_manager->lock));
  LOG_OUT();
  return ret;
}

marcel_memory_manager_t *g_memory_manager = NULL;

static
void ma_memory_segv_handler(int sig, siginfo_t *info, void *_context) {
  ucontext_t *context = _context;
  void *addr;
  int err, source, dest;
  marcel_memory_data_t *data = NULL;

  marcel_spin_lock(&(g_memory_manager->lock));

#ifdef __x86_64__
  addr = (void *)(context->uc_mcontext.gregs[REG_CR2]);
#elif __i386__
  addr = (void *)(context->uc_mcontext.cr2);
#else
#error Unsupported architecture
#endif

  ma_memory_locate(g_memory_manager, g_memory_manager->root, addr, &source, &data);
  if (source == -1) {
    // The address is not managed by MAMI. Reset the segv handler to its default action, to cause a segfault
    struct sigaction act;
    act.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &act, NULL);
  }
  if (data->status != MARCEL_MEMORY_NEXT_TOUCHED_STATUS) {
    data->status = MARCEL_MEMORY_NEXT_TOUCHED_STATUS;
    dest = marcel_current_node();
    ma_memory_migrate_pages(g_memory_manager, addr, data, source, dest);
    err = mprotect(data->startaddress, data->size, data->protection);
    if (err < 0) {
      char *msg = "mprotect(handler): ";
      write(2, msg, strlen(msg));
      switch (errno) {
      case -EACCES: write(2, "The memory cannot be given the specified access.\n", 50);
      case -EFAULT: write(2, "The memory cannot be accessed.\n", 32);
      case -EINVAL: write(2, "Invalid pointer.\n", 18);
      case -ENOMEM: write(2, "Out of memory\n", 15);
      default: write(2, "Error\n", 6);
      }
    }
  }
  marcel_spin_unlock(&(g_memory_manager->lock));
}

int marcel_memory_migrate_on_next_touch(marcel_memory_manager_t *memory_manager, void *buffer) {
  int err, source;
  static int handler_set = 0;
  marcel_memory_data_t *data = NULL;

  marcel_spin_lock(&(memory_manager->lock));
  LOG_IN();

  if (!handler_set) {
    handler_set = 1;
    struct sigaction act;
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = ma_memory_segv_handler;
    err = sigaction(SIGSEGV, &act, NULL);
    if (err < 0) {
      perror("sigaction");
    }
  }

  g_memory_manager = memory_manager;
  ma_memory_locate(memory_manager, memory_manager->root, buffer, &source, &data);
  if (source == -1) {
    mdebug_heap("The address %p is not managed by MAMI.\n", buffer);
    errno = ENOENT;
    marcel_spin_unlock(&(memory_manager->lock));
    LOG_OUT();
    return -errno;
  }

  data->status = MARCEL_MEMORY_INITIAL_STATUS;
  err = mprotect(data->startaddress, data->size, PROT_NONE);
  if (err < 0) {
    perror("mprotect");
  }
  marcel_spin_unlock(&(memory_manager->lock));
  LOG_OUT();
  return 0;
}

#endif /* MARCEL_MAMI_ENABLED */
