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

#ifdef MM_MAMI_ENABLED

#include <errno.h>
#include <fcntl.h>
#ifdef LINUX_SYS
#  include <malloc.h>
#endif /* LINUX_SYS */
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pm2_valgrind.h"
#include "mm_mami.h"
#include "mm_mami_private.h"
#include "mm_debug.h"
#include "mm_helper.h"
#include "mm_mami_thread.h"

#ifdef MARCEL
#  include "mm_mami_marcel_private.h"
#endif /* MARCEL */

#ifndef MPOL_MF_LAZY
#  define MPOL_MF_LAZY (1<<3)
#endif

static
mami_manager_t *_mami_memory_manager = NULL;
static
int _mami_sigsegv_handler_set = 0;

void mami_init(mami_manager_t **memory_manager_p) {
  int node, dest;
  void *ptr;
  int err;
  mami_manager_t *memory_manager;

  MEMORY_LOG_IN();
  memory_manager = th_mami_malloc(sizeof(mami_manager_t));
  memory_manager->root = NULL;
  th_mami_mutex_init(&(memory_manager->lock), NULL);
  memory_manager->normal_page_size = getpagesize();
  memory_manager->initially_preallocated_pages = 1024;
  memory_manager->cache_line_size = 64;
  memory_manager->membind_policy = MAMI_MEMBIND_POLICY_NONE;
  memory_manager->alignment = 1;
  memory_manager->nb_nodes = marcel_nbnodes;
  mdebug_memory("Number of NUMA nodes = %d\n", memory_manager->nb_nodes);

  memory_manager->max_node = marcel_topo_node_level ? marcel_topo_node_level[memory_manager->nb_nodes-1].os_node+2 : 2;
  mdebug_memory("Max node = %d\n", memory_manager->max_node);

  memory_manager->os_nodes = th_mami_malloc(memory_manager->nb_nodes * sizeof(int));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    memory_manager->os_nodes[node] = marcel_topo_node_level ? marcel_topo_node_level[node].os_node : node;
    mdebug_memory("Node #%d --> OS Node #%d\n", node, memory_manager->os_nodes[node]);
  }

  memory_manager->huge_page_size = marcel_topo_node_level ? marcel_topo_node_level[0].huge_page_size : 0;
  mdebug_memory("Huge page size : %ld\n", memory_manager->huge_page_size);
  memory_manager->huge_page_free = th_mami_malloc(memory_manager->nb_nodes * sizeof(int));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    memory_manager->huge_page_free[node] = marcel_topo_node_level ? marcel_topo_node_level[node].huge_page_free : 0;
    mdebug_memory("Number of huge pages on node #%d = %d\n", node, memory_manager->huge_page_free[node]);
  }

  // Is in-kernel migration available
#ifdef LINUX_SYS
  memory_manager->kernel_nexttouch_migration_available = 0;
  ptr = memalign(memory_manager->normal_page_size, memory_manager->normal_page_size);
  err = _mm_mbind(ptr, memory_manager->normal_page_size, MPOL_PREFERRED, NULL, 0, MPOL_MF_MOVE|MPOL_MF_LAZY);
  if (err >= 0) {
    // TODO: That test is not enough, we also need to test that the
    // file /cpusets/migrate_on_fault contains 1, or as it depends on
    // the current cpuset, it would be better to migrate pages
    memory_manager->kernel_nexttouch_migration_available = MAMI_KERNEL_NEXT_TOUCH_MOF;
  }
  else {
    err = madvise(ptr, memory_manager->normal_page_size, 12);
    if (err >= 0)
      memory_manager->kernel_nexttouch_migration_available = MAMI_KERNEL_NEXT_TOUCH_BRICE;
  }
  free(ptr);
#else /* !LINUX_SYS */
  memory_manager->kernel_nexttouch_migration_available = 0;
#endif /* LINUX_SYS */
  memory_manager->kernel_nexttouch_migration_requested = memory_manager->kernel_nexttouch_migration_available;
  mdebug_memory("Kernel next_touch migration: %d\n", memory_manager->kernel_nexttouch_migration_available);

  // Is migration available
  if (marcel_topo_node_level) {
    unsigned long nodemask;
    nodemask = (1<<memory_manager->os_nodes[0]);
    ptr = memalign(memory_manager->normal_page_size, memory_manager->normal_page_size);
    err = _mm_mbind(ptr,  memory_manager->normal_page_size, MPOL_BIND, &nodemask, memory_manager->max_node, MPOL_MF_MOVE);
    memory_manager->migration_flag = err>=0 ? MPOL_MF_MOVE : 0;
    free(ptr);
  }
  else {
    memory_manager->migration_flag = 0;
  }
  mdebug_memory("Migration: %d\n", memory_manager->migration_flag);

  // How much total and free memory per node
  memory_manager->mem_total = th_mami_malloc(memory_manager->nb_nodes * sizeof(unsigned long));
  memory_manager->mem_free = th_mami_malloc(memory_manager->nb_nodes * sizeof(unsigned long));
  if (marcel_topo_node_level) {
    for(node=0 ; node<memory_manager->nb_nodes ; node++) {
      memory_manager->mem_total[node] = marcel_topo_node_level[node].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_NODE];
      memory_manager->mem_free[node] = marcel_topo_node_level[node].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_NODE];
    }
  }
  else {
    memory_manager->mem_total[0] = marcel_topo_levels[0][0].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_MACHINE];
    memory_manager->mem_free[0] = marcel_topo_levels[0][0].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_MACHINE];
  }
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    mdebug_memory("Memory on node #%d = %ld\n", node, memory_manager->mem_total[node]);
  }

  // Preallocate memory on each node
  memory_manager->heaps = th_mami_malloc((memory_manager->nb_nodes+1) * sizeof(mami_area_t *));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    _mami_preallocate(memory_manager, &(memory_manager->heaps[node]), memory_manager->initially_preallocated_pages, node, memory_manager->os_nodes[node]);
    mdebug_memory("Preallocating %p for node #%d %d\n", memory_manager->heaps[node]->start, node, memory_manager->os_nodes[node]);
  }
  _mami_preallocate(memory_manager, &(memory_manager->heaps[MAMI_FIRST_TOUCH_NODE]), memory_manager->initially_preallocated_pages, MAMI_FIRST_TOUCH_NODE, MAMI_FIRST_TOUCH_NODE);
  mdebug_memory("Preallocating %p for anonymous heap\n", memory_manager->heaps[MAMI_FIRST_TOUCH_NODE]->start);

  // Initialise space with huge pages on each node
  memory_manager->huge_pages_heaps = th_mami_malloc(memory_manager->nb_nodes * sizeof(mami_huge_pages_area_t *));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    memory_manager->huge_pages_heaps[node] = NULL;
  }

  // Load the model for the migration costs
  memory_manager->migration_costs = th_mami_malloc(memory_manager->nb_nodes * sizeof(p_tbx_slist_t *));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    memory_manager->migration_costs[node] = th_mami_malloc(memory_manager->nb_nodes * sizeof(p_tbx_slist_t));
    for(dest=0 ; dest<memory_manager->nb_nodes ; dest++) {
      memory_manager->migration_costs[node][dest] = tbx_slist_nil();
    }
  }
  _mami_load_model_for_memory_migration(memory_manager);

  // Load the model for the access costs
  memory_manager->costs_for_write_access = th_mami_malloc(memory_manager->nb_nodes * sizeof(mami_access_cost_t *));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    memory_manager->costs_for_write_access[node] = th_mami_malloc(memory_manager->nb_nodes * sizeof(mami_access_cost_t));
    for(dest=0 ; dest<memory_manager->nb_nodes ; dest++) {
      memory_manager->costs_for_write_access[node][dest].cost = 0;
    }
  }
  memory_manager->costs_for_read_access = th_mami_malloc(memory_manager->nb_nodes * sizeof(mami_access_cost_t *));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    memory_manager->costs_for_read_access[node] = th_mami_malloc(memory_manager->nb_nodes * sizeof(mami_access_cost_t));
    for(dest=0 ; dest<memory_manager->nb_nodes ; dest++) {
      memory_manager->costs_for_read_access[node][dest].cost = 0;
    }
  }
  _mami_load_model_for_memory_access(memory_manager);

#ifdef PM2DEBUG
  if (debug_memory.show > PM2DEBUG_STDLEVEL) {
    for(node=0 ; node<memory_manager->nb_nodes ; node++) {
      for(dest=0 ; dest<memory_manager->nb_nodes ; dest++) {
        p_tbx_slist_t migration_costs = memory_manager->migration_costs[node][dest];
        if (!tbx_slist_is_nil(migration_costs)) {
          tbx_slist_ref_to_head(migration_costs);
          do {
            mami_migration_cost_t *object = NULL;
            object = tbx_slist_ref_get(migration_costs);

            th_mami_fprintf(stderr, "[%d:%d] [%ld:%ld] %f %f %f\n", node, dest, (long)object->size_min, (long)object->size_max, object->slope, object->intercept, object->correlation);
          } while (tbx_slist_ref_forward(migration_costs));
        }
      }
    }

    for(node=0 ; node<memory_manager->nb_nodes ; node++) {
      for(dest=0 ; dest<memory_manager->nb_nodes ; dest++) {
        mami_access_cost_t wcost = memory_manager->costs_for_write_access[node][dest];
        mami_access_cost_t rcost = memory_manager->costs_for_write_access[node][dest];
        th_mami_fprintf(stderr, "[%d:%d] %f %f\n", node, dest, wcost.cost, rcost.cost);
      }
    }
  }
#endif /* PM2DEBUG */

  *memory_manager_p = memory_manager;
  MEMORY_LOG_OUT();
}

static
void _mami_sampling_free(p_tbx_slist_t migration_costs) {
  MEMORY_ILOG_IN();
  while (!tbx_slist_is_nil(migration_costs)) {
    mami_migration_cost_t *ptr = tbx_slist_extract(migration_costs);
    th_mami_free(ptr);
  }
  tbx_slist_free(migration_costs);
  MEMORY_ILOG_OUT();
}

static
int _mami_deallocate_huge_pages(mami_manager_t *memory_manager, mami_huge_pages_area_t **space, int node) {
  int err=0;

  MEMORY_ILOG_IN();
  mdebug_memory("Releasing huge pages %s\n", (*space)->filename);

  err = close((*space)->file);
  if (err < 0) perror("(_mami_deallocate_huge_pages) close");
  else {
    err = unlink((*space)->filename);
    if (err < 0) perror("(_mami_deallocate_huge_pages) unlink");
  }

  th_mami_free((*space)->filename);
  MEMORY_ILOG_OUT();
  return err;
}

static
void _mami_deallocate(mami_manager_t *memory_manager, mami_area_t **space, int node) {
  mami_area_t *ptr, *ptr2;

  MEMORY_ILOG_IN();
  mdebug_memory("Deallocating memory for node #%d\n", node);
  ptr = (*space);
  while (ptr != NULL) {
    mdebug_memory("Unmapping memory area from %p\n", ptr->start);
    munmap(ptr->start, ptr->nb_pages * ptr->page_size);
    ptr2 = ptr;
    ptr = ptr->next;
    th_mami_free(ptr2);
  }
  MEMORY_ILOG_OUT();
}

static
void _mami_clean_memory(mami_manager_t *memory_manager) {
  while (memory_manager->root) {
    mami_free(memory_manager, memory_manager->root->data->start_address);
  }
}

void mami_exit(mami_manager_t **memory_manager_p) {
  int node, dest;
  struct sigaction act;
  mami_manager_t *memory_manager;

  MEMORY_LOG_IN();
  memory_manager = *memory_manager_p;
  if (_mami_sigsegv_handler_set) {
    act.sa_flags = SA_SIGINFO;
    act.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &act, NULL);
  }

  if (memory_manager->root) {
    th_mami_fprintf(stderr, "MaMI Warning: some memory areas have not been free-d\n");
#ifdef PM2DEBUG
    if (debug_memory.show > PM2DEBUG_STDLEVEL) {
      mami_fprint(memory_manager, stderr);
    }
#endif /* PM2DEBUG */
    _mami_clean_memory(memory_manager);
  }

  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    _mami_deallocate(memory_manager, &(memory_manager->heaps[node]), node);
  }
  _mami_deallocate(memory_manager, &(memory_manager->heaps[MAMI_FIRST_TOUCH_NODE]), MAMI_FIRST_TOUCH_NODE);
  th_mami_free(memory_manager->heaps);

  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    if (memory_manager->huge_pages_heaps[node]) {
      _mami_deallocate_huge_pages(memory_manager, &(memory_manager->huge_pages_heaps[node]), node);
    }
  }
  th_mami_free(memory_manager->huge_pages_heaps);

  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    for(dest=0 ; dest<memory_manager->nb_nodes ; dest++) {
      _mami_sampling_free(memory_manager->migration_costs[node][dest]);
    }
    th_mami_free(memory_manager->migration_costs[node]);
  }
  th_mami_free(memory_manager->migration_costs);

  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    th_mami_free(memory_manager->costs_for_read_access[node]);
    th_mami_free(memory_manager->costs_for_write_access[node]);
  }
  th_mami_free(memory_manager->costs_for_read_access);
  th_mami_free(memory_manager->costs_for_write_access);
  th_mami_free(memory_manager->mem_total);
  th_mami_free(memory_manager->mem_free);
  th_mami_free(memory_manager->os_nodes);
  th_mami_free(memory_manager->huge_page_free);
  th_mami_free(memory_manager);

  MEMORY_LOG_OUT();
}

int mami_set_alignment(mami_manager_t *memory_manager) {
  memory_manager->alignment = 1;
  return 0;
}

int mami_unset_alignment(mami_manager_t *memory_manager) {
  memory_manager->alignment = 0;
  return 0;
}

static
void _mami_init_memory_data(mami_manager_t *memory_manager,
                            void **pageaddrs, int nb_pages, size_t size, int node, int *nodes,
                            int protection, int with_huge_pages, int mami_allocated,
                            mami_data_t **memory_data) {
  MEMORY_ILOG_IN();

  *memory_data = th_mami_malloc(sizeof(mami_data_t));

  // Set the interval addresses and the length
  (*memory_data)->start_address = pageaddrs[0];
  (*memory_data)->mprotect_start_address = pageaddrs[0];
  (*memory_data)->end_address = pageaddrs[0]+size;
  (*memory_data)->size = size;
  (*memory_data)->mprotect_size = size;
  (*memory_data)->status = MAMI_INITIAL_STATUS;
  (*memory_data)->protection = protection;
  (*memory_data)->with_huge_pages = with_huge_pages;
  (*memory_data)->mami_allocated = mami_allocated;
  (*memory_data)->owners = tbx_slist_nil();
  (*memory_data)->next = NULL;
  (*memory_data)->node = node;
  if (nodes == NULL) (*memory_data)->nodes = NULL;
  else {
    (*memory_data)->nodes = th_mami_malloc(nb_pages * sizeof(int));
    memcpy((*memory_data)->nodes, nodes, nb_pages*sizeof(int));
  }

  if ((*memory_data)->node != MAMI_MULTIPLE_LOCATION_NODE) {
    (*memory_data)->pages_per_node = NULL;
    (*memory_data)->pages_on_undefined_node = -1;
  }
  else {
    int i;
    (*memory_data)->pages_per_node = th_mami_malloc(memory_manager->nb_nodes * sizeof(int));
    memset((*memory_data)->pages_per_node, 0, memory_manager->nb_nodes*sizeof(int));
    (*memory_data)->pages_on_undefined_node = 0;
    for(i=0 ; i<nb_pages ; i++) {
      if (nodes[i] >= 0) (*memory_data)->pages_per_node[nodes[i]] ++; else (*memory_data)->pages_on_undefined_node ++;
    }
  }

  // Set the page addresses
  (*memory_data)->nb_pages = nb_pages;
  (*memory_data)->pageaddrs = th_mami_malloc((*memory_data)->nb_pages * sizeof(void *));
  memcpy((*memory_data)->pageaddrs, pageaddrs, nb_pages*sizeof(void*));

  MEMORY_ILOG_OUT();
}

static
void _mami_clean_memory_data(mami_data_t **memory_data) {
  mdebug_memory("Cleaning memory area %p\n", (*memory_data)->start_address);
  if (!(tbx_slist_is_nil((*memory_data)->owners))) {
    th_mami_fprintf(stderr, "MaMI Warning: some threads are still attached to the memory area [%p:%p]\n",
                    (*memory_data)->start_address, (*memory_data)->end_address);
    tbx_slist_clear((*memory_data)->owners);
  }
  tbx_slist_free((*memory_data)->owners);
  if ((*memory_data)->nodes != NULL) th_mami_free((*memory_data)->nodes);
  th_mami_free((*memory_data)->pageaddrs);
  th_mami_free(*memory_data);
}

void _mami_delete_tree(mami_manager_t *memory_manager, mami_tree_t **memory_tree) {
  MEMORY_ILOG_IN();
  if ((*memory_tree)->left_child == NULL) {
    mami_tree_t *temp = (*memory_tree);
    _mami_clean_memory_data(&(temp->data));
    (*memory_tree) = (*memory_tree)->right_child;
    th_mami_free(temp);
  }
  else if ((*memory_tree)->right_child == NULL) {
    mami_tree_t *temp = *memory_tree;
    _mami_clean_memory_data(&(temp->data));
    (*memory_tree) = (*memory_tree)->left_child;
    th_mami_free(temp);
  }
  else {
    // In-order predecessor (rightmost child of left subtree)
    // Node has two children - get max of left subtree
    mami_tree_t *temp = (*memory_tree)->left_child; // get left node of the original node

    // find the rightmost child of the subtree of the left node
    while (temp->right_child != NULL) {
      temp = temp->right_child;
    }

    // copy the value from the in-order predecessor to the original node
    _mami_clean_memory_data(&((*memory_tree)->data));
    _mami_init_memory_data(memory_manager, temp->data->pageaddrs, temp->data->nb_pages, temp->data->size, temp->data->node,
                           temp->data->nodes, temp->data->protection, temp->data->with_huge_pages, temp->data->mami_allocated,
                           &((*memory_tree)->data));

    // then delete the predecessor
    _mami_unregister(memory_manager, &((*memory_tree)->left_child), temp->data->pageaddrs[0]);
  }
  MEMORY_ILOG_OUT();
}

static
void _mami_free_from_node(mami_manager_t *memory_manager, void *buffer, size_t size, int nb_pages, int node, int protection, int with_huge_pages) {
  mami_area_t *available;
  mami_area_t **ptr;
  mami_area_t **prev;
  mami_area_t *next;

  MEMORY_ILOG_IN();

  mdebug_memory("Freeing space [%p:%p] with %d pages\n", buffer, buffer+size, nb_pages);

  available = th_mami_malloc(sizeof(mami_area_t));
  available->start = buffer;
  available->end = buffer + size;
  available->nb_pages = nb_pages;
  available->page_size = (with_huge_pages) ? memory_manager->huge_page_size : memory_manager->normal_page_size;
  available->protection = protection;
  available->next = NULL;

  // Insert the new area at the right place
  if (with_huge_pages) {
    ptr = &(memory_manager->huge_pages_heaps[node]->heap);
  }
  else {
    ptr = &(memory_manager->heaps[node]);
    if (node >= 0 && node < memory_manager->nb_nodes) {
      memory_manager->mem_free[node] += (size/1024);
    }
  }

  if (*ptr == NULL) *ptr = available;
  else {
    prev = ptr;
    while (*ptr != NULL && ((*ptr)->start < available->start)) {
      prev = ptr;
      ptr = &((*ptr)->next);
    }
    if (prev == ptr) {
      available->next = *ptr;
      *ptr = available;
    }
    else {
      available->next = (*prev)->next;
      (*prev)->next = available;
    }
  }

  // Defragment the areas
  ptr = (with_huge_pages) ? &(memory_manager->huge_pages_heaps[node]->heap) : &(memory_manager->heaps[node]);
  while ((*ptr)->next != NULL) {
    if (((*ptr)->end == (*ptr)->next->start) && ((*ptr)->protection == (*ptr)->next->protection)) {
      mdebug_memory("[%p:%p:%d] and [%p:%p:%d] can be defragmented\n", (*ptr)->start, (*ptr)->end, (*ptr)->protection,
                  (*ptr)->next->start, (*ptr)->next->end, (*ptr)->next->protection);
      (*ptr)->end = (*ptr)->next->end;
      (*ptr)->nb_pages += (*ptr)->next->nb_pages;
      next = (*ptr)->next;
      (*ptr)->next = (*ptr)->next->next;
      th_mami_free(next);
    }
    else
      ptr = &((*ptr)->next);
  }

  MEMORY_ILOG_OUT();
}

void _mami_unregister(mami_manager_t *memory_manager, mami_tree_t **memory_tree, void *buffer) {
  MEMORY_ILOG_IN();
  if (*memory_tree!=NULL) {
    if (buffer == (*memory_tree)->data->pageaddrs[0]) {
      mami_data_t *data = (*memory_tree)->data;
      mami_data_t *next_data = data->next;

      if (data->mami_allocated) {
	mdebug_memory("Removing [%p, %p] from node #%d\n", (*memory_tree)->data->pageaddrs[0], (*memory_tree)->data->pageaddrs[0]+(*memory_tree)->data->size,
                      data->node);
        VALGRIND_MAKE_MEM_NOACCESS((*memory_tree)->data->pageaddrs[0], (*memory_tree)->data->size);
	// Free memory
        if (data->node >= 0 && data->node <= memory_manager->nb_nodes)
          _mami_free_from_node(memory_manager, buffer, data->size, data->nb_pages, data->node, data->protection, data->with_huge_pages);
      }
      else {
	mdebug_memory("Address %p is not allocated by MaMI.\n", buffer);
      }

      // Delete tree
      _mami_delete_tree(memory_manager, memory_tree);

      if (next_data) {
        mdebug_memory("Need to unregister the next memory area\n");
        _mami_unregister(memory_manager, &(memory_manager->root), next_data->start_address);
      }
    }
    else if (buffer < (*memory_tree)->data->pageaddrs[0])
      _mami_unregister(memory_manager, &((*memory_tree)->left_child), buffer);
    else
      _mami_unregister(memory_manager, &((*memory_tree)->right_child), buffer);
  }
  MEMORY_ILOG_OUT();
}

static
void _mami_register_pages(mami_manager_t *memory_manager, mami_tree_t **memory_tree,
                          void **pageaddrs, int nb_pages, size_t size, int node, int *nodes,
                          int protection, int with_huge_pages, int mami_allocated,
                          mami_data_t **data) {
  MEMORY_ILOG_IN();
  if (*memory_tree==NULL) {
    *memory_tree = th_mami_malloc(sizeof(mami_tree_t));
    (*memory_tree)->left_child = NULL;
    (*memory_tree)->right_child = NULL;
    _mami_init_memory_data(memory_manager, pageaddrs, nb_pages, size, node, nodes,
                           protection, with_huge_pages, mami_allocated, &((*memory_tree)->data));
    mdebug_memory("Adding data %p into tree\n", (*memory_tree)->data);
    if (data) *data = (*memory_tree)->data;
  }
  else {
    if (pageaddrs[0] < (*memory_tree)->data->pageaddrs[0])
      _mami_register_pages(memory_manager, &((*memory_tree)->left_child), pageaddrs, nb_pages, size, node, nodes,
                           protection, with_huge_pages, mami_allocated, data);
    else
      _mami_register_pages(memory_manager, &((*memory_tree)->right_child), pageaddrs, nb_pages, size, node, nodes,
                           protection, with_huge_pages, mami_allocated, data);
  }
  MEMORY_ILOG_OUT();
}

int _mami_preallocate_huge_pages(mami_manager_t *memory_manager, mami_huge_pages_area_t **space, int node) {
  pid_t pid;

  MEMORY_ILOG_IN();
  if (memory_manager->huge_page_free[node] == 0) {
    mdebug_memory("No huge pages on node #%d\n", node);
    *space = NULL;
    return -1;
  }

  (*space) = th_mami_malloc(sizeof(mami_huge_pages_area_t));
  (*space)->size = memory_manager->huge_page_free[node] * memory_manager->huge_page_size;
  (*space)->filename = th_mami_malloc(1024 * sizeof(char));
  pid = getpid();
  th_mami_sprintf((*space)->filename, "/hugetlbfs/mami_pid_%d_node_%d", pid, node);

  (*space)->file = open((*space)->filename, O_CREAT|O_RDWR, S_IRWXU);
  if ((*space)->file == -1) {
    th_mami_free(*space);
    *space = NULL;
    perror("(_mami_preallocate_huge_pages) open");
    return -1;
  }

  (*space)->buffer = mmap(NULL, (*space)->size, PROT_READ|PROT_WRITE, MAP_PRIVATE, (*space)->file, 0);
  if ((*space)->buffer == MAP_FAILED) {
    perror("(_mami_preallocate_huge_pages) mmap");
    th_mami_free(*space);
    *space = NULL;
    return -1;
  }
#warning todo: readv pour prefaulter
  /* mark the memory as unaccessible until it gets allocated to the application */
  VALGRIND_MAKE_MEM_NOACCESS((*space)->buffer, (*space)->size);

  (*space)->heap = th_mami_malloc(sizeof(mami_area_t));
  (*space)->heap->start = (*space)->buffer;
  (*space)->heap->end = (*space)->buffer + (*space)->size;
  (*space)->heap->nb_pages = memory_manager->huge_page_free[node];
  (*space)->heap->protection = PROT_READ|PROT_WRITE;
  (*space)->heap->page_size = memory_manager->huge_page_size;
  (*space)->heap->next = NULL;

  MEMORY_ILOG_OUT();
  return 0;
}

int _mami_preallocate(mami_manager_t *memory_manager, mami_area_t **space, int nb_pages, int vnode, int pnode) {
  unsigned long nodemask;
  size_t length;
  void *buffer;
  int i, err=0;

  MEMORY_ILOG_IN();

  if (vnode != MAMI_FIRST_TOUCH_NODE && memory_manager->mem_total[vnode] == 0) {
    mdebug_memory("No memory available on node #%d, os_node #%d\n", vnode, pnode);
    errno = EINVAL;
    err = -1;
    buffer = NULL;
    length = 0;
  }
  else {
    length = nb_pages * memory_manager->normal_page_size;
    buffer = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED) {
      perror("(_mami_preallocate) mmap");
      err = -1;
    }
    else {
      if (vnode != MAMI_FIRST_TOUCH_NODE) {
        nodemask = (1<<pnode);
        mdebug_memory("Mbinding on node %d with nodemask %ld and max node %d\n", pnode, nodemask, memory_manager->max_node);
        err = _mm_mbind(buffer, length, MPOL_BIND, &nodemask, memory_manager->max_node, MPOL_MF_STRICT|memory_manager->migration_flag);
        if (err < 0) {
          perror("(_mami_preallocate) mbind");
          err = 0;
        }

        for(i=0 ; i<nb_pages ; i++) {
          int *ptr = buffer+i*memory_manager->normal_page_size;
          *ptr = 0;
        }
      }
      /* mark the memory as unaccessible until it gets allocated to the application */
      VALGRIND_MAKE_MEM_NOACCESS(buffer, length);
    }
  }

  if (!buffer) mdebug_memory("No space available on node #%d, os_node #%d\n", vnode, pnode);
  else {
    (*space) = th_mami_malloc(sizeof(mami_area_t));
    (*space)->start = buffer;
    (*space)->end = buffer + length;
    (*space)->nb_pages = nb_pages;
    (*space)->protection = PROT_READ|PROT_WRITE;
    (*space)->page_size = memory_manager->normal_page_size;
    (*space)->next = NULL;

    mdebug_memory("Preallocating [%p:%p] on node #%d, os_node #%d\n", buffer,buffer+length, vnode, pnode);
  }
  MEMORY_ILOG_OUT();
  return err;
}

static
void* _mami_get_buffer_from_huge_pages_heap(mami_manager_t *memory_manager, int node, int nb_pages, size_t size, int *protection) {
  mami_huge_pages_area_t *heap = memory_manager->huge_pages_heaps[node];
  mami_area_t *hheap = NULL, *prev = NULL;
  void *buffer = NULL;
  unsigned long nodemask;
  int err=0;

  if (memory_manager->huge_pages_heaps[node] == NULL) {
    err = _mami_preallocate_huge_pages(memory_manager, &(memory_manager->huge_pages_heaps[node]), node);
    if (err < 0) {
      return NULL;
    }
    mdebug_memory("Preallocating heap %p with huge pages for node #%d\n", memory_manager->huge_pages_heaps[node], node);
    heap = memory_manager->huge_pages_heaps[node];
  }

  if (heap == NULL) {
    mdebug_memory("No huge pages are available on node #%d\n", node);
    return NULL;
  }

  mdebug_memory("Requiring space from huge pages area of size=%lu on node #%d\n", (long unsigned)heap->size, node);

  // Look for a space big enough
  hheap = heap->heap;
  prev = hheap;
  while (hheap != NULL) {
    mdebug_memory("Current space from %p with %d pages\n", hheap->start, hheap->nb_pages);
    if (hheap->nb_pages >= nb_pages)
      break;
    prev = hheap;
    hheap = hheap->next;
  }
  if (hheap == NULL) {
    mdebug_memory("Not enough huge pages are available on node #%d\n", node);
    return NULL;
  }

  buffer = hheap->start;
  *protection = hheap->protection;
  if (nb_pages == hheap->nb_pages) {
    if (prev == hheap)
      heap->heap = prev->next;
    else
      prev->next = hheap->next;
  }
  else {
    hheap->start += size;
    hheap->nb_pages -= nb_pages;
  }

  nodemask = (1<<node);
  err = _mami_set_mempolicy(MPOL_BIND, &nodemask, memory_manager->max_node);
  if (err < 0) {
    perror("(_mami_get_buffer_from_huge_pages_heap) set_mempolicy");
    return NULL;
  }
  memset(buffer, 0, size);
  err = _mami_set_mempolicy(MPOL_DEFAULT, NULL, 0);
  if (err < 0) {
    perror("(_mami_get_buffer_from_huge_pages_heap) set_mempolicy");
    return NULL;
  }

  return buffer;
}

static
void* _mami_get_buffer_from_heap(mami_manager_t *memory_manager, int node, int nb_pages, size_t size, int *protection) {
  mami_area_t *heap = memory_manager->heaps[node];
  mami_area_t *prev = NULL;
  void *buffer;

  mdebug_memory("Requiring space of %d pages from heap %p on node #%d\n", nb_pages, heap, node);

  // Look for a space big enough
  prev = heap;
  while (heap != NULL && heap->start != NULL) {
    mdebug_memory("Current space from %p with %d pages\n", heap->start, heap->nb_pages);
    if (heap->nb_pages >= nb_pages)
      break;
    prev = heap;
    heap = heap->next;
  }
  if (heap == NULL || heap->start == NULL) {
    int err=0, preallocatedpages;

    preallocatedpages = nb_pages;
    if (preallocatedpages < memory_manager->initially_preallocated_pages) {
      preallocatedpages = memory_manager->initially_preallocated_pages;
    }
    mdebug_memory("not enough space, let's allocate %d extra pages on the OS node #%d\n", preallocatedpages, memory_manager->os_nodes[node]);
    err = _mami_preallocate(memory_manager, &heap, preallocatedpages, node, memory_manager->os_nodes[node]);
    if (err < 0) {
      return NULL;
    }
    if (prev == NULL) {
      memory_manager->heaps[node] = heap;
      prev = heap;
    }
    else {
      prev->next = heap;
    }
  }

  buffer = heap->start;
  *protection = heap->protection;
  if (nb_pages == heap->nb_pages) {
    if (prev == heap)
      memory_manager->heaps[node] = prev->next;
    else
      prev->next = heap->next;
  }
  else {
    heap->start += size;
    heap->nb_pages -= nb_pages;
  }
  if (node >= 0 && node < memory_manager->nb_nodes) {
    memory_manager->mem_free[node] -= (size/1024);
  }
  return buffer;
}

static
void* _mami_malloc(mami_manager_t *memory_manager, size_t size, unsigned long page_size, int node, int with_huge_pages) {
  void *buffer;
  int i, nb_pages, protection=0;
  size_t realsize;
  void **pageaddrs;

  MEMORY_ILOG_IN();

  if (!size) {
    MEMORY_ILOG_OUT();
    return NULL;
  }

  th_mami_mutex_lock(&(memory_manager->lock));

  // Round-up the size
  nb_pages = size / page_size;
  if (nb_pages * page_size != size) nb_pages++;
  realsize = nb_pages * page_size;

  if (!with_huge_pages) {
    buffer = _mami_get_buffer_from_heap(memory_manager, node, nb_pages, realsize, &protection);
  }
  else {
    buffer = _mami_get_buffer_from_huge_pages_heap(memory_manager, node, nb_pages, realsize, &protection);
  }

  if (!buffer) {
    errno = ENOMEM;
  }
  else {
    // Set the page addresses
    pageaddrs = th_mami_malloc(nb_pages * sizeof(void *));
    for(i=0; i<nb_pages ; i++) pageaddrs[i] = buffer + i*page_size;

    // Register memory
    mdebug_memory("Registering [%p:%p:%ld]\n", pageaddrs[0], pageaddrs[0]+size, (long)size);
    _mami_register_pages(memory_manager, &(memory_manager->root), pageaddrs, nb_pages, realsize, node, NULL,
                         protection, with_huge_pages, 1, NULL);

    th_mami_free(pageaddrs);
    VALGRIND_MAKE_MEM_UNDEFINED(buffer, size);
  }

  th_mami_mutex_unlock(&(memory_manager->lock));
  mdebug_memory("Allocating %p on node #%d\n", buffer, node);
  MEMORY_ILOG_OUT();
  return buffer;
}

void* mami_malloc(mami_manager_t *memory_manager, size_t size, mami_membind_policy_t policy, int node) {
  void *ptr;
  int with_huge_pages;
  unsigned long page_size;

  MEMORY_LOG_IN();

  page_size = memory_manager->normal_page_size;
  with_huge_pages = 0;
  if (policy == MAMI_MEMBIND_POLICY_DEFAULT) {
    policy = memory_manager->membind_policy;
    node = memory_manager->membind_node;
  }

  if (policy == MAMI_MEMBIND_POLICY_NONE) {
    node = th_mami_current_node();
    if (tbx_unlikely(node == -1)) node=0;
  }
  else if (policy == MAMI_MEMBIND_POLICY_LEAST_LOADED_NODE) {
    mami_select_node(memory_manager, MAMI_LEAST_LOADED_NODE, &node);
  }
  else if (policy == MAMI_MEMBIND_POLICY_HUGE_PAGES) {
    page_size = memory_manager->huge_page_size;
    with_huge_pages = 1;
  }
  else if (policy == MAMI_MEMBIND_POLICY_FIRST_TOUCH) {
    node = MAMI_FIRST_TOUCH_NODE;
  }

  if (tbx_unlikely(policy != MAMI_MEMBIND_POLICY_FIRST_TOUCH && node >= memory_manager->nb_nodes)) {
    mdebug_memory("Node %d not managed by MaMI\n", node);
    MEMORY_LOG_OUT();
    return NULL;
  }

  ptr = _mami_malloc(memory_manager, size, page_size, node, with_huge_pages);

  MEMORY_LOG_OUT();
  return ptr;
}

void* mami_calloc(mami_manager_t *memory_manager, size_t nmemb, size_t size,
                  mami_membind_policy_t policy, int node) {
  void *ptr;

  MEMORY_LOG_IN();

  ptr = mami_malloc(memory_manager, nmemb*size, policy, node);
  ptr = memset(ptr, 0, nmemb*size);

  MEMORY_LOG_OUT();
  return ptr;
}

int _mami_locate(mami_manager_t *memory_manager, mami_tree_t *memory_tree, void *buffer, size_t size, mami_data_t **data) {
  if (memory_tree==NULL) {
    mdebug_memory("The interval [%p:%p] is not managed by MaMI.\n", buffer, buffer+size);
    errno = EINVAL;
    return -1;
  }
  //mdebug_memory("Comparing [%p:%p] and [%p:%p]\n", buffer,buffer+size, memory_tree->data->start_address, memory_tree->data->end_address);
  if (buffer >= memory_tree->data->start_address && buffer+size <= memory_tree->data->end_address) {
    // the address is stored on the current memory_data
    mdebug_memory("Found interval [%p:%p] in [%p:%p]\n", buffer, buffer+size, memory_tree->data->start_address, memory_tree->data->end_address);
    mdebug_memory("Interval [%p:%p] is located on node #%d\n", buffer, buffer+size, memory_tree->data->node);
    *data = memory_tree->data;
    return 0;
  }
  else if (buffer <= memory_tree->data->start_address) {
    return _mami_locate(memory_manager, memory_tree->left_child, buffer, size, data);
  }
  else if (buffer >= memory_tree->data->end_address) {
    return _mami_locate(memory_manager, memory_tree->right_child, buffer, size, data);
  }
  else {
    errno = EINVAL;
    return -1;
  }
}

int _mami_get_pages_location(mami_manager_t *memory_manager, void **pageaddrs, int nb_pages, int *node, int *nodes) {
  int i, err=0;

  MEMORY_ILOG_IN();
  err = _mm_move_pages(pageaddrs, nb_pages, NULL, nodes, 0);
  mdebug_memory("Locating %d pages --> err=%d\n", nb_pages, err);
  if (err < 0) {
    mdebug_memory("Could not locate pages (err = %d - nodes[0] = %d)\n", err, nodes[0]);
    if (*node != MAMI_FIRST_TOUCH_NODE) {
      *node = MAMI_UNKNOWN_LOCATION_NODE;
    }
    errno = ENOENT;
    err = -1;
  }
  else {
    // Are all the pages not present */
    int unlocated = 1;
    for(i=0 ; i<nb_pages ; i++) {
      if (nodes[i] != -ENOENT) unlocated=0;
    }
    if (unlocated) {
      mdebug_memory("All the pages are not present\n");
      if (*node != MAMI_FIRST_TOUCH_NODE) {
        *node = MAMI_UNKNOWN_LOCATION_NODE;
      }
    }
    else {
      *node = nodes[0];
      for(i=1 ; i<nb_pages ; i++) {
        if (nodes[i] != nodes[0]) {
#ifdef PM2DEBUG
          th_mami_fprintf(stderr, "#MaMI Warning: Memory located on different nodes\n");
#endif
          if (*node != MAMI_FIRST_TOUCH_NODE) {
            *node = MAMI_MULTIPLE_LOCATION_NODE;
          }
          break;
        }
      }
    }
  }
  MEMORY_ILOG_OUT();
  return err;
}

int _mami_register(mami_manager_t *memory_manager,
                   void *buffer,
                   void *endbuffer,
                   size_t size,
                   void *initial_buffer,
                   size_t initial_size,
                   int mami_allocated,
                   mami_data_t **data) {
  void **pageaddrs;
  int nb_pages, protection, with_huge_pages;
  int i, node, *nodes;
  mami_data_t *pdata = NULL;
  int err=0;

  mdebug_memory("Registering address interval [%p:%p:%lu]\n", buffer, buffer+size, (long)size);

  err = _mami_locate(memory_manager, memory_manager->root, buffer, 1, &pdata);
  if (err >= 0) {
    mdebug_memory("The memory area is already registered.\n");
    errno = EINVAL;
    return -1;
  }

  with_huge_pages = 0;
  protection = PROT_READ|PROT_WRITE;

  // Count the number of pages
  nb_pages = size / memory_manager->normal_page_size;
  if (nb_pages * memory_manager->normal_page_size != size) nb_pages++;

  // Set the page addresses
  pageaddrs = th_mami_malloc(nb_pages * sizeof(void *));
  for(i=0; i<nb_pages ; i++) pageaddrs[i] = buffer + i*memory_manager->normal_page_size;

  // Find out where the pages are
  node=0;
  nodes = th_mami_malloc(nb_pages * sizeof(int));
  _mami_get_pages_location(memory_manager, pageaddrs, nb_pages, &node, nodes);

  // Register the pages
  _mami_register_pages(memory_manager, &(memory_manager->root), pageaddrs, nb_pages, size, node, nodes,
                       protection, with_huge_pages, mami_allocated, data);

  if (endbuffer > initial_buffer+initial_size) {
    (*data)->mprotect_size = (*data)->size - memory_manager->normal_page_size;
  }
  if (buffer < initial_buffer) {
    (*data)->mprotect_start_address += memory_manager->normal_page_size;
    (*data)->mprotect_size -= memory_manager->normal_page_size;
  }

  // Free temporary array
  th_mami_free(nodes);
  th_mami_free(pageaddrs);
  return 0;
}

int mami_register(mami_manager_t *memory_manager,
                  void *buffer,
                  size_t size) {
  void *aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normal_page_size);
  void *aligned_endbuffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer+size, memory_manager->normal_page_size);
  size_t aligned_size = aligned_endbuffer-aligned_buffer;
  mami_data_t *data = NULL;
  int err=0;

  MEMORY_LOG_IN();
  if (aligned_size > size) aligned_size = size;
  if (aligned_buffer == aligned_endbuffer) {
    mdebug_memory("Cannit register a memory area smaller than a page\n");
    errno = EINVAL;
    err = -1;
  }
  else {
    th_mami_mutex_lock(&(memory_manager->lock));
    mdebug_memory("Registering [%p:%p:%ld]\n", aligned_buffer, aligned_buffer+aligned_size, (long)aligned_size);
    err = _mami_register(memory_manager, aligned_buffer, aligned_endbuffer, aligned_size, buffer, size, 0, &data);
    th_mami_mutex_unlock(&(memory_manager->lock));
  }
  MEMORY_LOG_OUT();
  return err;
}

int mami_unregister(mami_manager_t *memory_manager,
                    void *buffer) {
  mami_data_t *data = NULL;
  int err=0;
  void *aligned_buffer;

  MEMORY_LOG_IN();
  th_mami_mutex_lock(&(memory_manager->lock));

  aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normal_page_size);
  err = _mami_locate(memory_manager, memory_manager->root, aligned_buffer, 1, &data);
  if (err >= 0) {
    mdebug_memory("Unregistering [%p:%p]\n", buffer,buffer+data->size);
    _mami_unregister(memory_manager, &(memory_manager->root), aligned_buffer);
  }
  th_mami_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
  return err;
}

int mami_split(mami_manager_t *memory_manager,
               void *buffer,
               unsigned int subareas,
               void **newbuffers) {
  mami_data_t *data = NULL;
  int err=0;

  MEMORY_LOG_IN();
  th_mami_mutex_lock(&(memory_manager->lock));

  err = _mami_locate(memory_manager, memory_manager->root, buffer, 1, &data);
  if (err >= 0) {
    size_t subsize;
    int subpages, i;
    void **pageaddrs;

    subpages = data->nb_pages / subareas;
    if (!subpages) {
      mdebug_memory("Memory area from %p with size %ld and %d pages not big enough to be split in %d subareas\n",
                    data->start_address, (long) data->size, data->nb_pages, subareas);
      errno = EINVAL;
      err = -1;
    }
    else {
      mdebug_memory("Splitting area from %p in %d areas of %d pages\n", data->start_address, subareas, subpages);

      // Register new subareas
      pageaddrs = data->pageaddrs+subpages;
      subsize = (data->with_huge_pages) ? subpages * memory_manager->huge_page_size : subpages * memory_manager->normal_page_size;
      for(i=1 ; i<subareas ; i++) {
	if (newbuffers) newbuffers[i] = pageaddrs[0];
	_mami_register_pages(memory_manager, &(memory_manager->root), pageaddrs, subpages, subsize, data->node, data->nodes,
                             data->protection, data->with_huge_pages, data->mami_allocated, NULL);
	pageaddrs += subpages;
      }

      // Modify initial memory area
      if (newbuffers) newbuffers[0] = buffer;
      data->nb_pages = subpages;
      data->end_address = data->start_address + subsize;
      data->size = subsize;
    }
  }
  th_mami_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
  return err;
}

int mami_membind(mami_manager_t *memory_manager,
                 mami_membind_policy_t policy,
                 int node) {
  int err=0;

  MEMORY_LOG_IN();
  if (tbx_unlikely(node >= memory_manager->nb_nodes)) {
    mdebug_memory("Node #%d invalid\n", node);
    errno = EINVAL;
    err = -1;
  }
  else if (policy == MAMI_MEMBIND_POLICY_HUGE_PAGES && memory_manager->huge_page_size == 0) {
    mdebug_memory("Huge pages are not available. Cannot set memory policy.\n");
    errno = EINVAL;
    err = -1;
  }
  else {
    mdebug_memory("Set the current membind policy to %d (node #%d)\n", policy, node);
    memory_manager->membind_policy = policy;
    memory_manager->membind_node = node;
  }
  MEMORY_LOG_OUT();
  return err;
}

void mami_free(mami_manager_t *memory_manager, void *buffer) {
  void *aligned_buffer;

  MEMORY_LOG_IN();
  aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normal_page_size);
  mdebug_memory("Freeing [%p]\n", buffer);
  th_mami_mutex_lock(&(memory_manager->lock));
  _mami_unregister(memory_manager, &(memory_manager->root), aligned_buffer);
  th_mami_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
}

int _mami_set_mempolicy(int mode, const unsigned long *nmask, unsigned long maxnode) {
  int err=0;

  MEMORY_ILOG_IN();
  if (_mm_use_synthetic_topology()) {
    MEMORY_ILOG_OUT();
    return err;
  }
  err = syscall(__NR_set_mempolicy, mode, nmask, maxnode);
  if (err < 0) perror("(_mami_set_mempolicy) set_mempolicy");
  MEMORY_ILOG_OUT();
  return err;
}

int _mami_check_pages_location(void **pageaddrs, int pages, int node) {
  int *pagenodes;
  int i;
  int err=0;

  MEMORY_ILOG_IN();
  mdebug_memory("check location of the %d pages is #%d\n", pages, node);

  if (_mm_use_synthetic_topology()) {
    mdebug_memory("Using synthethic topology. No need to check physically\n");
    return err;
  }

  pagenodes = th_mami_malloc(pages * sizeof(int));
  err = _mm_move_pages(pageaddrs, pages, NULL, pagenodes, 0);
  if (err < 0) perror("(_mami_check_pages_location) move_pages");
  else {
    for(i=0; i<pages; i++) {
      if (pagenodes[i] != node) {
        th_mami_fprintf(stderr, "MaMI Warning: page #%d is not located on node #%d but on node #%d\n", i, node, pagenodes[i]);
        errno = EINVAL;
        err = -1;
      }
    }
  }
  th_mami_free(pagenodes);
  MEMORY_ILOG_OUT();
  return err;
}

int mami_locate(mami_manager_t *memory_manager, void *buffer, size_t size, int *node) {
  mami_data_t *data = NULL;
  void *aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normal_page_size);
  void *aligned_endbuffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer+size, memory_manager->normal_page_size);
  size_t aligned_size = aligned_endbuffer-aligned_buffer;
  int err=0;

  MEMORY_LOG_IN();
  if (aligned_size > size) aligned_size = size;
  mdebug_memory("Trying to locate [%p:%p:%ld]\n", aligned_buffer, aligned_buffer+aligned_size, (long)aligned_size);
  err = _mami_locate(memory_manager, memory_manager->root, aligned_buffer, aligned_size, &data);
  if (err >= 0) {
    if (data->status == MAMI_KERNEL_MIGRATION_STATUS) {
      int node;
      int *nodes = th_mami_malloc(data->nb_pages * sizeof(int));
      _mami_get_pages_location(memory_manager, data->pageaddrs, data->nb_pages, &node, nodes);
      if (node != data->node) {
        mdebug_memory("Updating location of the pages after a kernel migration\n");
        data->node = node;
        data->status = MAMI_INITIAL_STATUS;
      }
      th_mami_free(nodes);
    }
    *node = data->node;
  }
  MEMORY_LOG_OUT();
  return err;
}

int mami_check_pages_location(mami_manager_t *memory_manager, void *buffer, size_t size, int node) {
  mami_data_t *data = NULL;
  void *aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normal_page_size);
  void *aligned_endbuffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer+size, memory_manager->normal_page_size);
  size_t aligned_size = aligned_endbuffer-aligned_buffer;
  int err=0;

  MEMORY_LOG_IN();
  if (aligned_size > size) aligned_size = size;
  err = _mami_locate(memory_manager, memory_manager->root, aligned_buffer, aligned_size, &data);
  if (err >= 0) {
    err = _mami_check_pages_location(data->pageaddrs, data->nb_pages, node);
    if (err < 0) {
      th_mami_fprintf(stderr, "MaMI: The %d pages are NOT all on node #%d\n", data->nb_pages, node);
    }
    else {
      th_mami_fprintf(stderr, "MaMI: The %d pages are all on node #%d\n", data->nb_pages, node);
    }
  }
  MEMORY_LOG_OUT();
  return err;
}

int mami_update_pages_location(mami_manager_t *memory_manager,
                               void *buffer,
                               size_t size) {
  mami_data_t *data = NULL;
  void *aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normal_page_size);
  void *aligned_endbuffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer+size, memory_manager->normal_page_size);
  size_t aligned_size = aligned_endbuffer-aligned_buffer;
  int err=0;

  MEMORY_LOG_IN();
  if (aligned_size > size) aligned_size = size;
  err = _mami_locate(memory_manager, memory_manager->root, aligned_buffer, aligned_size, &data);
  if (err >= 0) {
    int node;
    int *nodes = th_mami_malloc(data->nb_pages * sizeof(int));
    _mami_get_pages_location(memory_manager, data->pageaddrs, data->nb_pages, &node, nodes);
    if (node != data->node) {
      mdebug_memory("Updating pages location from node #%d to node #%d\n", data->node, node);
      data->node = node;
    }
    th_mami_free(nodes);
  }
  MEMORY_LOG_OUT();
  return err;
}

static
void _mami_print(mami_tree_t *memory_tree, FILE *stream, int indent) {
  if (memory_tree) {
    int x;
    _mami_print(memory_tree->left_child, stream, indent+2);
    for(x=0 ; x<indent ; x++) th_mami_fprintf(stream, " ");
    th_mami_fprintf(stream, "[%p, %p]\n", memory_tree->data->start_address, memory_tree->data->end_address);
    _mami_print(memory_tree->right_child, stream, indent+2);
  }
}

void mami_print(mami_manager_t *memory_manager) {
  MEMORY_LOG_IN();
  mdebug_memory("******************** TREE BEGIN *********************************\n");
  _mami_print(memory_manager->root, stdout, 0);
  mdebug_memory("******************** TREE END *********************************\n");
  MEMORY_LOG_OUT();
}

void mami_fprint(mami_manager_t *memory_manager, FILE *stream) {
  MEMORY_LOG_IN();
  mdebug_memory("******************** TREE BEGIN *********************************\n");
  _mami_print(memory_manager->root, stream, 0);
  mdebug_memory("******************** TREE END *********************************\n");
  MEMORY_LOG_OUT();
}

int mami_migration_cost(mami_manager_t *memory_manager,
                        int source,
                        int dest,
                        size_t size,
                        float *cost) {
  p_tbx_slist_t migration_costs;
  int err=0;

  MEMORY_LOG_IN();
  if (tbx_unlikely(source >= memory_manager->nb_nodes || dest >= memory_manager->nb_nodes)) {
    mdebug_memory("Invalid node id\n");
    errno = EINVAL;
    err = -1;
  }
  else {
    migration_costs = memory_manager->migration_costs[source][dest];
    if (!tbx_slist_is_nil(migration_costs)) {
      tbx_slist_ref_to_head(migration_costs);
      do {
	mami_migration_cost_t *object = NULL;
	object = tbx_slist_ref_get(migration_costs);

	if (size >= object->size_min && size <= object->size_max) {
	  *cost = (object->slope * size) + object->intercept;
	  break;
	}
      } while (tbx_slist_ref_forward(migration_costs));
    }
  }
  MEMORY_LOG_OUT();
  return err;
}

int mami_cost_for_write_access(mami_manager_t *memory_manager,
                               int source,
                               int dest,
                               size_t size,
                               float *cost) {
  mami_access_cost_t access_cost;
  int err=0;

  MEMORY_LOG_IN();
  if (tbx_unlikely(source >= memory_manager->nb_nodes || dest >= memory_manager->nb_nodes)) {
    mdebug_memory("Invalid node id\n");
    errno = EINVAL;
    err = -1;
  }
  else {
    access_cost = memory_manager->costs_for_write_access[source][dest];
    *cost = (size/memory_manager->cache_line_size) * access_cost.cost;
  }
  MEMORY_LOG_OUT();
  return err;
}

int mami_cost_for_read_access(mami_manager_t *memory_manager,
                              int source,
                              int dest,
                              size_t size,
                              float *cost) {
  mami_access_cost_t access_cost;
  int err=0;

  MEMORY_LOG_IN();
  if (tbx_unlikely(source >= memory_manager->nb_nodes || dest >= memory_manager->nb_nodes)) {
    mdebug_memory("Invalid node id\n");
    errno = EINVAL;
    err = -1;
  }
  else {
    access_cost = memory_manager->costs_for_read_access[source][dest];
    *cost = ((float)size/(float)memory_manager->cache_line_size) * access_cost.cost;
  }
  MEMORY_LOG_OUT();
  return err;
}

int mami_select_node(mami_manager_t *memory_manager,
                     mami_node_selection_policy_t policy,
                     int *node) {
  int err=0;

  MEMORY_LOG_IN();
  th_mami_mutex_lock(&(memory_manager->lock));

  if (policy == MAMI_LEAST_LOADED_NODE) {
    int i;
    unsigned long maxspace=memory_manager->mem_free[0];
    mdebug_memory("Space on node %d = %ld\n", 0, maxspace);
    *node = 0;
    for(i=1 ; i<memory_manager->nb_nodes ; i++) {
      mdebug_memory("Space on node %d = %ld\n", i, memory_manager->mem_free[i]);
      if (memory_manager->mem_free[i] > maxspace) {
        maxspace = memory_manager->mem_free[i];
        *node = i;
      }
    }
  }
  else {
    mdebug_memory("Policy #%d unknown\n", policy);
    errno = EINVAL;
    err = -1;
  }

  th_mami_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
  return err;
}

int _mami_migrate_on_node(mami_manager_t *memory_manager,
                          mami_data_t *data,
                          int dest) {
  int err=0;

  MEMORY_ILOG_IN();
  if (data->node == dest) {
    mdebug_memory("The address %p is already located at the required node.\n", data->start_address);
    errno = EALREADY;
    err = -1;
  }
  else {
    unsigned long nodemask;

    mdebug_memory("Mbinding %d page(s) to node #%d\n", data->nb_pages, dest);
    nodemask = (1<<dest);
    err = _mm_mbind(data->start_address, data->size, MPOL_BIND, &nodemask, memory_manager->max_node, MPOL_MF_MOVE|MPOL_MF_STRICT);
    if (err < 0) {
      mdebug_memory("Error when mbinding: %d\n", err);
    }
    else {
      _mami_update_stats_for_entities(memory_manager, data, -1);
      data->node = dest;
      _mami_update_stats_for_entities(memory_manager, data, +1);
    }
  }

  MEMORY_ILOG_OUT();
  return err;
}

int mami_migrate_on_node(mami_manager_t *memory_manager,
                         void *buffer,
                         int dest) {
  int err=0;
  mami_data_t *data = NULL;

  MEMORY_LOG_IN();
  th_mami_mutex_lock(&(memory_manager->lock));
  err = _mami_locate(memory_manager, memory_manager->root, buffer, 1, &data);
  if (err >= 0) {
    err = _mami_migrate_on_node(memory_manager, data, dest);
  }
  th_mami_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
  return err;
}

static
void _mami_segv_handler(int sig, siginfo_t *info, void *_context) {
  void *addr;
  int err=0, dest;
  mami_data_t *data = NULL;

  th_mami_mutex_lock(&(_mami_memory_manager->lock));

#warning look at www.gnu.org/software/libsigsegv/

  addr = info->si_addr;
  err = _mami_locate(_mami_memory_manager, _mami_memory_manager->root, addr, 1, &data);
  if (err < 0) {
    // The address is not managed by MaMI. Reset the segv handler to its default action, to cause a segfault
    struct sigaction act;
    act.sa_flags = SA_SIGINFO;
    act.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &act, NULL);
  }
  if (data && data->status != MAMI_NEXT_TOUCHED_STATUS) {
    data->status = MAMI_NEXT_TOUCHED_STATUS;
    dest = th_mami_current_node();
    _mami_migrate_on_node(_mami_memory_manager, data, dest);
    err = mprotect(data->mprotect_start_address, data->mprotect_size, data->protection);
    if (err < 0) {
      const char *msg = "mprotect(handler): ";
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
  th_mami_mutex_unlock(&(_mami_memory_manager->lock));
}

int mami_migrate_on_next_touch(mami_manager_t *memory_manager, void *buffer) {
  int err=0;
  mami_data_t *data = NULL;
  void *aligned_buffer;

  MEMORY_LOG_IN();
  th_mami_mutex_lock(&(memory_manager->lock));

  _mami_memory_manager = memory_manager;
  aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normal_page_size);
  mdebug_memory("Trying to locate [%p:%p:1]\n", aligned_buffer, aligned_buffer+1);
  err = _mami_locate(memory_manager, memory_manager->root, aligned_buffer, 1, &data);
  if (err >= 0) {
    mdebug_memory("Setting migrate on next touch on address %p (%p)\n", data->start_address, buffer);
    if (memory_manager->kernel_nexttouch_migration_requested > 0 && data->mami_allocated) {
      data->status = MAMI_KERNEL_MIGRATION_STATUS;
      if (memory_manager->kernel_nexttouch_migration_available == MAMI_KERNEL_NEXT_TOUCH_MOF) {
        mdebug_memory("... using in-kernel migration\n");
        err = _mm_mbind(data->start_address, data->size, MPOL_PREFERRED, NULL, 0, MPOL_MF_MOVE|MPOL_MF_LAZY);
        if (err < 0) {
          perror("(mami_migrate_on_next_touch) mbind");
        }
      }
      else {
        mdebug_memory("... using in-kernel migration (brice's patch)\n");
        err = _mm_mbind(data->start_address, data->size, MPOL_DEFAULT, NULL, 0, 0);
        if (err < 0) {
          perror("(mami_migrate_on_next_touch) mbind");
        }
        else {
          err = madvise(data->start_address, data->size, 12);
          if (err < 0) {
            perror("(mami_migrate_on_next_touch) madvise");
          }
        }
      }
    }
    else {
      mdebug_memory("... using user-space migration\n");
      if (!_mami_sigsegv_handler_set) {
        struct sigaction act;
        _mami_sigsegv_handler_set = 1;
        act.sa_flags = SA_SIGINFO;
        act.sa_sigaction = _mami_segv_handler;
        err = sigaction(SIGSEGV, &act, NULL);
        if (err < 0) {
          perror("(mami_migrate_on_next_touch) sigaction");
          th_mami_mutex_unlock(&(memory_manager->lock));
          MEMORY_LOG_OUT();
          return -1;
        }
      }
      mdebug_memory("Mprotecting [%p:%p:%ld] (initially [%p:%p:%ld]\n", data->mprotect_start_address, data->start_address+data->mprotect_size,
		    (long) data->mprotect_size, data->start_address, data->end_address, (long) data->size);
      data->status = MAMI_INITIAL_STATUS;
      err = mprotect(data->mprotect_start_address, data->mprotect_size, PROT_NONE);
      if (err < 0) {
        perror("(mami_migrate_on_next_touch) mprotect");
      }
    }
  }
  th_mami_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
  return err;
}

int mami_huge_pages_available(mami_manager_t *memory_manager) {
  return (memory_manager->huge_page_size != 0);
}

int mami_unset_kernel_migration(mami_manager_t *memory_manager) {
  memory_manager->kernel_nexttouch_migration_requested = 0;
  return 0;
}

int mami_set_kernel_migration(mami_manager_t *memory_manager) {
  if (memory_manager->kernel_nexttouch_migration_available)
    memory_manager->kernel_nexttouch_migration_requested = 1;
  return memory_manager->kernel_nexttouch_migration_available;
}

int mami_stats(mami_manager_t *memory_manager,
               int node,
               mami_stats_t stat,
               unsigned long *value) {
  int err=0;

  MEMORY_LOG_IN();

  if (tbx_unlikely(node >= memory_manager->nb_nodes)) {
    mdebug_memory("Node #%d invalid\n", node);
    errno = EINVAL;
    err = -1;
  }
  else {
    th_mami_mutex_lock(&(memory_manager->lock));
    if (stat == MAMI_STAT_MEMORY_TOTAL) {
      *value = memory_manager->mem_total[node];
    }
    else if (stat == MAMI_STAT_MEMORY_FREE) {
      *value = memory_manager->mem_free[node];
    }
    else {
      mdebug_memory("Statistic #%d unknown\n", stat);
      errno = EINVAL;
      err = -1;
    }
    th_mami_mutex_unlock(&(memory_manager->lock));
  }

  MEMORY_LOG_OUT();
  return err;
}

int mami_distribute(mami_manager_t *memory_manager,
                    void *buffer,
                    int *nodes,
                    int nb_nodes) {
  mami_data_t *data;
  int err=0, i, nb_pages;

  MEMORY_LOG_IN();

  th_mami_mutex_lock(&(memory_manager->lock));
  err = _mami_locate(memory_manager, memory_manager->root, buffer, 1, &data);

  if (err >= 0) {
    void **pageaddrs;
    int *dests, *status;

    dests = th_mami_malloc(data->nb_pages * sizeof(int));
    status = th_mami_malloc(data->nb_pages * sizeof(int));
    pageaddrs = th_mami_malloc(data->nb_pages * sizeof(void *));
    nb_pages=0;
    // Find out which pages have to be moved
    for(i=0 ; i<data->nb_pages ; i++) {
      if ((data->node == MAMI_MULTIPLE_LOCATION_NODE && data->nodes[i] != nodes[i%nb_nodes]) ||
          (data->node != MAMI_MULTIPLE_LOCATION_NODE && data->node != nodes[i%nb_nodes])) {
        mdebug_memory("Moving page %d from node #%d to node #%d\n", i, data->node != MAMI_MULTIPLE_LOCATION_NODE ? data->node : data->nodes[i], nodes[i%nb_nodes]);
        pageaddrs[nb_pages] = buffer + (i*memory_manager->normal_page_size);
        dests[nb_pages] = nodes[i%nb_nodes];
        nb_pages ++;
      }
    }

    // If there is some pages to be moved, move them
    if (nb_pages != 0) {
      err = _mm_move_pages(pageaddrs, nb_pages, dests, status, MPOL_MF_MOVE);

      if (err >= 0) {
        // If some threads are attached to the memory area, the statistics need to be updated
        _mami_update_stats_for_entities(memory_manager, data, -1);

        // Check the pages have been properly moved and update the data->nodes information
        if (data->pages_per_node == NULL) data->pages_per_node = th_mami_malloc(memory_manager->nb_nodes * sizeof(int));
        memset(data->pages_per_node, 0, memory_manager->nb_nodes*sizeof(int));
        data->pages_on_undefined_node = 0;
        if (data->nodes == NULL) data->nodes = th_mami_malloc(data->nb_pages * sizeof(int));
        data->node = MAMI_MULTIPLE_LOCATION_NODE;
        err = _mm_move_pages(data->pageaddrs, data->nb_pages, NULL, status, 0);
        for(i=0 ; i<data->nb_pages ; i++) {
          if (status[i] != nodes[i%nb_nodes]) {
            th_mami_fprintf(stderr, "MaMI Warning: Page %d is on node %d, but it should be on node %d\n", i, status[i], nodes[i%nb_nodes]);
          }
          data->nodes[i] = status[i];
          if (data->nodes[i] >= 0) data->pages_per_node[data->nodes[i]] ++; else data->pages_on_undefined_node ++;
        }

        // If some threads are attached to the memory area, the statistics need to be updated
        _mami_update_stats_for_entities(memory_manager, data, +1);
      }
    }
    th_mami_free(dests);
    th_mami_free(status);
    th_mami_free(pageaddrs);
  }

  th_mami_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
  return err;
}

int mami_gather(mami_manager_t *memory_manager,
                void *buffer,
                int node) {
  mami_data_t *data;
  int err=0;

  MEMORY_LOG_IN();

  th_mami_mutex_lock(&(memory_manager->lock));
  err = _mami_locate(memory_manager, memory_manager->root, buffer, 1, &data);

  if ((err >= 0) && (data->nodes)) {
    int i, *dests, *status;
    int nb_pages = 0;
    void **pageaddrs;

    dests = th_mami_malloc(data->nb_pages * sizeof(int));
    status = th_mami_malloc(data->nb_pages * sizeof(int));
    pageaddrs  = th_mami_malloc(data->nb_pages * sizeof(void *));

    for(i=0 ; i<data->nb_pages ; i++) {
      dests[i] = node;
      if (data->nodes[i] != node) {
        pageaddrs[nb_pages] = data->pageaddrs[i];
        nb_pages ++;
      }
    }

    // If some threads are attached to the memory area, the statistics need to be updated
    _mami_update_stats_for_entities(memory_manager, data, -1);

    err = _mm_move_pages(pageaddrs, nb_pages, dests, status, MPOL_MF_MOVE);
    if (err >= 0) {
      data->node = node;
      th_mami_free(data->nodes);
      data->nodes = NULL;
    }

    // If some threads are attached to the memory area, the statistics need to be updated
    _mami_update_stats_for_entities(memory_manager, data, +1);

    th_mami_free(dests);
    th_mami_free(status);
    th_mami_free(pageaddrs);
  }

  th_mami_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
  return err;
}

#if !defined(MARCEL)
int _mami_current_node(void) {
#warning undefined
  return -1;
}

int _mami_attr_settopo_level(th_mami_attr_t *attr, int node) {
#warning undefined
  return -1;
}
#endif /* !MARCEL */

#endif /* MM_MAMI_ENABLED */
