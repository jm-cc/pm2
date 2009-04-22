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

#define MARCEL_INTERNAL_INCLUDE

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

debug_type_t debug_memory = NEW_DEBUG_TYPE("MAR: ", "mar-mami-debug");
debug_type_t debug_memory_log = NEW_DEBUG_TYPE("MAR: ", "mar-mami-log");
debug_type_t debug_memory_ilog = NEW_DEBUG_TYPE("MAR: ", "mar-mami-ilog");
debug_type_t debug_memory_warn = NEW_DEBUG_TYPE("MAR: ", "mar-mami-warn");

/* The following is normally defined in <linux/mempolicy.h>. But that file is not always available. */
/* Policies */
enum {
	MPOL_DEFAULT,
	MPOL_PREFERRED,
	MPOL_BIND,
	MPOL_INTERLEAVE,
	MPOL_MAX,	/* always last member of enum */
};
/* Flags for mbind */
#  define MPOL_MF_STRICT	(1<<0)	/* Verify existing pages in the mapping */
#  define MPOL_MF_MOVE		(1<<1)	/* Move pages owned by this process to conform to mapping */

#if !defined(__NR_move_pages)

#  ifdef X86_64_ARCH
#    define __NR_move_pages 279
#  elif IA64_ARCH
#    define __NR_move_pages 1276
#  elif X86_ARCH
#    define __NR_move_pages 317
#  elif PPC_ARCH
#    define __NR_move_pages 301
#  elif PPC64_ARCH
#    define __NR_move_pages 301
#  else
#    error Syscall move pages undefined
#  endif

#endif /* __NR_move_pages */

#if !defined(__NR_mbind)

#  ifdef X86_64_ARCH
#    define __NR_mbind 237
#  elif IA64_ARCH
#    define __NR_mbind 1259
#  elif X86_ARCH
#    define __NR_mbind 274
#  elif PPC_ARCH
#    define __NR_mbind 259
#  elif PPC64_ARCH
#    define __NR_mbind 259
#  else
#    error Syscall mbind undefined
#  endif

#endif /* __NR_mbind */

#if !defined(__NR_set_mempolicy)

#  ifdef X86_64_ARCH
#    define __NR_set_mempolicy 238
#  elif IA64_ARCH
#    define __NR_set_mempolicy 1261
#  elif X86_ARCH
#    define __NR_set_mempolicy 276
#  elif PPC_ARCH
#    define __NR_set_mempolicy 261
#  elif PPC64_ARCH
#    define __NR_set_mempolicy 261
#  else
#    error Syscall set_mempolicy undefined
#  endif

#endif /* __NR_set_mempolicy */

static
mami_manager_t *g_memory_manager = NULL;
static
int memory_manager_sigsegv_handler_set = 0;

/* align a application-given address to the closest page-boundary:
 * re-add the lower bits to increase the bit above pagesize if needed, and truncate
 */
#define __ALIGN_ON_PAGE(address, pagesize) (void*)((((uintptr_t) address) + (pagesize >> 1)) & (~(pagesize-1)))
#define ALIGN_ON_PAGE(memory_manager, address, pagesize) (memory_manager->alignment?__ALIGN_ON_PAGE(address, pagesize):address)

/* Node id used for first touch allocated memory */
#define FIRST_TOUCH_NODE memory_manager->nb_nodes
/* Node id used when memory area located on several nodes */
#define MULTIPLE_LOCATION_NODE -2
/* Node id used when unknown location */
#define UNKNOWN_LOCATION_NODE -1

void mami_init(mami_manager_t *memory_manager) {
  int node, dest;
  void *ptr;
  int err;

  MAMI_LOG_IN();
  memory_manager->root = NULL;
  marcel_mutex_init(&(memory_manager->lock), NULL);
  memory_manager->normalpagesize = getpagesize();
  memory_manager->hugepagesize = marcel_topo_node_level[0].huge_page_size;
  memory_manager->initially_preallocated_pages = 1024;
  memory_manager->cache_line_size = 64;
  memory_manager->membind_policy = MAMI_MEMBIND_POLICY_NONE;
  memory_manager->nb_nodes = marcel_nbnodes;
  memory_manager->alignment = 1;

  // Is in-kernel migration available
#ifdef LINUX_SYS
  ptr = memalign(memory_manager->normalpagesize, memory_manager->normalpagesize);
  err = madvise(ptr, memory_manager->normalpagesize, 12);
  memory_manager->kernel_nexttouch_migration = (err>=0);
  free(ptr);
#else /* !LINUX_SYS */
  memory_manager->kernel_nexttouch_migration = 0;
#endif /* LINUX_SYS */
  mdebug_mami("Kernel next_touch migration: %d\n", memory_manager->kernel_nexttouch_migration);

  // Is migration available
  {
      unsigned long nodemask;
      nodemask = (1<<marcel_topo_node_level[0].os_node);
      ptr = memalign(memory_manager->normalpagesize, memory_manager->normalpagesize);
      err = ma_memory_mbind(ptr,  memory_manager->normalpagesize, MPOL_BIND, &nodemask, marcel_topo_node_level[memory_manager->nb_nodes-1].os_node+2, MPOL_MF_MOVE);
      memory_manager->migration_flag = err>=0 ? MPOL_MF_MOVE : 0;
      free(ptr);
      mdebug_mami("Migration: %d\n", memory_manager->migration_flag);
  }

  // How much total and free memory per node
  memory_manager->memtotal = tmalloc(memory_manager->nb_nodes * sizeof(unsigned long));
  memory_manager->memfree = tmalloc(memory_manager->nb_nodes * sizeof(unsigned long));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    if (marcel_topo_node_level) {
      memory_manager->memtotal[node] = marcel_topo_node_level[node].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_NODE];
      memory_manager->memfree[node] = marcel_topo_node_level[node].memory_kB[MARCEL_TOPO_LEVEL_MEMORY_NODE];
    }
    else {
      memory_manager->memtotal[node] = 0;
      memory_manager->memfree[node] = 0;
    }
  }

  // Preallocate memory on each node
  memory_manager->heaps = tmalloc((memory_manager->nb_nodes+1) * sizeof(mami_area_t *));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    ma_memory_preallocate(memory_manager, &(memory_manager->heaps[node]), memory_manager->initially_preallocated_pages, node, marcel_topo_node_level[node].os_node);
    mdebug_mami("Preallocating %p for node #%d %d\n", memory_manager->heaps[node]->start, node, marcel_topo_node_level[node].os_node);
  }
  ma_memory_preallocate(memory_manager, &(memory_manager->heaps[FIRST_TOUCH_NODE]), memory_manager->initially_preallocated_pages, FIRST_TOUCH_NODE, FIRST_TOUCH_NODE);
  mdebug_mami("Preallocating %p for anonymous heap\n", memory_manager->heaps[FIRST_TOUCH_NODE]->start);

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
  ma_memory_load_model_for_memory_migration(memory_manager);

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
  ma_memory_load_model_for_memory_access(memory_manager);

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

  MAMI_LOG_OUT();
}

static
void ma_memory_sampling_free(p_tbx_slist_t migration_costs) {
  MAMI_ILOG_IN();
  while (!tbx_slist_is_nil(migration_costs)) {
    mami_migration_cost_t *ptr = tbx_slist_extract(migration_costs);
    tfree(ptr);
  }
  tbx_slist_free(migration_costs);
  MAMI_ILOG_OUT();
}

static
int ma_memory_deallocate_huge_pages(mami_manager_t *memory_manager, mami_huge_pages_area_t **space, int node) {
  int err;

  MAMI_ILOG_IN();
  mdebug_mami("Releasing huge pages %s\n", (*space)->filename);

  err = close((*space)->file);
  if (err < 0) {
    perror("(ma_memory_deallocate_huge_pages) close");
    err = -errno;
  }
  else {
    err = unlink((*space)->filename);
    if (err < 0) {
      perror("(ma_memory_deallocate_huge_pages) unlink");
      err = -errno;
    }
  }

  tfree((*space)->filename);
  MAMI_ILOG_OUT();
  return err;
}

static
void ma_memory_deallocate(mami_manager_t *memory_manager, mami_area_t **space, int node) {
  mami_area_t *ptr, *ptr2;

  MAMI_ILOG_IN();
  mdebug_mami("Deallocating memory for node #%d\n", node);
  ptr = (*space);
  while (ptr != NULL) {
    mdebug_mami("Unmapping memory area from %p\n", ptr->start);
    munmap(ptr->start, ptr->nbpages * ptr->pagesize);
    ptr2 = ptr;
    ptr = ptr->next;
    tfree(ptr2);
  }
  MAMI_ILOG_OUT();
}

static
void ma_memory_clean_memory(mami_manager_t *memory_manager) {
  while (memory_manager->root) {
    mami_free(memory_manager, memory_manager->root->data->startaddress);
  }
}

void mami_exit(mami_manager_t *memory_manager) {
  int node, dest;
  struct sigaction act;

  MAMI_LOG_IN();

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
    ma_memory_clean_memory(memory_manager);
  }

  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    ma_memory_deallocate(memory_manager, &(memory_manager->heaps[node]), node);
  }
  ma_memory_deallocate(memory_manager, &(memory_manager->heaps[FIRST_TOUCH_NODE]), FIRST_TOUCH_NODE);
  tfree(memory_manager->heaps);

  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    if (memory_manager->huge_pages_heaps[node]) {
      ma_memory_deallocate_huge_pages(memory_manager, &(memory_manager->huge_pages_heaps[node]), node);
    }
  }
  tfree(memory_manager->huge_pages_heaps);

  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    for(dest=0 ; dest<memory_manager->nb_nodes ; dest++) {
      ma_memory_sampling_free(memory_manager->migration_costs[node][dest]);
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

  MAMI_LOG_OUT();
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
void ma_memory_init_memory_data(mami_manager_t *memory_manager,
                                void **pageaddrs, int nbpages, size_t size, int node, int *nodes,
                                int protection, int with_huge_pages, int mami_allocated,
                                mami_data_t **memory_data) {
  MAMI_ILOG_IN();

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

  if (node != MULTIPLE_LOCATION_NODE) (*memory_data)->nodes = NULL;
  else {
    (*memory_data)->nodes = nodes;
  }

  // Set the page addresses
  (*memory_data)->nbpages = nbpages;
  (*memory_data)->pageaddrs = tmalloc((*memory_data)->nbpages * sizeof(void *));
  memcpy((*memory_data)->pageaddrs, pageaddrs, nbpages*sizeof(void*));

  MAMI_ILOG_OUT();
}

static
void ma_memory_clean_memory_data(mami_data_t **memory_data) {
  mdebug_mami("Cleaning memory area %p\n", (*memory_data)->startaddress);
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

void ma_memory_delete_tree(mami_manager_t *memory_manager, mami_tree_t **memory_tree) {
  MAMI_ILOG_IN();
  if ((*memory_tree)->leftchild == NULL) {
    mami_tree_t *temp = (*memory_tree);
    ma_memory_clean_memory_data(&(temp->data));
    (*memory_tree) = (*memory_tree)->rightchild;
    tfree(temp);
  }
  else if ((*memory_tree)->rightchild == NULL) {
    mami_tree_t *temp = *memory_tree;
    ma_memory_clean_memory_data(&(temp->data));
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
    ma_memory_clean_memory_data(&((*memory_tree)->data));
    ma_memory_init_memory_data(memory_manager, temp->data->pageaddrs, temp->data->nbpages, temp->data->size, temp->data->node,
                               temp->data->nodes, temp->data->protection, temp->data->with_huge_pages, temp->data->mami_allocated,
                               &((*memory_tree)->data));

    // then delete the predecessor
    ma_memory_unregister(memory_manager, &((*memory_tree)->leftchild), temp->data->pageaddrs[0]);
  }
  MAMI_ILOG_OUT();
}

static
void ma_memory_free_from_node(mami_manager_t *memory_manager, void *buffer, size_t size, int nbpages, int node, int protection, int with_huge_pages) {
  mami_area_t *available;
  mami_area_t **ptr;
  mami_area_t **prev;
  mami_area_t *next;

  MAMI_ILOG_IN();

  mdebug_mami("Freeing space [%p:%p] with %d pages\n", buffer, buffer+size, nbpages);

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
      mdebug_mami("[%p:%p:%d] and [%p:%p:%d] can be defragmented\n", (*ptr)->start, (*ptr)->end, (*ptr)->protection,
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

  MAMI_ILOG_OUT();
}

void ma_memory_unregister(mami_manager_t *memory_manager, mami_tree_t **memory_tree, void *buffer) {
  MAMI_ILOG_IN();
  if (*memory_tree!=NULL) {
    if (buffer == (*memory_tree)->data->pageaddrs[0]) {
      mami_data_t *data = (*memory_tree)->data;
      mami_data_t *next_data = data->next;

      if (data->mami_allocated) {
	mdebug_mami("Removing [%p, %p] from node #%d\n", (*memory_tree)->data->pageaddrs[0], (*memory_tree)->data->pageaddrs[0]+(*memory_tree)->data->size,
                    data->node);
	VALGRIND_MAKE_MEM_NOACCESS((*memory_tree)->data->pageaddrs[0], (*memory_tree)->data->size);
	// Free memory
        if (data->node >= 0 && data->node <= memory_manager->nb_nodes)
          ma_memory_free_from_node(memory_manager, buffer, data->size, data->nbpages, data->node, data->protection, data->with_huge_pages);
      }
      else {
	mdebug_mami("Address %p is not allocated by MaMI.\n", buffer);
      }

      // Delete tree
      ma_memory_delete_tree(memory_manager, memory_tree);

      if (next_data) {
        mdebug_mami("Need to unregister the next memory area\n");
        ma_memory_unregister(memory_manager, &(memory_manager->root), next_data->startaddress);
      }
    }
    else if (buffer < (*memory_tree)->data->pageaddrs[0])
      ma_memory_unregister(memory_manager, &((*memory_tree)->leftchild), buffer);
    else
      ma_memory_unregister(memory_manager, &((*memory_tree)->rightchild), buffer);
  }
  MAMI_ILOG_OUT();
}

static
void ma_memory_register_pages(mami_manager_t *memory_manager, mami_tree_t **memory_tree,
                              void **pageaddrs, int nbpages, size_t size, int node, int *nodes,
                              int protection, int with_huge_pages, int mami_allocated,
                              mami_data_t **data) {
  MAMI_ILOG_IN();
  if (*memory_tree==NULL) {
    *memory_tree = tmalloc(sizeof(mami_tree_t));
    (*memory_tree)->leftchild = NULL;
    (*memory_tree)->rightchild = NULL;
    ma_memory_init_memory_data(memory_manager, pageaddrs, nbpages, size, node, nodes,
                               protection, with_huge_pages, mami_allocated, &((*memory_tree)->data));
    mdebug_mami("Adding data %p into tree\n", (*memory_tree)->data);
    if (data) *data = (*memory_tree)->data;
  }
  else {
    if (pageaddrs[0] < (*memory_tree)->data->pageaddrs[0])
      ma_memory_register_pages(memory_manager, &((*memory_tree)->leftchild), pageaddrs, nbpages, size, node, nodes,
                               protection, with_huge_pages, mami_allocated, data);
    else
      ma_memory_register_pages(memory_manager, &((*memory_tree)->rightchild), pageaddrs, nbpages, size, node, nodes,
                               protection, with_huge_pages, mami_allocated, data);
  }
  MAMI_ILOG_OUT();
}

int ma_memory_preallocate_huge_pages(mami_manager_t *memory_manager, mami_huge_pages_area_t **space, int node) {
  int nbpages;
  pid_t pid;

  MAMI_ILOG_IN();
  nbpages = marcel_topo_node_level[node].huge_page_free;
  if (!nbpages) {
    mdebug_mami("No huge pages on node #%d\n", node);
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
    perror("(ma_memory_preallocate_huge_pages) open");
    return -errno;
  }

  (*space)->buffer = mmap(NULL, (*space)->size, PROT_READ|PROT_WRITE, MAP_PRIVATE, (*space)->file, 0);
  if ((*space)->buffer == MAP_FAILED) {
    perror("(ma_memory_preallocate_huge_pages) mmap");
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

  MAMI_ILOG_OUT();
  return 0;
}

int ma_memory_preallocate(mami_manager_t *memory_manager, mami_area_t **space, int nbpages, int vnode, int pnode) {
  unsigned long nodemask;
  size_t length;
  void *buffer;
  int i, err=0;

  MAMI_ILOG_IN();

  if (vnode != FIRST_TOUCH_NODE && memory_manager->memtotal[vnode] == 0) {
    mdebug_mami("No memory available on node #%d, os_node #%d\n", vnode, pnode);
    err = -EINVAL;
    buffer = NULL;
    length = 0;
  }
  else {
    length = nbpages * memory_manager->normalpagesize;
    buffer = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED) {
      perror("(ma_memory_preallocate) mmap");
      err = -errno;
    }
    else {
      if (vnode != FIRST_TOUCH_NODE) {
        nodemask = (1<<pnode);
        mdebug_mami("Mbinding on node %d with nodemask %ld and max node %d\n", pnode, nodemask, marcel_topo_node_level[memory_manager->nb_nodes-1].os_node+2);
        err = ma_memory_mbind(buffer, length, MPOL_BIND, &nodemask, marcel_topo_node_level[memory_manager->nb_nodes-1].os_node+2, MPOL_MF_STRICT|memory_manager->migration_flag);
        if (err < 0) {
          perror("(ma_memory_preallocate) mbind");
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

  mdebug_mami("Preallocating [%p:%p] on node #%d\n", buffer,buffer+length, pnode);
  MAMI_ILOG_OUT();
  return err;
}

static
void* ma_memory_get_buffer_from_huge_pages_heap(mami_manager_t *memory_manager, int node, int nbpages, size_t size, int *protection) {
  mami_huge_pages_area_t *heap = memory_manager->huge_pages_heaps[node];
  mami_area_t *hheap = NULL, *prev = NULL;
  void *buffer = NULL;
  unsigned long nodemask;
  int err=0;

  if (memory_manager->huge_pages_heaps[node] == NULL) {
    err = ma_memory_preallocate_huge_pages(memory_manager, &(memory_manager->huge_pages_heaps[node]), node);
    if (err < 0) {
      return NULL;
    }
    mdebug_mami("Preallocating heap %p with huge pages for node #%d\n", memory_manager->huge_pages_heaps[node], node);
    heap = memory_manager->huge_pages_heaps[node];
  }

  if (heap == NULL) {
    mdebug_mami("No huge pages are available on node #%d\n", node);
    return NULL;
  }

  mdebug_mami("Requiring space from huge pages area of size=%lu on node #%d\n", (long unsigned)heap->size, node);

  // Look for a space big enough
  hheap = heap->heap;
  prev = hheap;
  while (hheap != NULL) {
    mdebug_mami("Current space from %p with %d pages\n", hheap->start, hheap->nbpages);
    if (hheap->nbpages >= nbpages)
      break;
    prev = hheap;
    hheap = hheap->next;
  }
  if (hheap == NULL) {
    mdebug_mami("Not enough huge pages are available on node #%d\n", node);
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
  err = ma_memory_set_mempolicy(MPOL_BIND, &nodemask, memory_manager->nb_nodes+2);
  if (err < 0) {
    perror("(ma_memory_get_buffer_from_huge_pages_heap) set_mempolicy");
    return NULL;
  }
  memset(buffer, 0, size);
  err = ma_memory_set_mempolicy(MPOL_DEFAULT, NULL, 0);
  if (err < 0) {
    perror("(ma_memory_get_buffer_from_huge_pages_heap) set_mempolicy");
    return NULL;
  }

  return buffer;
}

static
void* ma_memory_get_buffer_from_heap(mami_manager_t *memory_manager, int node, int nbpages, size_t size, int *protection) {
  mami_area_t *heap = memory_manager->heaps[node];
  mami_area_t *prev = NULL;
  void *buffer;

  mdebug_mami("Requiring space of %d pages from heap %p on node #%d\n", nbpages, heap, node);

  // Look for a space big enough
  prev = heap;
  while (heap != NULL && heap->start != NULL) {
    mdebug_mami("Current space from %p with %d pages\n", heap->start, heap->nbpages);
    if (heap->nbpages >= nbpages)
      break;
    prev = heap;
    heap = heap->next;
  }
  if (heap == NULL || heap->start == NULL) {
    int err, preallocatedpages;

    preallocatedpages = nbpages;
    if (preallocatedpages < memory_manager->initially_preallocated_pages) {
      preallocatedpages = memory_manager->initially_preallocated_pages;
    }
    mdebug_mami("not enough space, let's allocate %d extra pages\n", preallocatedpages);
    err = ma_memory_preallocate(memory_manager, &heap, preallocatedpages, node, marcel_topo_node_level[node].os_node);
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
void* ma_memory_malloc(mami_manager_t *memory_manager, size_t size, unsigned long pagesize, int node, int with_huge_pages) {
  void *buffer;
  int i, nbpages, protection=0;
  size_t realsize;
  void **pageaddrs;

  MAMI_ILOG_IN();

  if (!size) {
    MAMI_ILOG_OUT();
    return NULL;
  }

  marcel_mutex_lock(&(memory_manager->lock));

  // Round-up the size
  nbpages = size / pagesize;
  if (nbpages * pagesize != size) nbpages++;
  realsize = nbpages * pagesize;

  if (!with_huge_pages) {
    buffer = ma_memory_get_buffer_from_heap(memory_manager, node, nbpages, realsize, &protection);
  }
  else {
    buffer = ma_memory_get_buffer_from_huge_pages_heap(memory_manager, node, nbpages, realsize, &protection);
  }

  if (!buffer) {
    errno = ENOMEM;
  }
  else {
    // Set the page addresses
    pageaddrs = tmalloc(nbpages * sizeof(void *));
    for(i=0; i<nbpages ; i++) pageaddrs[i] = buffer + i*pagesize;

    // Register memory
    mdebug_mami("Registering [%p:%p:%ld]\n", pageaddrs[0], pageaddrs[0]+size, (long)size);
    ma_memory_register_pages(memory_manager, &(memory_manager->root), pageaddrs, nbpages, realsize, node, NULL,
                             protection, with_huge_pages, 1, NULL);

    tfree(pageaddrs);
    VALGRIND_MAKE_MEM_UNDEFINED(buffer, size);
  }

  marcel_mutex_unlock(&(memory_manager->lock));
  mdebug_mami("Allocating %p on node #%d\n", buffer, node);
  MAMI_ILOG_OUT();
  return buffer;
}

void* mami_malloc(mami_manager_t *memory_manager, size_t size, mami_membind_policy_t policy, int node) {
  void *ptr;
  int with_huge_pages;
  unsigned long pagesize;

  MAMI_LOG_IN();

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
    node = FIRST_TOUCH_NODE;
  }

  if (tbx_unlikely(policy != MAMI_MEMBIND_POLICY_FIRST_TOUCH && node >= memory_manager->nb_nodes)) {
    mdebug_mami("Node %d not managed by MaMI\n", node);
    MAMI_LOG_OUT();
    return NULL;
  }

  ptr = ma_memory_malloc(memory_manager, size, pagesize, node, with_huge_pages);

  MAMI_LOG_OUT();
  return ptr;
}

void* mami_calloc(mami_manager_t *memory_manager, size_t nmemb, size_t size,
                           mami_membind_policy_t policy, int node) {
  void *ptr;

  MAMI_LOG_IN();

  ptr = mami_malloc(memory_manager, nmemb*size, policy, node);
  ptr = memset(ptr, 0, nmemb*size);

  MAMI_LOG_OUT();
  return ptr;
}

static
int ma_memory_locate(mami_manager_t *memory_manager, mami_tree_t *memory_tree, void *buffer, size_t size, mami_data_t **data) {
  if (memory_tree==NULL) {
    mdebug_mami("The interval [%p:%p] is not managed by MaMI.\n", buffer, buffer+size);
    errno = EINVAL;
    return -errno;
  }
  //mdebug_mami("Comparing [%p:%p] and [%p:%p]\n", buffer,buffer+size, memory_tree->data->startaddress, memory_tree->data->endaddress);
  if (buffer >= memory_tree->data->startaddress && buffer+size <= memory_tree->data->endaddress) {
    // the address is stored on the current memory_data
    mdebug_mami("Found interval [%p:%p] in [%p:%p]\n", buffer, buffer+size, memory_tree->data->startaddress, memory_tree->data->endaddress);
    mdebug_mami("Interval [%p:%p] is located on node #%d\n", buffer, buffer+size, memory_tree->data->node);
    *data = memory_tree->data;
    return 0;
  }
  else if (buffer <= memory_tree->data->startaddress) {
    return ma_memory_locate(memory_manager, memory_tree->leftchild, buffer, size, data);
  }
  else if (buffer >= memory_tree->data->endaddress) {
    return ma_memory_locate(memory_manager, memory_tree->rightchild, buffer, size, data);
  }
  else {
    errno = EINVAL;
    return -errno;
  }
}

static
int ma_memory_get_pages_location(mami_manager_t *memory_manager, void **pageaddrs, int nbpages, int *node, int **nodes) {
  int *statuses;
  int i, err=0;

  statuses = tmalloc(nbpages * sizeof(int));
  err = ma_memory_move_pages(pageaddrs, nbpages, NULL, statuses, 0);
  if (err < 0 || statuses[0] == -ENOENT) {
    mdebug_mami("Could not locate pages\n");
    if (*node != FIRST_TOUCH_NODE) {
      *node = UNKNOWN_LOCATION_NODE;
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
        if (*node != FIRST_TOUCH_NODE) {
          *node = MULTIPLE_LOCATION_NODE;
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

void ma_memory_register(mami_manager_t *memory_manager,
                        void *buffer,
                        size_t size,
                        int mami_allocated,
                        mami_data_t **data) {
  void **pageaddrs;
  int nbpages, protection, with_huge_pages;
  int i, node, *nodes;

  mdebug_mami("Registering address interval [%p:%p:%lu]\n", buffer, buffer+size, (long)size);

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
  ma_memory_get_pages_location(memory_manager, pageaddrs, nbpages, &node, &nodes);

  // Register the pages
  ma_memory_register_pages(memory_manager, &(memory_manager->root), pageaddrs, nbpages, size, node, nodes,
                           protection, with_huge_pages, mami_allocated, data);

  // Free temporary array
  tfree(pageaddrs);
}

int mami_register(mami_manager_t *memory_manager,
                  void *buffer,
                  size_t size) {
  void *aligned_buffer = ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
  void *aligned_endbuffer = ALIGN_ON_PAGE(memory_manager, buffer+size, memory_manager->normalpagesize);
  size_t aligned_size = aligned_endbuffer-aligned_buffer;
  mami_data_t *data = NULL;

  MAMI_LOG_IN();
  if (aligned_size > size) aligned_size = size;
  marcel_mutex_lock(&(memory_manager->lock));
  mdebug_mami("Registering [%p:%p:%ld]\n", aligned_buffer, aligned_buffer+aligned_size, (long)aligned_size);
  ma_memory_register(memory_manager, aligned_buffer, aligned_size, 0, &data);

  if (aligned_endbuffer > buffer+size) {
    data->mprotect_size = data->size - memory_manager->normalpagesize;
  }
  if (aligned_buffer < buffer) {
    data->mprotect_startaddress += memory_manager->normalpagesize;
    data->mprotect_size -= memory_manager->normalpagesize;
  }

  marcel_mutex_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
  return 0;
}

int mami_unregister(mami_manager_t *memory_manager,
                    void *buffer) {
  mami_data_t *data = NULL;
  int err=0;
  void *aligned_buffer;

  MAMI_LOG_IN();
  marcel_mutex_lock(&(memory_manager->lock));

  aligned_buffer = ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
  err = ma_memory_locate(memory_manager, memory_manager->root, aligned_buffer, 1, &data);
  if (err >= 0) {
    mdebug_mami("Unregistering [%p:%p]\n", buffer,buffer+data->size);
    ma_memory_unregister(memory_manager, &(memory_manager->root), aligned_buffer);
  }
  marcel_mutex_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
  return err;
}

int mami_split(mami_manager_t *memory_manager,
               void *buffer,
               unsigned int subareas,
               void **newbuffers) {
  mami_data_t *data = NULL;
  int err=0;

  MAMI_LOG_IN();
  marcel_mutex_lock(&(memory_manager->lock));

  err = ma_memory_locate(memory_manager, memory_manager->root, buffer, 1, &data);
  if (err >= 0) {
    size_t subsize;
    int subpages, i;
    void **pageaddrs;

    subpages = data->nbpages / subareas;
    if (!subpages) {
      mdebug_mami("Memory area from %p with size %ld and %d pages not big enough to be split in %d subareas\n",
                  data->startaddress, (long) data->size, data->nbpages, subareas);
      errno = EINVAL;
      err = -errno;
    }
    else {
      mdebug_mami("Splitting area from %p in %d areas of %d pages\n", data->startaddress, subareas, subpages);

      // Register new subareas
      pageaddrs = data->pageaddrs+subpages;
      if (data->with_huge_pages)
	subsize = subpages * memory_manager->hugepagesize;
      else
	subsize = subpages * memory_manager->normalpagesize;
      for(i=1 ; i<subareas ; i++) {
	newbuffers[i] = pageaddrs[0];
	ma_memory_register_pages(memory_manager, &(memory_manager->root), pageaddrs, subpages, subsize, data->node, data->nodes,
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
  MAMI_LOG_OUT();
  return err;
}

int mami_membind(mami_manager_t *memory_manager,
                 mami_membind_policy_t policy,
                 int node) {
  int err=0;

  MAMI_LOG_IN();
  if (tbx_unlikely(node >= memory_manager->nb_nodes)) {
    mdebug_mami("Node #%d invalid\n", node);
    errno = EINVAL;
    err = -errno;
  }
  else if (policy == MAMI_MEMBIND_POLICY_HUGE_PAGES && memory_manager->hugepagesize == 0) {
    mdebug_mami("Huge pages are not available. Cannot set memory policy.\n");
    errno = EINVAL;
    err = -errno;
  }
  else {
    mdebug_mami("Set the current membind policy to %d (node #%d)\n", policy, node);
    memory_manager->membind_policy = policy;
    memory_manager->membind_node = node;
  }
  MAMI_LOG_OUT();
  return err;
}

void mami_free(mami_manager_t *memory_manager, void *buffer) {
  void *aligned_buffer;

  MAMI_LOG_IN();
  aligned_buffer = ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
  mdebug_mami("Freeing [%p]\n", buffer);
  marcel_mutex_lock(&(memory_manager->lock));
  ma_memory_unregister(memory_manager, &(memory_manager->root), aligned_buffer);
  marcel_mutex_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
}

int ma_memory_mbind(void *start, unsigned long len, int mode,
                    const unsigned long *nmask, unsigned long maxnode, unsigned flags) {
  int err = 0;

  MAMI_ILOG_IN();
  if (ma_use_synthetic_topology) {
      MAMI_ILOG_OUT();
      return err;
  }
  mdebug_mami("binding on mask %ld\n", nmask);
#if defined (X86_64_ARCH) && defined (X86_ARCH)
  err = syscall6(__NR_mbind, (long)start, len, mode, (long)nmask, maxnode, flags);
#else
  err = syscall(__NR_mbind, (long)start, len, mode, (long)nmask, maxnode, flags);
#endif
  if (err < 0) perror("(ma_memory_mbind) mbind");
  return err;
}

int ma_memory_move_pages(void **pageaddrs, int pages, int *nodes, int *status, int flag) {
  int err=0;

  MAMI_ILOG_IN();
  if (ma_use_synthetic_topology) {
      MAMI_ILOG_OUT();
      return err;
  }
  if (nodes) mdebug_mami("binding on numa node #%d\n", nodes[0]);

#if defined (X86_64_ARCH) && defined (X86_ARCH)
  err = syscall6(__NR_move_pages, 0, pages, pageaddrs, nodes, status, flag);
#else
  err = syscall(__NR_move_pages, 0, pages, pageaddrs, nodes, status, flag);
#endif
  if (err < 0) perror("(ma_memory_move_pages) move_pages");
  MAMI_ILOG_OUT();
  return err;
}

int ma_memory_set_mempolicy(int mode, const unsigned long *nmask, unsigned long maxnode) {
  int err=0;

  MAMI_ILOG_IN();
  if (ma_use_synthetic_topology) {
    MAMI_ILOG_OUT();
    return err;
  }
  err = syscall(__NR_set_mempolicy, mode, nmask, maxnode);
  if (err < 0) perror("(ma_memory_set_mempolicy) set_mempolicy");
  MAMI_ILOG_OUT();
  return err;
}

int ma_memory_check_pages_location(void **pageaddrs, int pages, int node) {
  int *pagenodes;
  int i;
  int err=0;

  MAMI_ILOG_IN();
  mdebug_mami("check location of the %d pages is #%d\n", pages, node);

  if (ma_use_synthetic_topology) {
    mdebug_mami("Using synthethic topology. No need to check physically\n");
    return err;
  }

  pagenodes = tmalloc(pages * sizeof(int));
  err = ma_memory_move_pages(pageaddrs, pages, NULL, pagenodes, 0);
  if (err < 0) perror("(ma_memory_check_pages_location) move_pages");
  else {
    for(i=0; i<pages; i++) {
      if (pagenodes[i] != node) {
        marcel_fprintf(stderr, "MaMI Warning: page #%d is not located on node #%d but on node #%d\n", i, node, pagenodes[i]);
        err = -EINVAL;
      }
    }
  }
  tfree(pagenodes);
  MAMI_ILOG_OUT();
  return err;
}

int mami_locate(mami_manager_t *memory_manager, void *buffer, size_t size, int *node) {
  mami_data_t *data = NULL;
  void *aligned_buffer = ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
  void *aligned_endbuffer = ALIGN_ON_PAGE(memory_manager, buffer+size, memory_manager->normalpagesize);
  size_t aligned_size = aligned_endbuffer-aligned_buffer;
  int err, *nodes;

  MAMI_LOG_IN();
  if (aligned_size > size) aligned_size = size;
  mdebug_mami("Trying to locate [%p:%p:%ld]\n", aligned_buffer, aligned_buffer+aligned_size, (long)aligned_size);
  err = ma_memory_locate(memory_manager, memory_manager->root, aligned_buffer, aligned_size, &data);
  if (err >= 0) {
    if (data->status == MAMI_KERNEL_MIGRATION_STATUS) {
      int node;
      ma_memory_get_pages_location(memory_manager, data->pageaddrs, data->nbpages, &node, &nodes);
      if (node != data->node) {
        mdebug_mami("Updating location of the pages after a kernel migration\n");
        data->node = node;
        data->status = MAMI_INITIAL_STATUS;
      }
    }
    *node = data->node;
  }
  MAMI_LOG_OUT();
  return err;
}

int mami_check_pages_location(mami_manager_t *memory_manager, void *buffer, size_t size, int node) {
  mami_data_t *data = NULL;
  void *aligned_buffer = ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
  void *aligned_endbuffer = ALIGN_ON_PAGE(memory_manager, buffer+size, memory_manager->normalpagesize);
  size_t aligned_size = aligned_endbuffer-aligned_buffer;
  int err=0;

  MAMI_LOG_IN();
  if (aligned_size > size) aligned_size = size;
  err = ma_memory_locate(memory_manager, memory_manager->root, aligned_buffer, aligned_size, &data);
  if (err >= 0) {
    err = ma_memory_check_pages_location(data->pageaddrs, data->nbpages, node);
    if (err < 0) {
      marcel_fprintf(stderr, "MaMI: The %d pages are NOT all on node #%d\n", data->nbpages, node);
    }
    else {
      marcel_fprintf(stderr, "MaMI: The %d pages are all on node #%d\n", data->nbpages, node);
    }
  }
  MAMI_LOG_OUT();
  return err;
}

int mami_update_pages_location(mami_manager_t *memory_manager,
                               void *buffer,
                               size_t size) {
  mami_data_t *data = NULL;
  void *aligned_buffer = ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
  void *aligned_endbuffer = ALIGN_ON_PAGE(memory_manager, buffer+size, memory_manager->normalpagesize);
  size_t aligned_size = aligned_endbuffer-aligned_buffer;
  int err, node, *nodes;

  MAMI_LOG_IN();
  if (aligned_size > size) aligned_size = size;
  err = ma_memory_locate(memory_manager, memory_manager->root, aligned_buffer, aligned_size, &data);
  if (err >= 0) {
    ma_memory_get_pages_location(memory_manager, data->pageaddrs, data->nbpages, &node, &nodes);
    if (node != data->node) {
      mdebug_mami("Updating pages location from node #%d to node #%d\n", data->node, node);
      data->node = node;
    }
  }
  MAMI_LOG_OUT();
  return err;
}

static
void ma_memory_print(mami_tree_t *memory_tree, FILE *stream, int indent) {
  if (memory_tree) {
    int x;
    ma_memory_print(memory_tree->leftchild, stream, indent+2);
    for(x=0 ; x<indent ; x++) marcel_fprintf(stream, " ");
    marcel_fprintf(stream, "[%p, %p]\n", memory_tree->data->startaddress, memory_tree->data->endaddress);
    ma_memory_print(memory_tree->rightchild, stream, indent+2);
  }
}

void mami_print(mami_manager_t *memory_manager) {
  MAMI_LOG_IN();
  mdebug_mami("******************** TREE BEGIN *********************************\n");
  ma_memory_print(memory_manager->root, stdout, 0);
  mdebug_mami("******************** TREE END *********************************\n");
  MAMI_LOG_OUT();
}

void mami_fprint(mami_manager_t *memory_manager, FILE *stream) {
  MAMI_LOG_IN();
  mdebug_mami("******************** TREE BEGIN *********************************\n");
  ma_memory_print(memory_manager->root, stream, 0);
  mdebug_mami("******************** TREE END *********************************\n");
  MAMI_LOG_OUT();
}

int mami_migration_cost(mami_manager_t *memory_manager,
                        int source,
                        int dest,
                        size_t size,
                        float *cost) {
  p_tbx_slist_t migration_costs;
  int err=0;

  MAMI_LOG_IN();
  if (tbx_unlikely(source >= memory_manager->nb_nodes || dest >= memory_manager->nb_nodes)) {
    mdebug_mami("Invalid node id\n");
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
  MAMI_LOG_OUT();
  return err;
}

int mami_cost_for_write_access(mami_manager_t *memory_manager,
                               int source,
                               int dest,
                               size_t size,
                               float *cost) {
  mami_access_cost_t access_cost;
  int err;

  MAMI_LOG_IN();
  if (tbx_unlikely(source >= memory_manager->nb_nodes || dest >= memory_manager->nb_nodes)) {
    mdebug_mami("Invalid node id\n");
    errno = EINVAL;
    err = -errno;
  }
  else {
    access_cost = memory_manager->costs_for_write_access[source][dest];
    *cost = (size/memory_manager->cache_line_size) * access_cost.cost;
  }
  MAMI_LOG_OUT();
  return err;
}

int mami_cost_for_read_access(mami_manager_t *memory_manager,
                              int source,
                              int dest,
                              size_t size,
                              float *cost) {
  mami_access_cost_t access_cost;
  int err;

  MAMI_LOG_IN();
  if (tbx_unlikely(source >= memory_manager->nb_nodes || dest >= memory_manager->nb_nodes)) {
    mdebug_mami("Invalid node id\n");
    errno = EINVAL;
    err = -errno;
  }
  else {
    access_cost = memory_manager->costs_for_read_access[source][dest];
    *cost = ((float)size/(float)memory_manager->cache_line_size) * access_cost.cost;
  }
  MAMI_LOG_OUT();
  return err;
}

int mami_select_node(mami_manager_t *memory_manager,
                     mami_node_selection_policy_t policy,
                     int *node) {
  int err=0;

  MAMI_LOG_IN();
  marcel_mutex_lock(&(memory_manager->lock));

  if (policy == MAMI_LEAST_LOADED_NODE) {
    int i;
    unsigned long maxspace=memory_manager->memfree[0];
    mdebug_mami("Space on node %d = %ld\n", 0, maxspace);
    *node = 0;
    for(i=1 ; i<memory_manager->nb_nodes ; i++) {
      mdebug_mami("Space on node %d = %ld\n", i, memory_manager->memfree[i]);
      if (memory_manager->memfree[i] > maxspace) {
        maxspace = memory_manager->memfree[i];
        *node = i;
      }
    }
  }
  else {
    mdebug_mami("Policy #%d unknown\n", policy);
    errno = EINVAL;
    err = -errno;
  }

  marcel_mutex_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
  return err;
}

static
int ma_memory_migrate_pages(mami_manager_t *memory_manager,
                            mami_data_t *data, int dest) {
  int err=0;

  MAMI_ILOG_IN();
  if (data->node == dest) {
    mdebug_mami("The address %p is already located at the required node.\n", data->startaddress);
    err = EALREADY;
  }
  else {
    if (data->node == FIRST_TOUCH_NODE || data->node == UNKNOWN_LOCATION_NODE) {
      unsigned long nodemask;

      mdebug_mami("Mbinding %d page(s) to node #%d\n", data->nbpages, dest);
      nodemask = (1<<dest);
      err = ma_memory_mbind(data->startaddress, data->size, MPOL_BIND, &nodemask, memory_manager->nb_nodes+2, MPOL_MF_MOVE|MPOL_MF_STRICT);
    }
    else {
      int i, dests[data->nbpages], status[data->nbpages];

      mdebug_mami("Migrating %d page(s) to node #%d\n", data->nbpages, dest);
      for(i=0 ; i<data->nbpages ; i++) dests[i] = dest;

      if (data->node != MULTIPLE_LOCATION_NODE) {
        err = ma_memory_move_pages(data->pageaddrs, data->nbpages, dests, status, MPOL_MF_MOVE);
      }
      else {
        void *pageaddrs_to_be_moved[data->nbpages];
        int nbpages_to_be_moved=0;
        unsigned long nodemask;

        nodemask = (1<<dest);
        mdebug_mami("Pages on different locations. Check which ones need to be moved and which ones need to be bound\n");
        // Some pages might already be at the right location */
        for(i=0 ; i<data->nbpages ; i++) {
          if (data->nodes[i] == -ENOENT) {
            mdebug_mami("Mbinding page %d (%p) to node #%d\n", i, data->pageaddrs[i], dest);
            err = ma_memory_mbind(data->pageaddrs[i], memory_manager->normalpagesize, MPOL_BIND, &nodemask,
                                  memory_manager->nb_nodes+2, MPOL_MF_MOVE|MPOL_MF_STRICT);
          }
          else if (data->nodes[i] != dest) {
            pageaddrs_to_be_moved[nbpages_to_be_moved] = data->pageaddrs[i];
            nbpages_to_be_moved ++;
          }
        }
        mdebug_mami("%d page(s) need to be moved\n", nbpages_to_be_moved);
        if (nbpages_to_be_moved) {
          err = ma_memory_move_pages(pageaddrs_to_be_moved, nbpages_to_be_moved, dests, status, MPOL_MF_MOVE);
        }
      }
    }

    if (err < 0) {
      mdebug_mami("Error when mbinding or migrating: %d\n", err);
    }
    else {
      if (!tbx_slist_is_nil(data->owners)) {
        tbx_slist_ref_to_head(data->owners);
        do {
          marcel_entity_t *object = NULL;
          object = tbx_slist_ref_get(data->owners);

          mdebug_mami("Moving data from node #%d to node #%d\n", data->node, dest);
          if (data->node >= 0) ((long *) ma_stats_get (object, ma_stats_memnode_offset))[data->node] -= data->size;
          ((long *) ma_stats_get (object, ma_stats_memnode_offset))[dest] += data->size;
        } while (tbx_slist_ref_forward(data->owners));
      }
      data->node = dest;
    }
  }

  MAMI_ILOG_OUT();
  errno = err;
  return -err;
}

int mami_migrate_pages(mami_manager_t *memory_manager,
                       void *buffer, int dest) {
  int ret;
  mami_data_t *data = NULL;

  MAMI_LOG_IN();
  marcel_mutex_lock(&(memory_manager->lock));
  ret = ma_memory_locate(memory_manager, memory_manager->root, buffer, 1, &data);
  if (ret >= 0) {
    ret = ma_memory_migrate_pages(memory_manager, data, dest);
  }
  marcel_mutex_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
  return ret;
}

static
void ma_memory_segv_handler(int sig, siginfo_t *info, void *_context) {
  void *addr;
  int err, dest;
  mami_data_t *data = NULL;

  marcel_mutex_lock(&(g_memory_manager->lock));

#warning look at www.gnu.org/software/libsigsegv/

  addr = info->si_addr;
  err = ma_memory_locate(g_memory_manager, g_memory_manager->root, addr, 1, &data);
  if (err < 0) {
    // The address is not managed by MaMI. Reset the segv handler to its default action, to cause a segfault
    struct sigaction act;
    act.sa_flags = SA_SIGINFO;
    act.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &act, NULL);
  }
  if (data->status != MAMI_NEXT_TOUCHED_STATUS) {
    data->status = MAMI_NEXT_TOUCHED_STATUS;
    dest = marcel_current_node();
    ma_memory_migrate_pages(g_memory_manager, data, dest);
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

  MAMI_LOG_IN();
  marcel_mutex_lock(&(memory_manager->lock));

  g_memory_manager = memory_manager;
  aligned_buffer = ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
  mdebug_mami("Trying to locate [%p:%p:1]\n", aligned_buffer, aligned_buffer+1);
  err = ma_memory_locate(memory_manager, memory_manager->root, aligned_buffer, 1, &data);
  if (err >= 0) {
    mdebug_mami("Setting migrate on next touch on address %p (%p)\n", data->startaddress, buffer);
    if (memory_manager->kernel_nexttouch_migration == 1 && data->mami_allocated) {
      mdebug_mami("... using in-kernel migration\n");
      data->status = MAMI_KERNEL_MIGRATION_STATUS;
      err = ma_memory_mbind(data->startaddress, data->size, MPOL_DEFAULT, NULL, 0, 0);
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
      mdebug_mami("... using user-space migration\n");
      if (!memory_manager_sigsegv_handler_set) {
        struct sigaction act;
        memory_manager_sigsegv_handler_set = 1;
        act.sa_flags = SA_SIGINFO;
        act.sa_sigaction = ma_memory_segv_handler;
        err = sigaction(SIGSEGV, &act, NULL);
        if (err < 0) {
          perror("(mami_migrate_on_next_touch) sigaction");
          marcel_mutex_unlock(&(memory_manager->lock));
          MAMI_LOG_OUT();
          return -errno;
        }
      }
      mdebug_mami("Mprotecting [%p:%p:%ld] (initially [%p:%p:%ld]\n", data->mprotect_startaddress, data->startaddress+data->mprotect_size,
		  data->mprotect_size, data->startaddress, data->endaddress, data->size);
      data->status = MAMI_INITIAL_STATUS;
      err = mprotect(data->mprotect_startaddress, data->mprotect_size, PROT_NONE);
      if (err < 0) {
        perror("(mami_migrate_on_next_touch) mprotect");
      }
    }
  }
  marcel_mutex_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
  return err;
}

int mami_migrate_on_node(mami_manager_t *memory_manager,
                         void *buffer,
                         int node) {
  mami_data_t *data = NULL;
  int err;

  MAMI_LOG_IN();
  marcel_mutex_lock(&(memory_manager->lock));

  err = ma_memory_locate(memory_manager, memory_manager->root, buffer, 1, &data);
  if (err >= 0) {
    err = ma_memory_migrate_pages(memory_manager, data, node);
  }
  marcel_mutex_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
  return err;
}

static
int ma_memory_entity_attach(mami_manager_t *memory_manager,
                            void *buffer,
                            size_t size,
                            marcel_entity_t *owner,
                            int *node) {
  int err=0;
  mami_data_t *data;

  MAMI_ILOG_IN();
  marcel_mutex_lock(&(memory_manager->lock));

  mdebug_mami("Attaching [%p:%p:%ld] to entity %p\n", buffer, buffer+size, (long)size, owner);

  if (!buffer) {
    mdebug_mami("Cannot attach NULL buffer\n");
    errno = EINVAL;
    err = -errno;
  }
  else {
    void *aligned_buffer = ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);
    void *aligned_endbuffer = ALIGN_ON_PAGE(memory_manager, buffer+size, memory_manager->normalpagesize);
    size_t aligned_size = aligned_endbuffer-aligned_buffer;
    mami_data_link_t *area;

    if (!aligned_size) {
      aligned_endbuffer = aligned_buffer + memory_manager->normalpagesize;
      aligned_size = memory_manager->normalpagesize;
    }

    err = ma_memory_locate(memory_manager, memory_manager->root, aligned_buffer, 1, &data);
    if (err < 0) {
      mdebug_mami("The address interval [%p:%p] is not managed by MaMI. Let's register it\n", aligned_buffer, aligned_endbuffer);
      ma_memory_register(memory_manager, aligned_buffer, aligned_size, 0, &data);

      if (aligned_endbuffer > buffer+size) {
	data->mprotect_size = data->size - memory_manager->normalpagesize;
      }
      if (aligned_buffer < buffer) {
	data->mprotect_startaddress += memory_manager->normalpagesize;
	data->mprotect_size -= memory_manager->normalpagesize;
      }

      err = 0;
    }
    else {
      if (data->node == FIRST_TOUCH_NODE || data->node == UNKNOWN_LOCATION_NODE) {
        mdebug_mami("Need to find out the location of the memory area\n");
        ma_memory_get_pages_location(memory_manager, data->pageaddrs, data->nbpages, &(data->node), &(data->nodes));
      }
      if (size < data->size) {
        size_t newsize;
        mami_data_t *next_data;

        newsize = data->size-aligned_size;
        if (!newsize) {
          mdebug_mami("Cannot split a page\n");
        }
        else {
          mdebug_mami("Splitting [%p:%p] in [%p:%p] and [%p:%p]\n", data->startaddress,data->endaddress,
                      data->startaddress,data->startaddress+aligned_size, aligned_endbuffer,aligned_endbuffer+newsize);
          data->nbpages = aligned_size/memory_manager->normalpagesize;
          data->endaddress = data->startaddress + aligned_size;
          data->size = aligned_size;
          ma_memory_register(memory_manager, aligned_endbuffer, newsize, data->mami_allocated, &next_data);
          data->next = next_data;
        }
      }
    }

    mdebug_mami("Adding entity %p to data %p\n", owner, data);
    tbx_slist_push(data->owners, owner);

    *node = data->node;
    if (*node >= 0) {
      mdebug_mami("Adding %lu bits to memnode offset for node #%d\n", (long unsigned)data->size, *node);
      ((long *) ma_stats_get (owner, ma_stats_memnode_offset))[*node] += data->size;
    }
    else {
      mdebug_mami("Cannot attach data as location undefined #%d\n", *node);
    }

    area = tmalloc(sizeof(mami_data_link_t));
    area->data = data;
    INIT_LIST_HEAD(&(area->list));
    ma_spin_lock(&(owner->memory_areas_lock));
    list_add(&(area->list), &(owner->memory_areas));
    ma_spin_unlock(&(owner->memory_areas_lock));
  }
  marcel_mutex_unlock(&(memory_manager->lock));
  MAMI_ILOG_OUT();
  return err;
}

static
int ma_memory_entity_unattach(mami_manager_t *memory_manager,
                              void *buffer,
                              marcel_entity_t *owner) {
  int err=0;
  mami_data_t *data = NULL;
  void *aligned_buffer;
  mami_data_link_t *area = NULL;

  MAMI_ILOG_IN();
  marcel_mutex_lock(&(memory_manager->lock));
  aligned_buffer = ALIGN_ON_PAGE(memory_manager, buffer, memory_manager->normalpagesize);

  err = ma_memory_locate(memory_manager, memory_manager->root, aligned_buffer, 1, &data);
  if (err >= 0) {
    marcel_entity_t *res;

    mdebug_mami("Removing entity %p from data %p\n", owner, data);

    if (tbx_slist_is_nil(data->owners)) {
      mdebug_mami("The entity %p is not attached to the memory area %p(1).\n", owner, aligned_buffer);
      errno = ENOENT;
      err = -errno;
    }
    else {
      res = tbx_slist_search_and_extract(data->owners, NULL, owner);
      if (res == owner) {
        if (data->node >= 0) {
          mdebug_mami("Removing %lu bits from memnode offset for node #%d\n", (long unsigned)data->size, data->node);
          ((long *) ma_stats_get (owner, ma_stats_memnode_offset))[data->node] -= data->size;
        }

        mdebug_mami("Removing data %p from entity %p\n", data, owner);
	ma_spin_lock(&(owner->memory_areas_lock));
	list_for_each_entry(area, &(owner->memory_areas), list) {
          if (area->data == data) {
            list_del_init(&area->list);
            tfree(area);
            break;
          }
        }
        ma_spin_unlock(&(owner->memory_areas_lock));
      }
      else {
        mdebug_mami("The entity %p is not attached to the memory area %p.\n", owner, aligned_buffer);
        errno = ENOENT;
        err = -errno;
      }
    }
  }

  marcel_mutex_unlock(&(memory_manager->lock));
  if (!area) tfree(area);

  MAMI_ILOG_OUT();
  return err;
}

static
int ma_memory_entity_unattach_all(mami_manager_t *memory_manager,
                                  marcel_entity_t *owner) {
  mami_data_link_t *area, *narea;

  MAMI_ILOG_IN();
  mdebug_mami("Unattaching all memory areas from entity %p\n", owner);
  //ma_spin_lock(&(owner->memory_areas_lock));
  list_for_each_entry_safe(area, narea, &(owner->memory_areas), list) {
    ma_memory_entity_unattach(memory_manager, area->data->startaddress, owner);
  }
  //ma_spin_unlock(&(owner->memory_areas_lock));
  MAMI_ILOG_OUT();
  return 0;
}

static
int ma_memory_entity_migrate_all(mami_manager_t *memory_manager,
                                 marcel_entity_t *owner,
                                 int node) {
  mami_data_link_t *area, *narea;
  list_for_each_entry_safe(area, narea, &(owner->memory_areas), list) {
    ma_memory_migrate_pages(memory_manager, area->data, node);
  }
  return 0;
}

int mami_task_attach(mami_manager_t *memory_manager,
                     void *buffer,
                     size_t size,
                     marcel_t owner,
                     int *node) {
  marcel_entity_t *entity;
  entity = ma_entity_task(owner);
  return ma_memory_entity_attach(memory_manager, buffer, size, entity, node);
}

int mami_task_unattach(mami_manager_t *memory_manager,
                       void *buffer,
                       marcel_t owner) {
  marcel_entity_t *entity;
  entity = ma_entity_task(owner);
  return ma_memory_entity_unattach(memory_manager, buffer, entity);
}

int mami_task_unattach_all(mami_manager_t *memory_manager,
                           marcel_t owner) {
  marcel_entity_t *entity;
  entity = ma_entity_task(owner);
  return ma_memory_entity_unattach_all(memory_manager, entity);
}

int mami_task_migrate_all(mami_manager_t *memory_manager,
                          marcel_t owner,
                          int node) {
  marcel_entity_t *entity;
  entity = ma_entity_task(owner);
  return ma_memory_entity_migrate_all(memory_manager, entity, node);
}

int mami_bubble_attach(mami_manager_t *memory_manager,
                       void *buffer,
                       size_t size,
                       marcel_bubble_t *owner,
                       int *node) {
  marcel_entity_t *entity;
  entity = ma_entity_bubble(owner);
  return ma_memory_entity_attach(memory_manager, buffer, size, entity, node);
}

int mami_bubble_unattach(mami_manager_t *memory_manager,
                         void *buffer,
                         marcel_bubble_t *owner) {
  marcel_entity_t *entity;
  entity = ma_entity_bubble(owner);
  return ma_memory_entity_unattach(memory_manager, buffer, entity);
}

int mami_bubble_unattach_all(mami_manager_t *memory_manager,
                             marcel_bubble_t *owner) {
  marcel_entity_t *entity;
  entity = ma_entity_bubble(owner);
  return ma_memory_entity_unattach_all(memory_manager, entity);
}

int mami_bubble_migrate_all(mami_manager_t *memory_manager,
                            marcel_bubble_t *owner,
                            int node) {
  marcel_entity_t *entity;
  entity = ma_entity_bubble(owner);
  return ma_memory_entity_migrate_all(memory_manager, entity, node);
}

int mami_huge_pages_available(mami_manager_t *memory_manager) {
  return (memory_manager->hugepagesize != 0);
}

int mami_stats(mami_manager_t *memory_manager,
               int node,
               mami_stats_t stat,
               unsigned long *value) {
  int err=0;

  MAMI_LOG_IN();

  if (tbx_unlikely(node >= memory_manager->nb_nodes)) {
    mdebug_mami("Node #%d invalid\n", node);
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
      mdebug_mami("Statistic #%d unknown\n", stat);
      errno = EINVAL;
      err = -errno;
    }
    marcel_mutex_unlock(&(memory_manager->lock));
  }

  MAMI_LOG_OUT();
  return err;
}

int mami_distribute(mami_manager_t *memory_manager,
                    void *buffer,
                    int *nodes,
                    int nb_nodes) {
  mami_data_t *data;
  int err, i, nbpages;

  MAMI_LOG_IN();

  marcel_mutex_lock(&(memory_manager->lock));
  err = ma_memory_locate(memory_manager, memory_manager->root, buffer, 1, &data);

  if (err >= 0) {
    void **pageaddrs;
    int *dests, *status;

    dests = tmalloc(data->nbpages * sizeof(int));
    status = tmalloc(data->nbpages * sizeof(int));
    pageaddrs = tmalloc(data->nbpages * sizeof(void *));
    nbpages=0;
    // Find out which pages have to be moved
    for(i=0 ; i<data->nbpages ; i++) {
      if ((data->node == MULTIPLE_LOCATION_NODE && data->nodes[i] != nodes[i%nb_nodes]) ||
          (data->node != MULTIPLE_LOCATION_NODE && data->node != nodes[i%nb_nodes])) {
        mdebug_mami("Moving page %d from node #%d to node #%d\n", i, data->node != MULTIPLE_LOCATION_NODE ? data->node : data->nodes[i], nodes[i%nb_nodes]);
        pageaddrs[nbpages] = buffer + (i*memory_manager->normalpagesize);
        dests[nbpages] = nodes[i%nb_nodes];
        nbpages ++;
      }
    }

    // If there is some pages to be moved, move them
    if (nbpages != 0) {
      err = ma_memory_move_pages(pageaddrs, nbpages, dests, status, MPOL_MF_MOVE);

      if (err >= 0) {
        // Check the pages have been properly moved and update the data->nodes information
        if (data->nodes == NULL) data->nodes = tmalloc(data->nbpages * sizeof(int));
        data->node = MULTIPLE_LOCATION_NODE;
        err = ma_memory_move_pages(data->pageaddrs, data->nbpages, NULL, status, 0);
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
  MAMI_LOG_OUT();
  return err;
}

int mami_gather(mami_manager_t *memory_manager,
                void *buffer,
                int node) {
  mami_data_t *data;
  int err;

  MAMI_LOG_IN();

  marcel_mutex_lock(&(memory_manager->lock));
  err = ma_memory_locate(memory_manager, memory_manager->root, buffer, 1, &data);

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

      err = ma_memory_move_pages(pageaddrs, nbpages, dests, status, MPOL_MF_MOVE);
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
  MAMI_LOG_OUT();
  return err;
}

#endif /* MM_MAMI_ENABLED */
