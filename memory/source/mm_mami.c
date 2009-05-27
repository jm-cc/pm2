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
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef LINUX_SYS
#  include <malloc.h>
#endif /* LINUX_SYS */
#include "mm_mami.h"
#include "mm_mami_private.h"
#include "mm_debug.h"
#include "mm_helper.h"

static
mami_manager_t *g_memory_manager = NULL;
static
int memory_manager_sigsegv_handler_set = 0;

void mami_init(mami_manager_t *memory_manager) {
  int node, dest;
  void *ptr;
  int err;

  MEMORY_LOG_IN();
  memory_manager->root = NULL;
  marcel_mutex_init(&(memory_manager->lock), NULL);
  memory_manager->normalpagesize = getpagesize();
  memory_manager->hugepagesize = marcel_topo_node_level ? marcel_topo_node_level[0].huge_page_size : 0;
  memory_manager->initially_preallocated_pages = 1024;
  memory_manager->cache_line_size = 64;
  memory_manager->membind_policy = MAMI_MEMBIND_POLICY_NONE;
  memory_manager->alignment = 1;
  memory_manager->nb_nodes = marcel_nbnodes;
  memory_manager->max_node = marcel_topo_node_level ? marcel_topo_node_level[memory_manager->nb_nodes-1].os_node+2 : 2;

  // Is in-kernel migration available
#ifdef LINUX_SYS
  ptr = memalign(memory_manager->normalpagesize, memory_manager->normalpagesize);
  err = madvise(ptr, memory_manager->normalpagesize, 12);
  memory_manager->kernel_nexttouch_migration = (err>=0);
  free(ptr);
#else /* !LINUX_SYS */
  memory_manager->kernel_nexttouch_migration = 0;
#endif /* LINUX_SYS */
  mdebug_memory("Kernel next_touch migration: %d\n", memory_manager->kernel_nexttouch_migration);

  // Is migration available
  {
    if (marcel_topo_node_level) {
      unsigned long nodemask;
      nodemask = (1<<marcel_topo_node_level[0].os_node);
      ptr = memalign(memory_manager->normalpagesize, memory_manager->normalpagesize);
      err = _mm_mbind(ptr,  memory_manager->normalpagesize, MPOL_BIND, &nodemask, memory_manager->max_node, MPOL_MF_MOVE);
      memory_manager->migration_flag = err>=0 ? MPOL_MF_MOVE : 0;
      free(ptr);
    }
    else {
      memory_manager->migration_flag = 0;
    }
    mdebug_memory("Migration: %d\n", memory_manager->migration_flag);
  }

  // How much total and free memory per node
  memory_manager->memtotal = tmalloc(memory_manager->nb_nodes * sizeof(unsigned long));
  memory_manager->memfree = tmalloc(memory_manager->nb_nodes * sizeof(unsigned long));
  if (marcel_topo_node_level) {
    for(node=0 ; node<memory_manager->nb_nodes ; node++) {
      memory_manager->memtotal[node] = marcel_topo_node_level[node].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_NODE];
      memory_manager->memfree[node] = marcel_topo_node_level[node].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_NODE];
      mdebug_memory("Memory on node #%d = %ld\n", node, memory_manager->memtotal[node]);
    }
  }
  else {
    memory_manager->memtotal[0] = marcel_topo_levels[0][0].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_MACHINE];
    memory_manager->memfree[0] = marcel_topo_levels[0][0].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_MACHINE];
  }

  // Preallocate memory on each node
  memory_manager->heaps = tmalloc((memory_manager->nb_nodes+1) * sizeof(mami_area_t *));
  if (marcel_topo_node_level) {
    for(node=0 ; node<memory_manager->nb_nodes ; node++) {
      _mami_preallocate(memory_manager, &(memory_manager->heaps[node]), memory_manager->initially_preallocated_pages, node, marcel_topo_node_level[node].os_node);
      mdebug_memory("Preallocating %p for node #%d %d\n", memory_manager->heaps[node]->start, node, marcel_topo_node_level[node].os_node);
    }
  }
  else {
    _mami_preallocate(memory_manager, &(memory_manager->heaps[0]), memory_manager->initially_preallocated_pages, 0, 0);
    mdebug_memory("Preallocating %p for node #%d %d\n", memory_manager->heaps[0]->start, 0, 0);
  }
  _mami_preallocate(memory_manager, &(memory_manager->heaps[MAMI_FIRST_TOUCH_NODE]), memory_manager->initially_preallocated_pages, MAMI_FIRST_TOUCH_NODE, MAMI_FIRST_TOUCH_NODE);
  mdebug_memory("Preallocating %p for anonymous heap\n", memory_manager->heaps[MAMI_FIRST_TOUCH_NODE]->start);

  // Initialise space with huge pages on each node
  memory_manager->huge_pages_heaps = tmalloc(memory_manager->nb_nodes * sizeof(mami_huge_pages_area_t *));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    memory_manager->huge_pages_heaps[node] = NULL;
  }

  // Load the model for the migration costs
  memory_manager->migration_costs = tmalloc(memory_manager->nb_nodes * sizeof(p_tbx_slist_t *));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    memory_manager->migration_costs[node] = tmalloc(memory_manager->nb_nodes * sizeof(p_tbx_slist_t));
    for(dest=0 ; dest<memory_manager->nb_nodes ; dest++) {
      memory_manager->migration_costs[node][dest] = tbx_slist_nil();
    }
  }
  _mami_load_model_for_memory_migration(memory_manager);

  // Load the model for the access costs
  memory_manager->costs_for_write_access = tmalloc(memory_manager->nb_nodes * sizeof(mami_access_cost_t *));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    memory_manager->costs_for_write_access[node] = tmalloc(memory_manager->nb_nodes * sizeof(mami_access_cost_t));
    for(dest=0 ; dest<memory_manager->nb_nodes ; dest++) {
      memory_manager->costs_for_write_access[node][dest].cost = 0;
    }
  }
  memory_manager->costs_for_read_access = tmalloc(memory_manager->nb_nodes * sizeof(mami_access_cost_t *));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    memory_manager->costs_for_read_access[node] = tmalloc(memory_manager->nb_nodes * sizeof(mami_access_cost_t));
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

            marcel_fprintf(stderr, "[%d:%d] [%ld:%ld] %f %f %f\n", node, dest, (long)object->size_min, (long)object->size_max, object->slope, object->intercept, object->correlation);
          } while (tbx_slist_ref_forward(migration_costs));
        }
      }
    }

    for(node=0 ; node<memory_manager->nb_nodes ; node++) {
      for(dest=0 ; dest<memory_manager->nb_nodes ; dest++) {
        mami_access_cost_t wcost = memory_manager->costs_for_write_access[node][dest];
        mami_access_cost_t rcost = memory_manager->costs_for_write_access[node][dest];
        marcel_fprintf(stderr, "[%d:%d] %f %f\n", node, dest, wcost.cost, rcost.cost);
      }
    }
  }
#endif /* PM2DEBUG */

  MEMORY_LOG_OUT();
}

static
void _mami_sampling_free(p_tbx_slist_t migration_costs) {
  MEMORY_ILOG_IN();
  while (!tbx_slist_is_nil(migration_costs)) {
    mami_migration_cost_t *ptr = tbx_slist_extract(migration_costs);
    tfree(ptr);
  }
  tbx_slist_free(migration_costs);
  MEMORY_ILOG_OUT();
}

static
int _mami_deallocate_huge_pages(mami_manager_t *memory_manager, mami_huge_pages_area_t **space, int node) {
  int err;

  MEMORY_ILOG_IN();
  mdebug_memory("Releasing huge pages %s\n", (*space)->filename);

  err = close((*space)->file);
  if (err < 0) {
    perror("(_mami_deallocate_huge_pages) close");
    err = -errno;
  }
  else {
    err = unlink((*space)->filename);
    if (err < 0) {
      perror("(_mami_deallocate_huge_pages) unlink");
      err = -errno;
    }
  }

  tfree((*space)->filename);
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
    munmap(ptr->start, ptr->nbpages * ptr->pagesize);
    ptr2 = ptr;
    ptr = ptr->next;
    tfree(ptr2);
  }
  MEMORY_ILOG_OUT();
}

static
void _mami_clean_memory(mami_manager_t *memory_manager) {
  while (memory_manager->root) {
    mami_free(memory_manager, memory_manager->root->data->startaddress);
  }
}

void mami_exit(mami_manager_t *memory_manager) {
  int node, dest;
  struct sigaction act;

  MEMORY_LOG_IN();

  if (memory_manager_sigsegv_handler_set) {
    act.sa_flags = SA_SIGINFO;
    act.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &act, NULL);
  }

  if (memory_manager->root) {
    marcel_fprintf(stderr, "MaMI Warning: some memory areas have not been free-d\n");
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
  tfree(memory_manager->heaps);

  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    if (memory_manager->huge_pages_heaps[node]) {
      _mami_deallocate_huge_pages(memory_manager, &(memory_manager->huge_pages_heaps[node]), node);
    }
  }
  tfree(memory_manager->huge_pages_heaps);

  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    for(dest=0 ; dest<memory_manager->nb_nodes ; dest++) {
      _mami_sampling_free(memory_manager->migration_costs[node][dest]);
    }
    tfree(memory_manager->migration_costs[node]);
  }
  tfree(memory_manager->migration_costs);

  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    tfree(memory_manager->costs_for_read_access[node]);
    tfree(memory_manager->costs_for_write_access[node]);
  }
  tfree(memory_manager->costs_for_read_access);
  tfree(memory_manager->costs_for_write_access);
  tfree(memory_manager->memtotal);
  tfree(memory_manager->memfree);

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
                                void **pageaddrs, int nbpages, size_t size, int node, int *nodes,
                                int protection, int with_huge_pages, int mami_allocated,
                                mami_data_t **memory_data) {
  MEMORY_ILOG_IN();

  *memory_data = tmalloc(sizeof(mami_data_t));

  // Set the interval addresses and the length
  (*memory_data)->startaddress = pageaddrs[0];
  (*memory_data)->mprotect_startaddress = pageaddrs[0];
  (*memory_data)->endaddress = pageaddrs[0]+size;
  (*memory_data)->size = size;
  (*memory_data)->mprotect_size = size;
  (*memory_data)->status = MAMI_INITIAL_STATUS;
  (*memory_data)->protection = protection;
  (*memory_data)->with_huge_pages = with_huge_pages;
  (*memory_data)->mami_allocated = mami_allocated;
  (*memory_data)->owners = tbx_slist_nil();
  (*memory_data)->next = NULL;
  (*memory_data)->node = node;

  if (node != MAMI_MULTIPLE_LOCATION_NODE) (*memory_data)->nodes = NULL;
  else {
    (*memory_data)->nodes = nodes;
  }

  // Set the page addresses
  (*memory_data)->nbpages = nbpages;
  (*memory_data)->pageaddrs = tmalloc((*memory_data)->nbpages * sizeof(void *));
  memcpy((*memory_data)->pageaddrs, pageaddrs, nbpages*sizeof(void*));

  MEMORY_ILOG_OUT();
}

static
void _mami_clean_memory_data(mami_data_t **memory_data) {
  mdebug_memory("Cleaning memory area %p\n", (*memory_data)->startaddress);
  if (!(tbx_slist_is_nil((*memory_data)->owners))) {
    marcel_fprintf(stderr, "MaMI Warning: some threads are still attached to the memory area [%p:%p]\n",
                   (*memory_data)->startaddress, (*memory_data)->endaddress);
    tbx_slist_clear((*memory_data)->owners);
  }
  tbx_slist_free((*memory_data)->owners);
  if ((*memory_data)->nodes != NULL) tfree((*memory_data)->nodes);
  tfree((*memory_data)->pageaddrs);
  tfree(*memory_data);
}

void _mami_delete_tree(mami_manager_t *memory_manager, mami_tree_t **memory_tree) {
  MEMORY_ILOG_IN();
  if ((*memory_tree)->leftchild == NULL) {
    mami_tree_t *temp = (*memory_tree);
    _mami_clean_memory_data(&(temp->data));
    (*memory_tree) = (*memory_tree)->rightchild;
    tfree(temp);
  }
  else if ((*memory_tree)->rightchild == NULL) {
    mami_tree_t *temp = *memory_tree;
    _mami_clean_memory_data(&(temp->data));
    (*memory_tree) = (*memory_tree)->leftchild;
    tfree(temp);
  }
  else {
    // In-order predecessor (rightmost child of left subtree)
    // Node has two children - get max of left subtree
    mami_tree_t *temp = (*memory_tree)->leftchild; // get left node of the original node

    // find the rightmost child of the subtree of the left node
    while (temp->rightchild != NULL) {
      temp = temp->rightchild;
    }

    // copy the value from the in-order predecessor to the original node
    _mami_clean_memory_data(&((*memory_tree)->data));
    _mami_init_memory_data(memory_manager, temp->data->pageaddrs, temp->data->nbpages, temp->data->size, temp->data->node,
                               temp->data->nodes, temp->data->protection, temp->data->with_huge_pages, temp->data->mami_allocated,
                               &((*memory_tree)->data));

    // then delete the predecessor
    _mami_unregister(memory_manager, &((*memory_tree)->leftchild), temp->data->pageaddrs[0]);
  }
  MEMORY_ILOG_OUT();
}

static
void _mami_free_from_node(mami_manager_t *memory_manager, void *buffer, size_t size, int nbpages, int node, int protection, int with_huge_pages) {
  mami_area_t *available;
  mami_area_t **ptr;
  mami_area_t **prev;
  mami_area_t *next;

  MEMORY_ILOG_IN();

  mdebug_memory("Freeing space [%p:%p] with %d pages\n", buffer, buffer+size, nbpages);

  available = tmalloc(sizeof(mami_area_t));
  available->start = buffer;
  available->end = buffer + size;
  available->nbpages = nbpages;
  if (with_huge_pages) {
    available->pagesize = memory_manager->hugepagesize;
  }
  else {
    available->pagesize = memory_manager->normalpagesize;
  }
  available->protection = protection;
  available->next = NULL;

  // Insert the new area at the right place
  if (with_huge_pages) {
    ptr = &(memory_manager->huge_pages_heaps[node]->heap);
  }
  else {
    ptr = &(memory_manager->heaps[node]);
    if (node >= 0 && node < memory_manager->nb_nodes) {
      memory_manager->memfree[node] += (size/1024);
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
  if (with_huge_pages) {
    ptr = &(memory_manager->huge_pages_heaps[node]->heap);
  }
  else {
    ptr = &(memory_manager->heaps[node]);
  }
  while ((*ptr)->next != NULL) {
    if (((*ptr)->end == (*ptr)->next->start) && ((*ptr)->protection == (*ptr)->next->protection)) {
      mdebug_memory("[%p:%p:%d] and [%p:%p:%d] can be defragmented\n", (*ptr)->start, (*ptr)->end, (*ptr)->protection,
                  (*ptr)->next->start, (*ptr)->next->end, (*ptr)->next->protection);
      (*ptr)->end = (*ptr)->next->end;
      (*ptr)->nbpages += (*ptr)->next->nbpages;
      next = (*ptr)->next;
      (*ptr)->next = (*ptr)->next->next;
      tfree(next);
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
          _mami_free_from_node(memory_manager, buffer, data->size, data->nbpages, data->node, data->protection, data->with_huge_pages);
      }
      else {
	mdebug_memory("Address %p is not allocated by MaMI.\n", buffer);
      }

      // Delete tree
      _mami_delete_tree(memory_manager, memory_tree);

      if (next_data) {
        mdebug_memory("Need to unregister the next memory area\n");
        _mami_unregister(memory_manager, &(memory_manager->root), next_data->startaddress);
      }
    }
    else if (buffer < (*memory_tree)->data->pageaddrs[0])
      _mami_unregister(memory_manager, &((*memory_tree)->leftchild), buffer);
    else
      _mami_unregister(memory_manager, &((*memory_tree)->rightchild), buffer);
  }
  MEMORY_ILOG_OUT();
}

static
void _mami_register_pages(mami_manager_t *memory_manager, mami_tree_t **memory_tree,
                              void **pageaddrs, int nbpages, size_t size, int node, int *nodes,
                              int protection, int with_huge_pages, int mami_allocated,
                              mami_data_t **data) {
  MEMORY_ILOG_IN();
  if (*memory_tree==NULL) {
    *memory_tree = tmalloc(sizeof(mami_tree_t));
    (*memory_tree)->leftchild = NULL;
    (*memory_tree)->rightchild = NULL;
    _mami_init_memory_data(memory_manager, pageaddrs, nbpages, size, node, nodes,
                               protection, with_huge_pages, mami_allocated, &((*memory_tree)->data));
    mdebug_memory("Adding data %p into tree\n", (*memory_tree)->data);
    if (data) *data = (*memory_tree)->data;
  }
  else {
    if (pageaddrs[0] < (*memory_tree)->data->pageaddrs[0])
      _mami_register_pages(memory_manager, &((*memory_tree)->leftchild), pageaddrs, nbpages, size, node, nodes,
                               protection, with_huge_pages, mami_allocated, data);
    else
      _mami_register_pages(memory_manager, &((*memory_tree)->rightchild), pageaddrs, nbpages, size, node, nodes,
                               protection, with_huge_pages, mami_allocated, data);
  }
  MEMORY_ILOG_OUT();
}

int _mami_preallocate_huge_pages(mami_manager_t *memory_manager, mami_huge_pages_area_t **space, int node) {
  int nbpages;
  pid_t pid;

  MEMORY_ILOG_IN();
  nbpages = marcel_topo_node_level ? marcel_topo_node_level[node].huge_page_free : 0;
  if (!nbpages) {
    mdebug_memory("No huge pages on node #%d\n", node);
    *space = NULL;
    return -1;
  }

  (*space) = tmalloc(sizeof(mami_huge_pages_area_t));
  (*space)->size = nbpages * memory_manager->hugepagesize;
  (*space)->filename = tmalloc(1024 * sizeof(char));
  pid = getpid();
  marcel_sprintf((*space)->filename, "/hugetlbfs/mami_pid_%d_node_%d", pid, node);

  (*space)->file = open((*space)->filename, O_CREAT|O_RDWR, S_IRWXU);
  if ((*space)->file == -1) {
    tfree(*space);
    *space = NULL;
    perror("(_mami_preallocate_huge_pages) open");
    return -errno;
  }

  (*space)->buffer = mmap(NULL, (*space)->size, PROT_READ|PROT_WRITE, MAP_PRIVATE, (*space)->file, 0);
  if ((*space)->buffer == MAP_FAILED) {
    perror("(_mami_preallocate_huge_pages) mmap");
    tfree(*space);
    *space = NULL;
    return -errno;
  }
#warning todo: readv pour prefaulter
  /* mark the memory as unaccessible until it gets allocated to the application */
  VALGRIND_MAKE_MEM_NOACCESS((*space)->buffer, (*space)->size);

  (*space)->heap = tmalloc(sizeof(mami_area_t));
  (*space)->heap->start = (*space)->buffer;
  (*space)->heap->end = (*space)->buffer + (*space)->size;
  (*space)->heap->nbpages = nbpages;
  (*space)->heap->protection = PROT_READ|PROT_WRITE;
  (*space)->heap->pagesize = memory_manager->hugepagesize;
  (*space)->heap->next = NULL;

  MEMORY_ILOG_OUT();
  return 0;
}

int _mami_preallocate(mami_manager_t *memory_manager, mami_area_t **space, int nbpages, int vnode, int pnode) {
  unsigned long nodemask;
  size_t length;
  void *buffer;
  int i, err=0;

  MEMORY_ILOG_IN();

  if (vnode != MAMI_FIRST_TOUCH_NODE && memory_manager->memtotal[vnode] == 0) {
    mdebug_memory("No memory available on node #%d, os_node #%d\n", vnode, pnode);
    err = -EINVAL;
    buffer = NULL;
    length = 0;
  }
  else {
    length = nbpages * memory_manager->normalpagesize;
    buffer = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED) {
      perror("(_mami_preallocate) mmap");
      err = -errno;
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

        for(i=0 ; i<nbpages ; i++) {
          int *ptr = buffer+i*memory_manager->normalpagesize;
          *ptr = 0;
        }
      }
      /* mark the memory as unaccessible until it gets allocated to the application */
      VALGRIND_MAKE_MEM_NOACCESS(buffer, length);
    }
  }

  (*space) = tmalloc(sizeof(mami_area_t));
  (*space)->start = buffer;
  (*space)->end = buffer + length;
  (*space)->nbpages = nbpages;
  (*space)->protection = PROT_READ|PROT_WRITE;
  (*space)->pagesize = memory_manager->normalpagesize;
  (*space)->next = NULL;

  mdebug_memory("Preallocating [%p:%p] on node #%d\n", buffer,buffer+length, pnode);
  MEMORY_ILOG_OUT();
  return err;
}

static
void* _mami_get_buffer_from_huge_pages_heap(mami_manager_t *memory_manager, int node, int nbpages, size_t size, int *protection) {
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
    mdebug_memory("Current space from %p with %d pages\n", hheap->start, hheap->nbpages);
    if (hheap->nbpages >= nbpages)
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
  if (nbpages == hheap->nbpages) {
    if (prev == hheap)
      heap->heap = prev->next;
    else
      prev->next = hheap->next;
  }
  else {
    hheap->start += size;
    hheap->nbpages -= nbpages;
  }

  nodemask = (1<<node);
  err = _mami_set_mempolicy(MPOL_BIND, &nodemask, memory_manager->nb_nodes+2);
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
void* _mami_get_buffer_from_heap(mami_manager_t *memory_manager, int node, int nbpages, size_t size, int *protection) {
  mami_area_t *heap = memory_manager->heaps[node];
  mami_area_t *prev = NULL;
  void *buffer;

  mdebug_memory("Requiring space of %d pages from heap %p on node #%d\n", nbpages, heap, node);

  // Look for a space big enough
  prev = heap;
  while (heap != NULL && heap->start != NULL) {
    mdebug_memory("Current space from %p with %d pages\n", heap->start, heap->nbpages);
    if (heap->nbpages >= nbpages)
      break;
    prev = heap;
    heap = heap->next;
  }
  if (heap == NULL || heap->start == NULL) {
    int err, preallocatedpages, osnode;

    preallocatedpages = nbpages;
    if (preallocatedpages < memory_manager->initially_preallocated_pages) {
      preallocatedpages = memory_manager->initially_preallocated_pages;
    }
    osnode = marcel_topo_node_level ? marcel_topo_node_level[node].os_node : 0;
    mdebug_memory("not enough space, let's allocate %d extra pages on the OS node #%d\n", preallocatedpages, osnode);
    err = _mami_preallocate(memory_manager, &heap, preallocatedpages, node, osnode);
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
  if (nbpages == heap->nbpages) {
    if (prev == heap)
      memory_manager->heaps[node] = prev->next;
    else
      prev->next = heap->next;
  }
  else {
    heap->start += size;
    heap->nbpages -= nbpages;
  }
  if (node >= 0 && node < memory_manager->nb_nodes) {
    memory_manager->memfree[node] -= (size/1024);
  }
  return buffer;
}

static
void* _mami_malloc(mami_manager_t *memory_manager, size_t size, unsigned long pagesize, int node, int with_huge_pages) {
  void *buffer;
  int i, nbpages, protection=0;
  size_t realsize;
  void **pageaddrs;

  MEMORY_ILOG_IN();

  if (!size) {
    MEMORY_ILOG_OUT();
    return NULL;
  }

  marcel_mutex_lock(&(memory_manager->lock));

  // Round-up the size
  nbpages = size / pagesize;
  if (nbpages * pagesize != size) nbpages++;
  realsize = nbpages * pagesize;

  if (!with_huge_pages) {
    buffer = _mami_get_buffer_from_heap(memory_manager, node, nbpages, realsize, &protection);
  }
  else {
    buffer = _mami_get_buffer_from_huge_pages_heap(memory_manager, node, nbpages, realsize, &protection);
  }

  if (!buffer) {
    errno = ENOMEM;
  }
  else {
    // Set the page addresses
    pageaddrs = tmalloc(nbpages * sizeof(void *));
    for(i=0; i<nbpages ; i++) pageaddrs[i] = buffer + i*pagesize;

    // Register memory
    mdebug_memory("Registering [%p:%p:%ld]\n", pageaddrs[0], pageaddrs[0]+size, (long)size);
    _mami_register_pages(memory_manager, &(memory_manager->root), pageaddrs, nbpages, realsize, node, NULL,
                             protection, with_huge_pages, 1, NULL);

    tfree(pageaddrs);
    VALGRIND_MAKE_MEM_UNDEFINED(buffer, size);
  }

  marcel_mutex_unlock(&(memory_manager->lock));
  mdebug_memory("Allocating %p on node #%d\n", buffer, node);
  MEMORY_ILOG_OUT();
  return buffer;
}

void* mami_malloc(mami_manager_t *memory_manager, size_t size, mami_membind_policy_t policy, int node) {
  void *ptr;
  int with_huge_pages;
  unsigned long pagesize;

  MEMORY_LOG_IN();

  pagesize = memory_manager->normalpagesize;
  with_huge_pages = 0;
  if (policy == MAMI_MEMBIND_POLICY_DEFAULT) {
    policy = memory_manager->membind_policy;
    node = memory_manager->membind_node;
  }

  if (policy == MAMI_MEMBIND_POLICY_NONE) {
    node = marcel_current_node();
    if (tbx_unlikely(node == -1)) node=0;
  }
  else if (policy == MAMI_MEMBIND_POLICY_LEAST_LOADED_NODE) {
    mami_select_node(memory_manager, MAMI_LEAST_LOADED_NODE, &node);
  }
  else if (policy == MAMI_MEMBIND_POLICY_HUGE_PAGES) {
    pagesize = memory_manager->hugepagesize;
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

  ptr = _mami_malloc(memory_manager, size, pagesize, node, with_huge_pages);

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
    return -errno;
  }
  //mdebug_memory("Comparing [%p:%p] and [%p:%p]\n", buffer,buffer+size, memory_tree->data->startaddress, memory_tree->data->endaddress);
  if (buffer >= memory_tree->data->startaddress && buffer+size <= memory_tree->data->endaddress) {
    // the address is stored on the current memory_data
    mdebug_memory("Found interval [%p:%p] in [%p:%p]\n", buffer, buffer+size, memory_tree->data->startaddress, memory_tree->data->endaddress);
    mdebug_memory("Interval [%p:%p] is located on node #%d\n", buffer, buffer+size, memory_tree->data->node);
    *data = memory_tree->data;
    return 0;
  }
  else if (buffer <= memory_tree->data->startaddress) {
    return _mami_locate(memory_manager, memory_tree->leftchild, buffer, size, data);
  }
  else if (buffer >= memory_tree->data->endaddress) {
    return _mami_locate(memory_manager, memory_tree->rightchild, buffer, size, data);
  }
  else {
    errno = EINVAL;
    return -errno;
  }
}

int _mami_get_pages_location(mami_manager_t *memory_manager, void **pageaddrs, int nbpages, int *node, int **nodes) {
  int *statuses;
  int i, err=0;

  statuses = tmalloc(nbpages * sizeof(int));
  err = _mm_move_pages(pageaddrs, nbpages, NULL, statuses, 0);
  if (err < 0 || statuses[0] == -ENOENT) {
    mdebug_memory("Could not locate pages\n");
    if (*node != MAMI_FIRST_TOUCH_NODE) {
      *node = MAMI_UNKNOWN_LOCATION_NODE;
    }
    errno = ENOENT;
    err = -errno;
  }
  else {
    *node = statuses[0];
    for(i=1 ; i<nbpages ; i++) {
      if (statuses[i] != statuses[0]) {
#ifdef PM2DEBUG
	marcel_fprintf(stderr, "#MaMI Warning: Memory located on different nodes\n");
#endif
        if (*node != MAMI_FIRST_TOUCH_NODE) {
          *node = MAMI_MULTIPLE_LOCATION_NODE;
          *nodes = tmalloc(nbpages * sizeof(int));
          memcpy(*nodes, statuses, nbpages*sizeof(int));
        }
        break;
      }
    }
  }
  tfree(statuses);
  return err;
}

void _mami_register(mami_manager_t *memory_manager,
                        void *buffer,
                        size_t size,
                        int mami_allocated,
                        mami_data_t **data) {
  void **pageaddrs;
  int nbpages, protection, with_huge_pages;
  int i, node, *nodes;

  mdebug_memory("Registering address interval [%p:%p:%lu]\n", buffer, buffer+size, (long)size);

  with_huge_pages = 0;
  protection = PROT_READ|PROT_WRITE;

  // Count the number of pages
  nbpages = size / memory_manager->normalpagesize;
  if (nbpages * memory_manager->normalpagesize != size) nbpages++;

  // Set the page addresses
  pageaddrs = tmalloc(nbpages * sizeof(void *));
  for(i=0; i<nbpages ; i++) pageaddrs[i] = buffer + i*memory_manager->normalpagesize;

  // Find out where the pages are
  node=0;
  _mami_get_pages_location(memory_manager, pageaddrs, nbpages, &node, &nodes);

  // Register the pages
  _mami_register_pages(memory_manager, &(memory_manager->root), pageaddrs, nbpages, size, node, nodes,
                       protection, with_huge_pages, mami_allocated, data);

  // Free temporary array
  tfree(pageaddrs);
}

int mami_register(mami_manager_t *memory_manager,
                  void *buffer,
                  size_t size) {
  void *aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
  void *aligned_endbuffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer+size, memory_manager->normalpagesize);
  size_t aligned_size = aligned_endbuffer-aligned_buffer;
  mami_data_t *data = NULL;

  MEMORY_LOG_IN();
  if (aligned_size > size) aligned_size = size;
  marcel_mutex_lock(&(memory_manager->lock));
  mdebug_memory("Registering [%p:%p:%ld]\n", aligned_buffer, aligned_buffer+aligned_size, (long)aligned_size);
  _mami_register(memory_manager, aligned_buffer, aligned_size, 0, &data);

  if (aligned_endbuffer > buffer+size) {
    data->mprotect_size = data->size - memory_manager->normalpagesize;
  }
  if (aligned_buffer < buffer) {
    data->mprotect_startaddress += memory_manager->normalpagesize;
    data->mprotect_size -= memory_manager->normalpagesize;
  }

  marcel_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
  return 0;
}

int mami_unregister(mami_manager_t *memory_manager,
                    void *buffer) {
  mami_data_t *data = NULL;
  int err=0;
  void *aligned_buffer;

  MEMORY_LOG_IN();
  marcel_mutex_lock(&(memory_manager->lock));

  aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
  err = _mami_locate(memory_manager, memory_manager->root, aligned_buffer, 1, &data);
  if (err >= 0) {
    mdebug_memory("Unregistering [%p:%p]\n", buffer,buffer+data->size);
    _mami_unregister(memory_manager, &(memory_manager->root), aligned_buffer);
  }
  marcel_mutex_unlock(&(memory_manager->lock));
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
  marcel_mutex_lock(&(memory_manager->lock));

  err = _mami_locate(memory_manager, memory_manager->root, buffer, 1, &data);
  if (err >= 0) {
    size_t subsize;
    int subpages, i;
    void **pageaddrs;

    subpages = data->nbpages / subareas;
    if (!subpages) {
      mdebug_memory("Memory area from %p with size %ld and %d pages not big enough to be split in %d subareas\n",
                  data->startaddress, (long) data->size, data->nbpages, subareas);
      errno = EINVAL;
      err = -errno;
    }
    else {
      mdebug_memory("Splitting area from %p in %d areas of %d pages\n", data->startaddress, subareas, subpages);

      // Register new subareas
      pageaddrs = data->pageaddrs+subpages;
      if (data->with_huge_pages)
	subsize = subpages * memory_manager->hugepagesize;
      else
	subsize = subpages * memory_manager->normalpagesize;
      for(i=1 ; i<subareas ; i++) {
	newbuffers[i] = pageaddrs[0];
	_mami_register_pages(memory_manager, &(memory_manager->root), pageaddrs, subpages, subsize, data->node, data->nodes,
                             data->protection, data->with_huge_pages, data->mami_allocated, NULL);
	pageaddrs += subpages;
      }

      // Modify initial memory area
      newbuffers[0] = buffer;
      data->nbpages = subpages;
      data->endaddress = data->startaddress + subsize;
      data->size = subsize;
    }
  }
  marcel_mutex_unlock(&(memory_manager->lock));
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
    err = -errno;
  }
  else if (policy == MAMI_MEMBIND_POLICY_HUGE_PAGES && memory_manager->hugepagesize == 0) {
    mdebug_memory("Huge pages are not available. Cannot set memory policy.\n");
    errno = EINVAL;
    err = -errno;
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
  aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
  mdebug_memory("Freeing [%p]\n", buffer);
  marcel_mutex_lock(&(memory_manager->lock));
  _mami_unregister(memory_manager, &(memory_manager->root), aligned_buffer);
  marcel_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
}

int _mami_set_mempolicy(int mode, const unsigned long *nmask, unsigned long maxnode) {
  int err=0;

  MEMORY_ILOG_IN();
  if (marcel_use_synthetic_topology) {
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

  if (marcel_use_synthetic_topology) {
    mdebug_memory("Using synthethic topology. No need to check physically\n");
    return err;
  }

  pagenodes = tmalloc(pages * sizeof(int));
  err = _mm_move_pages(pageaddrs, pages, NULL, pagenodes, 0);
  if (err < 0) perror("(_mami_check_pages_location) move_pages");
  else {
    for(i=0; i<pages; i++) {
      if (pagenodes[i] != node) {
        marcel_fprintf(stderr, "MaMI Warning: page #%d is not located on node #%d but on node #%d\n", i, node, pagenodes[i]);
        err = -EINVAL;
      }
    }
  }
  tfree(pagenodes);
  MEMORY_ILOG_OUT();
  return err;
}

int mami_locate(mami_manager_t *memory_manager, void *buffer, size_t size, int *node) {
  mami_data_t *data = NULL;
  void *aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
  void *aligned_endbuffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer+size, memory_manager->normalpagesize);
  size_t aligned_size = aligned_endbuffer-aligned_buffer;
  int err, *nodes;

  MEMORY_LOG_IN();
  if (aligned_size > size) aligned_size = size;
  mdebug_memory("Trying to locate [%p:%p:%ld]\n", aligned_buffer, aligned_buffer+aligned_size, (long)aligned_size);
  err = _mami_locate(memory_manager, memory_manager->root, aligned_buffer, aligned_size, &data);
  if (err >= 0) {
    if (data->status == MAMI_KERNEL_MIGRATION_STATUS) {
      int node;
      _mami_get_pages_location(memory_manager, data->pageaddrs, data->nbpages, &node, &nodes);
      if (node != data->node) {
        mdebug_memory("Updating location of the pages after a kernel migration\n");
        data->node = node;
        data->status = MAMI_INITIAL_STATUS;
      }
    }
    *node = data->node;
  }
  MEMORY_LOG_OUT();
  return err;
}

int mami_check_pages_location(mami_manager_t *memory_manager, void *buffer, size_t size, int node) {
  mami_data_t *data = NULL;
  void *aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
  void *aligned_endbuffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer+size, memory_manager->normalpagesize);
  size_t aligned_size = aligned_endbuffer-aligned_buffer;
  int err=0;

  MEMORY_LOG_IN();
  if (aligned_size > size) aligned_size = size;
  err = _mami_locate(memory_manager, memory_manager->root, aligned_buffer, aligned_size, &data);
  if (err >= 0) {
    err = _mami_check_pages_location(data->pageaddrs, data->nbpages, node);
    if (err < 0) {
      marcel_fprintf(stderr, "MaMI: The %d pages are NOT all on node #%d\n", data->nbpages, node);
    }
    else {
      marcel_fprintf(stderr, "MaMI: The %d pages are all on node #%d\n", data->nbpages, node);
    }
  }
  MEMORY_LOG_OUT();
  return err;
}

int mami_update_pages_location(mami_manager_t *memory_manager,
                               void *buffer,
                               size_t size) {
  mami_data_t *data = NULL;
  void *aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
  void *aligned_endbuffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer+size, memory_manager->normalpagesize);
  size_t aligned_size = aligned_endbuffer-aligned_buffer;
  int err, node, *nodes;

  MEMORY_LOG_IN();
  if (aligned_size > size) aligned_size = size;
  err = _mami_locate(memory_manager, memory_manager->root, aligned_buffer, aligned_size, &data);
  if (err >= 0) {
    _mami_get_pages_location(memory_manager, data->pageaddrs, data->nbpages, &node, &nodes);
    if (node != data->node) {
      mdebug_memory("Updating pages location from node #%d to node #%d\n", data->node, node);
      data->node = node;
    }
  }
  MEMORY_LOG_OUT();
  return err;
}

static
void _mami_print(mami_tree_t *memory_tree, FILE *stream, int indent) {
  if (memory_tree) {
    int x;
    _mami_print(memory_tree->leftchild, stream, indent+2);
    for(x=0 ; x<indent ; x++) marcel_fprintf(stream, " ");
    marcel_fprintf(stream, "[%p, %p]\n", memory_tree->data->startaddress, memory_tree->data->endaddress);
    _mami_print(memory_tree->rightchild, stream, indent+2);
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
    err = -errno;
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
  int err;

  MEMORY_LOG_IN();
  if (tbx_unlikely(source >= memory_manager->nb_nodes || dest >= memory_manager->nb_nodes)) {
    mdebug_memory("Invalid node id\n");
    errno = EINVAL;
    err = -errno;
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
  int err;

  MEMORY_LOG_IN();
  if (tbx_unlikely(source >= memory_manager->nb_nodes || dest >= memory_manager->nb_nodes)) {
    mdebug_memory("Invalid node id\n");
    errno = EINVAL;
    err = -errno;
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
  marcel_mutex_lock(&(memory_manager->lock));

  if (policy == MAMI_LEAST_LOADED_NODE) {
    int i;
    unsigned long maxspace=memory_manager->memfree[0];
    mdebug_memory("Space on node %d = %ld\n", 0, maxspace);
    *node = 0;
    for(i=1 ; i<memory_manager->nb_nodes ; i++) {
      mdebug_memory("Space on node %d = %ld\n", i, memory_manager->memfree[i]);
      if (memory_manager->memfree[i] > maxspace) {
        maxspace = memory_manager->memfree[i];
        *node = i;
      }
    }
  }
  else {
    mdebug_memory("Policy #%d unknown\n", policy);
    errno = EINVAL;
    err = -errno;
  }

  marcel_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
  return err;
}

int _mami_migrate_pages(mami_manager_t *memory_manager,
                        mami_data_t *data, int dest) {
  int err=0;

  MEMORY_ILOG_IN();
  if (data->node == dest) {
    mdebug_memory("The address %p is already located at the required node.\n", data->startaddress);
    err = EALREADY;
  }
  else {
    if (data->node == MAMI_FIRST_TOUCH_NODE || data->node == MAMI_UNKNOWN_LOCATION_NODE) {
      unsigned long nodemask;

      mdebug_memory("Mbinding %d page(s) to node #%d\n", data->nbpages, dest);
      nodemask = (1<<dest);
      err = _mm_mbind(data->startaddress, data->size, MPOL_BIND, &nodemask, memory_manager->nb_nodes+2, MPOL_MF_MOVE|MPOL_MF_STRICT);
    }
    else {
      int i, dests[data->nbpages], status[data->nbpages];

      mdebug_memory("Migrating %d page(s) to node #%d\n", data->nbpages, dest);
      for(i=0 ; i<data->nbpages ; i++) dests[i] = dest;

      if (data->node != MAMI_MULTIPLE_LOCATION_NODE) {
        err = _mm_move_pages(data->pageaddrs, data->nbpages, dests, status, MPOL_MF_MOVE);
      }
      else {
        void *pageaddrs_to_be_moved[data->nbpages];
        int nbpages_to_be_moved=0;
        unsigned long nodemask;

        nodemask = (1<<dest);
        mdebug_memory("Pages on different locations. Check which ones need to be moved and which ones need to be bound\n");
        // Some pages might already be at the right location */
        for(i=0 ; i<data->nbpages ; i++) {
          if (data->nodes[i] == -ENOENT) {
            mdebug_memory("Mbinding page %d (%p) to node #%d\n", i, data->pageaddrs[i], dest);
            err = _mm_mbind(data->pageaddrs[i], memory_manager->normalpagesize, MPOL_BIND, &nodemask,
                            memory_manager->nb_nodes+2, MPOL_MF_MOVE|MPOL_MF_STRICT);
          }
          else if (data->nodes[i] != dest) {
            pageaddrs_to_be_moved[nbpages_to_be_moved] = data->pageaddrs[i];
            nbpages_to_be_moved ++;
          }
        }
        mdebug_memory("%d page(s) need to be moved\n", nbpages_to_be_moved);
        if (nbpages_to_be_moved) {
          err = _mm_move_pages(pageaddrs_to_be_moved, nbpages_to_be_moved, dests, status, MPOL_MF_MOVE);
        }
      }
    }

    if (err < 0) {
      mdebug_memory("Error when mbinding or migrating: %d\n", err);
    }
    else {
      if (!tbx_slist_is_nil(data->owners)) {
        tbx_slist_ref_to_head(data->owners);
        do {
          marcel_entity_t *object = NULL;
          object = tbx_slist_ref_get(data->owners);
          _mm_mami_update_stats(object, data->node, dest, data->size);
        } while (tbx_slist_ref_forward(data->owners));
      }
      data->node = dest;
    }
  }

  MEMORY_ILOG_OUT();
  errno = err;
  return -err;
}

int mami_migrate_pages(mami_manager_t *memory_manager,
                       void *buffer, int dest) {
  int ret;
  mami_data_t *data = NULL;

  MEMORY_LOG_IN();
  marcel_mutex_lock(&(memory_manager->lock));
  ret = _mami_locate(memory_manager, memory_manager->root, buffer, 1, &data);
  if (ret >= 0) {
    ret = _mami_migrate_pages(memory_manager, data, dest);
  }
  marcel_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
  return ret;
}

static
void _mami_segv_handler(int sig, siginfo_t *info, void *_context) {
  void *addr;
  int err, dest;
  mami_data_t *data = NULL;

  marcel_mutex_lock(&(g_memory_manager->lock));

#warning look at www.gnu.org/software/libsigsegv/

  addr = info->si_addr;
  err = _mami_locate(g_memory_manager, g_memory_manager->root, addr, 1, &data);
  if (err < 0) {
    // The address is not managed by MaMI. Reset the segv handler to its default action, to cause a segfault
    struct sigaction act;
    act.sa_flags = SA_SIGINFO;
    act.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &act, NULL);
  }
  if (data && data->status != MAMI_NEXT_TOUCHED_STATUS) {
    data->status = MAMI_NEXT_TOUCHED_STATUS;
    dest = marcel_current_node();
    _mami_migrate_pages(g_memory_manager, data, dest);
    err = mprotect(data->mprotect_startaddress, data->mprotect_size, data->protection);
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
  marcel_mutex_unlock(&(g_memory_manager->lock));
}

int mami_migrate_on_next_touch(mami_manager_t *memory_manager, void *buffer) {
  int err=0;
  mami_data_t *data = NULL;
  void *aligned_buffer;

  MEMORY_LOG_IN();
  marcel_mutex_lock(&(memory_manager->lock));

  g_memory_manager = memory_manager;
  aligned_buffer = MAMI_ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
  mdebug_memory("Trying to locate [%p:%p:1]\n", aligned_buffer, aligned_buffer+1);
  err = _mami_locate(memory_manager, memory_manager->root, aligned_buffer, 1, &data);
  if (err >= 0) {
    mdebug_memory("Setting migrate on next touch on address %p (%p)\n", data->startaddress, buffer);
    if (memory_manager->kernel_nexttouch_migration == 1 && data->mami_allocated) {
      mdebug_memory("... using in-kernel migration\n");
      data->status = MAMI_KERNEL_MIGRATION_STATUS;
      err = _mm_mbind(data->startaddress, data->size, MPOL_DEFAULT, NULL, 0, 0);
      if (err < 0) {
        perror("(mami_migrate_on_next_touch) mbind");
      }
      else {
        err = madvise(data->startaddress, data->size, 12);
        if (err < 0) {
          perror("(mami_migrate_on_next_touch) madvise");
        }
      }
    }
    else {
      mdebug_memory("... using user-space migration\n");
      if (!memory_manager_sigsegv_handler_set) {
        struct sigaction act;
        memory_manager_sigsegv_handler_set = 1;
        act.sa_flags = SA_SIGINFO;
        act.sa_sigaction = _mami_segv_handler;
        err = sigaction(SIGSEGV, &act, NULL);
        if (err < 0) {
          perror("(mami_migrate_on_next_touch) sigaction");
          marcel_mutex_unlock(&(memory_manager->lock));
          MEMORY_LOG_OUT();
          return -errno;
        }
      }
      mdebug_memory("Mprotecting [%p:%p:%ld] (initially [%p:%p:%ld]\n", data->mprotect_startaddress, data->startaddress+data->mprotect_size,
		    (long) data->mprotect_size, data->startaddress, data->endaddress, (long) data->size);
      data->status = MAMI_INITIAL_STATUS;
      err = mprotect(data->mprotect_startaddress, data->mprotect_size, PROT_NONE);
      if (err < 0) {
        perror("(mami_migrate_on_next_touch) mprotect");
      }
    }
  }
  marcel_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
  return err;
}

int mami_migrate_on_node(mami_manager_t *memory_manager,
                         void *buffer,
                         int node) {
  mami_data_t *data = NULL;
  int err;

  MEMORY_LOG_IN();
  marcel_mutex_lock(&(memory_manager->lock));

  err = _mami_locate(memory_manager, memory_manager->root, buffer, 1, &data);
  if (err >= 0) {
    err = _mami_migrate_pages(memory_manager, data, node);
  }
  marcel_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
  return err;
}

int mami_huge_pages_available(mami_manager_t *memory_manager) {
  return (memory_manager->hugepagesize != 0);
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
    err = -errno;
  }
  else {
    marcel_mutex_lock(&(memory_manager->lock));
    if (stat == MAMI_STAT_MEMORY_TOTAL) {
      *value = memory_manager->memtotal[node];
    }
    else if (stat == MAMI_STAT_MEMORY_FREE) {
      *value = memory_manager->memfree[node];
    }
    else {
      mdebug_memory("Statistic #%d unknown\n", stat);
      errno = EINVAL;
      err = -errno;
    }
    marcel_mutex_unlock(&(memory_manager->lock));
  }

  MEMORY_LOG_OUT();
  return err;
}

int mami_distribute(mami_manager_t *memory_manager,
                    void *buffer,
                    int *nodes,
                    int nb_nodes) {
  mami_data_t *data;
  int err, i, nbpages;

  MEMORY_LOG_IN();

  marcel_mutex_lock(&(memory_manager->lock));
  err = _mami_locate(memory_manager, memory_manager->root, buffer, 1, &data);

  if (err >= 0) {
    void **pageaddrs;
    int *dests, *status;

    dests = tmalloc(data->nbpages * sizeof(int));
    status = tmalloc(data->nbpages * sizeof(int));
    pageaddrs = tmalloc(data->nbpages * sizeof(void *));
    nbpages=0;
    // Find out which pages have to be moved
    for(i=0 ; i<data->nbpages ; i++) {
      if ((data->node == MAMI_MULTIPLE_LOCATION_NODE && data->nodes[i] != nodes[i%nb_nodes]) ||
          (data->node != MAMI_MULTIPLE_LOCATION_NODE && data->node != nodes[i%nb_nodes])) {
        mdebug_memory("Moving page %d from node #%d to node #%d\n", i, data->node != MAMI_MULTIPLE_LOCATION_NODE ? data->node : data->nodes[i], nodes[i%nb_nodes]);
        pageaddrs[nbpages] = buffer + (i*memory_manager->normalpagesize);
        dests[nbpages] = nodes[i%nb_nodes];
        nbpages ++;
      }
    }

    // If there is some pages to be moved, move them
    if (nbpages != 0) {
      err = _mm_move_pages(pageaddrs, nbpages, dests, status, MPOL_MF_MOVE);

      if (err >= 0) {
        // Check the pages have been properly moved and update the data->nodes information
        if (data->nodes == NULL) data->nodes = tmalloc(data->nbpages * sizeof(int));
        data->node = MAMI_MULTIPLE_LOCATION_NODE;
        err = _mm_move_pages(data->pageaddrs, data->nbpages, NULL, status, 0);
        for(i=0 ; i<data->nbpages ; i++) {
          if (status[i] != nodes[i%nb_nodes]) {
            marcel_fprintf(stderr, "MaMI Warning: Page %d is on node %d, but it should be on node %d\n", i, status[i], nodes[i%nb_nodes]);
          }
          data->nodes[i] = status[i];
        }
      }
    }
    tfree(dests);
    tfree(status);
    tfree(pageaddrs);
  }

  marcel_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
  return err;
}

int mami_gather(mami_manager_t *memory_manager,
                void *buffer,
                int node) {
  mami_data_t *data;
  int err;

  MEMORY_LOG_IN();

  marcel_mutex_lock(&(memory_manager->lock));
  err = _mami_locate(memory_manager, memory_manager->root, buffer, 1, &data);

  if (err >= 0) {
    if (data->nodes) {
      int i, *dests, *status;
      int nbpages = 0;
      void **pageaddrs;

      dests = tmalloc(data->nbpages * sizeof(int));
      status = tmalloc(data->nbpages * sizeof(int));
      pageaddrs  = tmalloc(data->nbpages * sizeof(void *));

      for(i=0 ; i<data->nbpages ; i++) {
        dests[i] = node;
        if (data->nodes[i] != node) {
          pageaddrs[nbpages] = data->pageaddrs[i];
          nbpages ++;
        }
      }

      err = _mm_move_pages(pageaddrs, nbpages, dests, status, MPOL_MF_MOVE);
      if (err >= 0) {
        data->node = node;
        tfree(data->nodes);
        data->nodes = NULL;
      }

      tfree(dests);
      tfree(status);
      tfree(pageaddrs);
    }
  }

  marcel_mutex_unlock(&(memory_manager->lock));
  MEMORY_LOG_OUT();
  return err;
}

#endif /* MM_MAMI_ENABLED */
