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
#include <fcntl.h>
#include <numaif.h>
#include <sys/mman.h>

#ifndef __NR_move_pages

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

static
marcel_memory_manager_t *g_memory_manager = NULL;

/* align a application-given address to the closest page-boundary:
 * re-add the lower bits to increase the bit above pagesize if needed, and truncate
 */
#define ALIGN_ON_PAGE(address, pagesize) (void*)((((uintptr_t) address) + (pagesize >> 1)) & (~(pagesize-1)))

static unsigned long gethugepagesize(void) {
  char line[1024];
  FILE *f;
  unsigned long hugepagesize=0, size=-1;

  mdebug_mami("Reading /proc/meminfo\n");
  f = fopen("/proc/meminfo", "r");
  while (!(feof(f))) {
    fgets(line, 1024, f);
    if (!strncmp(line, "Hugepagesize:", 13)) {
      char *c, *endptr;

      c = strchr(line, ':') + 1;
      size = strtol(c, &endptr, 0);
      hugepagesize = size * 1024;
      mdebug_mami("Huge page size : %lu\n", hugepagesize);
    }
  }
  fclose(f);
  if (size == -1) {
    mdebug_mami("Hugepagesize information not available.");
    return 0;
  }
  return hugepagesize;
}

void marcel_memory_init(marcel_memory_manager_t *memory_manager) {
  int node, dest;

  MAMI_LOG_IN();
  memory_manager->root = NULL;
  marcel_spin_init(&(memory_manager->lock), 0);
  memory_manager->normalpagesize = getpagesize();
  memory_manager->hugepagesize = gethugepagesize();
  memory_manager->initially_preallocated_pages = 1024;
  memory_manager->cache_line_size = 64;
  memory_manager->membind_policy = MARCEL_MEMORY_MEMBIND_POLICY_NONE;
  memory_manager->nb_nodes = marcel_nbnodes;

  // Preallocate memory on each node
  memory_manager->heaps = tmalloc((memory_manager->nb_nodes+1) * sizeof(marcel_memory_area_t *));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    ma_memory_preallocate(memory_manager, &(memory_manager->heaps[node]), memory_manager->initially_preallocated_pages, node);
    mdebug_mami("Preallocating %p for node #%d\n", memory_manager->heaps[node]->start, node);
  }
  ma_memory_preallocate(memory_manager, &(memory_manager->heaps[memory_manager->nb_nodes]), memory_manager->initially_preallocated_pages, -1);
  mdebug_mami("Preallocating %p for anonymous heap\n", memory_manager->heaps[memory_manager->nb_nodes]->start);

  // Initialise space with huge pages on each node
  memory_manager->huge_pages_heaps = tmalloc(memory_manager->nb_nodes * sizeof(marcel_memory_huge_pages_area_t *));
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
  memory_manager->writing_access_costs = tmalloc(memory_manager->nb_nodes * sizeof(marcel_access_cost_t *));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    memory_manager->writing_access_costs[node] = tmalloc(memory_manager->nb_nodes * sizeof(marcel_access_cost_t));
    for(dest=0 ; dest<memory_manager->nb_nodes ; dest++) {
      memory_manager->writing_access_costs[node][dest].cost = 0;
    }
  }
  memory_manager->reading_access_costs = tmalloc(memory_manager->nb_nodes * sizeof(marcel_access_cost_t *));
  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    memory_manager->reading_access_costs[node] = tmalloc(memory_manager->nb_nodes * sizeof(marcel_access_cost_t));
    for(dest=0 ; dest<memory_manager->nb_nodes ; dest++) {
      memory_manager->reading_access_costs[node][dest].cost = 0;
    }
  }
  ma_memory_load_model_for_memory_access(memory_manager);

#ifdef PM2DEBUG
  if (marcel_mami_debug.show > PM2DEBUG_STDLEVEL) {
    for(node=0 ; node<memory_manager->nb_nodes ; node++) {
      for(dest=0 ; dest<memory_manager->nb_nodes ; dest++) {
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

    for(node=0 ; node<memory_manager->nb_nodes ; node++) {
      for(dest=0 ; dest<memory_manager->nb_nodes ; dest++) {
        marcel_access_cost_t wcost = memory_manager->writing_access_costs[node][dest];
        marcel_access_cost_t rcost = memory_manager->writing_access_costs[node][dest];
        marcel_printf("[%d:%d] %f %f\n", node, dest, wcost.cost, rcost.cost);
      }
    }
  }
#endif /* PM2DEBUG */

  MAMI_LOG_OUT();
}

static
void ma_memory_sampling_free(p_tbx_slist_t migration_costs) {
  MAMI_LOG_IN();
  while (!tbx_slist_is_nil(migration_costs)) {
    marcel_memory_migration_cost_t *ptr = tbx_slist_extract(migration_costs);
    tfree(ptr);
  }
  tbx_slist_free(migration_costs);
  MAMI_LOG_OUT();
}

static
int ma_memory_deallocate_huge_pages(marcel_memory_manager_t *memory_manager, marcel_memory_huge_pages_area_t **space, int node) {
  int err;

  MAMI_LOG_IN();
  mdebug_mami("Releasing huge pages %s\n", (*space)->filename);

  err = close((*space)->file);
  if (err < 0) {
    perror("close");
    return -errno;
  }
  err = unlink((*space)->filename);
  if (err < 0) {
    perror("unlink");
    return -errno;
  }

  MAMI_LOG_OUT();
  return 0;
}

static
void ma_memory_deallocate(marcel_memory_manager_t *memory_manager, marcel_memory_area_t **space, int node) {
  marcel_memory_area_t *ptr, *ptr2;

  MAMI_LOG_IN();
  mdebug_mami("Deallocating memory for node %d\n", node);
  ptr  = (*space);
  while (ptr != NULL) {
    mdebug_mami("Unmapping memory area from %p\n", ptr->start);
    munmap(ptr->start, ptr->nbpages * ptr->pagesize);
    ptr2 = ptr;
    ptr = ptr->next;
    tfree(ptr2);
  }
  MAMI_LOG_OUT();
}

static
void ma_memory_clean_memory(marcel_memory_manager_t *memory_manager) {
  while (memory_manager->root) {
    marcel_memory_free(memory_manager, memory_manager->root->data->startaddress);
  }
}

void marcel_memory_exit(marcel_memory_manager_t *memory_manager) {
  int node, dest;

  MAMI_LOG_IN();

  if (memory_manager->root) {
    marcel_fprintf(stderr, "MaMI Warning: some memory areas have not been free-d\n");
#ifdef PM2DEBUG
    if (marcel_mami_debug.show > PM2DEBUG_STDLEVEL) {
      marcel_memory_fprint(stderr, memory_manager);
    }
#endif /* PM2DEBUG */
    ma_memory_clean_memory(memory_manager);
  }

  for(node=0 ; node<memory_manager->nb_nodes ; node++) {
    ma_memory_deallocate(memory_manager, &(memory_manager->heaps[node]), node);
  }
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
    tfree(memory_manager->reading_access_costs[node]);
    tfree(memory_manager->writing_access_costs[node]);
  }
  tfree(memory_manager->reading_access_costs);
  tfree(memory_manager->writing_access_costs);

  MAMI_LOG_OUT();
}

static
void ma_memory_init_memory_data(marcel_memory_manager_t *memory_manager,
                                void **pageaddrs, int nbpages, size_t size, int node, int protection, int with_huge_pages,
				int mami_allocated,
                                marcel_memory_data_t **memory_data) {
  MAMI_LOG_IN();

  *memory_data = tmalloc(sizeof(marcel_memory_data_t));

  // Set the interval addresses and the length
  (*memory_data)->startaddress = pageaddrs[0];
  (*memory_data)->endaddress = pageaddrs[0]+size;
  (*memory_data)->size = size;
  (*memory_data)->status = MARCEL_MEMORY_INITIAL_STATUS;
  (*memory_data)->protection = protection;
  (*memory_data)->with_huge_pages = with_huge_pages;
  (*memory_data)->mami_allocated = mami_allocated;
  (*memory_data)->owners = tbx_slist_nil();
  (*memory_data)->next = NULL;
  INIT_LIST_HEAD(&((*memory_data)->list));

  if (node == memory_manager->nb_nodes) {
    (*memory_data)->node = -1;
  }
  else {
    (*memory_data)->node = node;
  }

  // Set the page addresses
  (*memory_data)->nbpages = nbpages;
  (*memory_data)->pageaddrs = tmalloc((*memory_data)->nbpages * sizeof(void *));
  memcpy((*memory_data)->pageaddrs, pageaddrs, nbpages*sizeof(void*));

  MAMI_LOG_OUT();
}

static
void ma_memory_clean_memory_data(marcel_memory_data_t **memory_data) {
  if (!(tbx_slist_is_nil((*memory_data)->owners))) {
    marcel_fprintf(stderr, "MaMI Warning: some threads are still attached to the memory area [%p:%p]\n",
                   (*memory_data)->startaddress, (*memory_data)->endaddress);
    tbx_slist_clear((*memory_data)->owners);
  }
  tbx_slist_free((*memory_data)->owners);
  free((*memory_data)->pageaddrs);
  free(*memory_data);
}

void ma_memory_delete_tree(marcel_memory_manager_t *memory_manager, marcel_memory_tree_t **memory_tree) {
  MAMI_LOG_IN();
  if ((*memory_tree)->leftchild == NULL) {
    marcel_memory_tree_t *temp = (*memory_tree);
    ma_memory_clean_memory_data(&(temp->data));
    (*memory_tree) = (*memory_tree)->rightchild;
    free(temp);
  }
  else if ((*memory_tree)->rightchild == NULL) {
    marcel_memory_tree_t *temp = *memory_tree;
    ma_memory_clean_memory_data(&(temp->data));
    (*memory_tree) = (*memory_tree)->leftchild;
    free(temp);
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
    ma_memory_clean_memory_data(&((*memory_tree)->data));
    ma_memory_init_memory_data(memory_manager, temp->data->pageaddrs, temp->data->nbpages, temp->data->size, temp->data->node,
                               temp->data->protection, temp->data->with_huge_pages, temp->data->mami_allocated, &((*memory_tree)->data));

    // then delete the predecessor
    ma_memory_unregister(memory_manager, &((*memory_tree)->leftchild), temp->data->pageaddrs[0]);
  }
  MAMI_LOG_OUT();
}

static
void ma_memory_free_from_node(marcel_memory_manager_t *memory_manager, void *buffer, size_t size, int nbpages, int node, int protection, int with_huge_pages) {
  marcel_memory_area_t *available;
  marcel_memory_area_t **ptr;
  marcel_memory_area_t **prev;

  MAMI_LOG_IN();

  mdebug_mami("Freeing space from %p with %d pages\n", buffer, nbpages);

  available = tmalloc(sizeof(marcel_memory_area_t));
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
    if (((*ptr)->end == (*ptr)->next->start) &&
        ((*ptr)->protection == (*ptr)->next->protection)) {
      mdebug_mami("[%p:%p:%d] and [%p:%p:%d] can be defragmented\n", (*ptr)->start, (*ptr)->end, (*ptr)->protection,
                  (*ptr)->next->start, (*ptr)->next->end, (*ptr)->next->protection);
      (*ptr)->end = (*ptr)->next->end;
      (*ptr)->nbpages += (*ptr)->next->nbpages;
      (*ptr)->next = (*ptr)->next->next;
    }
    else
      ptr = &((*ptr)->next);
  }

  MAMI_LOG_OUT();
}

void ma_memory_unregister(marcel_memory_manager_t *memory_manager, marcel_memory_tree_t **memory_tree, void *buffer) {
  MAMI_LOG_IN();
  if (*memory_tree!=NULL) {
    if (buffer == (*memory_tree)->data->pageaddrs[0]) {
      marcel_memory_data_t *data = (*memory_tree)->data;
      marcel_memory_data_t *next_data = data->next;

      if (data->mami_allocated) {
	mdebug_mami("Removing [%p, %p]\n", (*memory_tree)->data->pageaddrs[0], (*memory_tree)->data->pageaddrs[0]+(*memory_tree)->data->size);
	VALGRIND_MAKE_MEM_NOACCESS((*memory_tree)->data->pageaddrs[0], (*memory_tree)->data->size);
	// Free memory
	ma_memory_free_from_node(memory_manager, buffer, data->size, data->nbpages, data->node, data->protection, data->with_huge_pages);
      }
      else {
	mdebug_mami("Address %p is not allocated by MAMI.\n", buffer);
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
  MAMI_LOG_OUT();
}

static
void ma_memory_register_pages(marcel_memory_manager_t *memory_manager, marcel_memory_tree_t **memory_tree,
                              void **pageaddrs, int nbpages, size_t size, int node, int protection, int with_huge_pages, int mami_allocated,
                              marcel_memory_data_t **data) {
  MAMI_LOG_IN();
  if (*memory_tree==NULL) {
    *memory_tree = tmalloc(sizeof(marcel_memory_tree_t));
    (*memory_tree)->leftchild = NULL;
    (*memory_tree)->rightchild = NULL;
    ma_memory_init_memory_data(memory_manager, pageaddrs, nbpages, size, node, protection, with_huge_pages, mami_allocated, &((*memory_tree)->data));
    if (data) *data = (*memory_tree)->data;
  }
  else {
    if (pageaddrs[0] < (*memory_tree)->data->pageaddrs[0])
      ma_memory_register_pages(memory_manager, &((*memory_tree)->leftchild), pageaddrs, nbpages, size, node, protection, with_huge_pages, mami_allocated, data);
    else
      ma_memory_register_pages(memory_manager, &((*memory_tree)->rightchild), pageaddrs, nbpages, size, node, protection, with_huge_pages, mami_allocated, data);
  }
  MAMI_LOG_OUT();
}

int ma_memory_preallocate_huge_pages(marcel_memory_manager_t *memory_manager, marcel_memory_huge_pages_area_t **space, int node) {
  int nbpages;
  pid_t pid;

  MAMI_LOG_IN();
  nbpages = marcel_topo_node_level[node].huge_page_free;
  if (!nbpages) {
    mdebug_mami("No huge pages on node #%d\n", node);
    *space = NULL;
    return -1;
  }

  (*space) = tmalloc(sizeof(marcel_memory_huge_pages_area_t));
  (*space)->size = nbpages * memory_manager->hugepagesize;
  (*space)->filename = malloc(1024 * sizeof(char));
  pid = getpid();
  sprintf((*space)->filename, "/hugetlbfs/mami_pid_%d_node_%d", pid, node);

  (*space)->file = open((*space)->filename, O_CREAT|O_RDWR, S_IRWXU);
  if ((*space)->file == -1) {
    tfree(*space);
    *space = NULL;
    perror("open");
    return -errno;
  }

  (*space)->buffer = mmap(NULL, (*space)->size, PROT_READ|PROT_WRITE, MAP_PRIVATE, (*space)->file, 0);
  if ((*space)->buffer == MAP_FAILED) {
    perror("mmap");
    tfree(*space);
    *space = NULL;
    return -errno;
  }
  /* mark the memory as unaccessible until it gets allocated to the application */
  VALGRIND_MAKE_MEM_NOACCESS((*space)->buffer, (*space)->size);

  (*space)->heap = tmalloc(sizeof(marcel_memory_area_t));
  (*space)->heap->start = (*space)->buffer;
  (*space)->heap->end = (*space)->buffer + (*space)->size;
  (*space)->heap->nbpages = nbpages;
  (*space)->heap->protection = PROT_READ|PROT_WRITE;
  (*space)->heap->pagesize = memory_manager->hugepagesize;
  (*space)->heap->next = NULL;

  MAMI_LOG_OUT();
  return 0;
}

int ma_memory_preallocate(marcel_memory_manager_t *memory_manager, marcel_memory_area_t **space, int nbpages, int node) {
  unsigned long nodemask;
  size_t length;
  void *buffer;
  int i, err=0;

  MAMI_LOG_IN();

  length = nbpages * memory_manager->normalpagesize;

  buffer = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  if (buffer == MAP_FAILED) {
    perror("mmap");
    err = -errno;
  }
  else {
    if (node != -1) {
      nodemask = (1<<node);
      err = mbind(buffer, length, MPOL_BIND, &nodemask, memory_manager->nb_nodes+2, MPOL_MF_MOVE);
      if (err < 0) {
        perror("mbind");
        err = 0;
      }

      for(i=0 ; i<nbpages ; i++) {
        int *ptr = buffer+i*memory_manager->normalpagesize;
        *ptr = 0;
      }
    }
    /* mark the memory as unaccessible until it gets allocated to the application */
    VALGRIND_MAKE_MEM_NOACCESS(buffer, length);

    (*space) = tmalloc(sizeof(marcel_memory_area_t));
    (*space)->start = buffer;
    (*space)->end = buffer + length;
    (*space)->nbpages = nbpages;
    (*space)->protection = PROT_READ|PROT_WRITE;
    (*space)->pagesize = memory_manager->normalpagesize;
    (*space)->next = NULL;
  }

  MAMI_LOG_OUT();
  return err;
}

static void* ma_memory_get_buffer_from_huge_pages_heap(marcel_memory_manager_t *memory_manager, int node, int nbpages, int size, int *protection) {
  marcel_memory_huge_pages_area_t *heap = memory_manager->huge_pages_heaps[node];
  marcel_memory_area_t *hheap = NULL, *prev = NULL;
  void *buffer = NULL;
  unsigned long nodemask;
  int err;

  if (memory_manager->huge_pages_heaps[node] == NULL) {
    int err = ma_memory_preallocate_huge_pages(memory_manager, &(memory_manager->huge_pages_heaps[node]), node);
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
  err = set_mempolicy(MPOL_BIND, &nodemask, memory_manager->nb_nodes+2);
  if (err < 0) {
    perror("set_mempolicy");
    return NULL;
  }
  memset(buffer, 0, size);
  err = set_mempolicy(MPOL_DEFAULT, NULL, 0);
  if (err < 0) {
    perror("set_mempolicy");
    return NULL;
  }

  return buffer;
}

static void* ma_memory_get_buffer_from_heap(marcel_memory_manager_t *memory_manager, int node, int nbpages, int size, int *protection) {
  marcel_memory_area_t *heap = memory_manager->heaps[node];
  marcel_memory_area_t *prev = NULL;
  void *buffer;

  mdebug_mami("Requiring space of %d pages from heap %p on node %d\n", nbpages, heap, node);

  // Look for a space big enough
  prev = heap;
  while (heap != NULL) {
    mdebug_mami("Current space from %p with %d pages\n", heap->start, heap->nbpages);
    if (heap->nbpages >= nbpages)
      break;
    prev = heap;
    heap = heap->next;
  }
  if (heap == NULL) {
    int err, preallocatedpages;

    preallocatedpages = memory_manager->initially_preallocated_pages;
    while (nbpages > preallocatedpages) preallocatedpages *= 2;
    mdebug_mami("not enough space, let's allocate %d extra pages\n", preallocatedpages);
    err = ma_memory_preallocate(memory_manager, &heap, preallocatedpages, node);
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
  return buffer;
}

static
void* ma_memory_allocate_on_node(marcel_memory_manager_t *memory_manager, size_t size, unsigned long pagesize, int node, int with_huge_pages) {
  void *buffer;
  int i, nbpages, protection=0;
  size_t realsize;
  void **pageaddrs;

  MAMI_LOG_IN();

  marcel_spin_lock(&(memory_manager->lock));

  if (!size) {
    marcel_spin_unlock(&(memory_manager->lock));
    MAMI_LOG_OUT();
    return NULL;
  }

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
    mdebug_mami("Registering [%p, %p]\n", pageaddrs[0], pageaddrs[0]+size);
    ma_memory_register_pages(memory_manager, &(memory_manager->root), pageaddrs, nbpages, size, node, protection, with_huge_pages, 1, NULL);

    tfree(pageaddrs);
    VALGRIND_MAKE_MEM_UNDEFINED(buffer, size);
  }

  marcel_spin_unlock(&(memory_manager->lock));
  mdebug_mami("Allocating %p on node #%d\n", buffer, node);
  MAMI_LOG_OUT();
  return buffer;
}

void* marcel_memory_allocate_on_node(marcel_memory_manager_t *memory_manager, size_t size, int node) {
  int with_huge_pages;

  if (tbx_unlikely(node >= memory_manager->nb_nodes)) {
    mdebug_mami("Node #%d invalid\n", node);
    return NULL;
  }

  with_huge_pages = (memory_manager->membind_policy == MARCEL_MEMORY_MEMBIND_POLICY_HUGE_PAGES);
  return ma_memory_allocate_on_node(memory_manager, size, memory_manager->normalpagesize, node, with_huge_pages);
}

void* marcel_memory_malloc(marcel_memory_manager_t *memory_manager, size_t size) {
  int node;
  void *ptr;
  int with_huge_pages;
  unsigned long pagesize;

  MAMI_LOG_IN();

  pagesize = memory_manager->normalpagesize;
  with_huge_pages = 0;
  node = 0;
  if (memory_manager->membind_policy == MARCEL_MEMORY_MEMBIND_POLICY_NONE) {
    node = marcel_current_node();
    if (tbx_unlikely(node == -1)) node=0;
  }
  else if (memory_manager->membind_policy == MARCEL_MEMORY_MEMBIND_POLICY_SPECIFIC_NODE) {
    node = memory_manager->membind_node;
  }
  else if (memory_manager->membind_policy == MARCEL_MEMORY_MEMBIND_POLICY_LEAST_LOADED_NODE) {
    marcel_memory_select_node(memory_manager, MARCEL_MEMORY_LEAST_LOADED_NODE, &node);
  }
  else if (memory_manager->membind_policy == MARCEL_MEMORY_MEMBIND_POLICY_HUGE_PAGES) {
    node = memory_manager->membind_node;
    pagesize = memory_manager->hugepagesize;
    with_huge_pages = 1;
  }
  else if (memory_manager->membind_policy == MARCEL_MEMORY_MEMBIND_POLICY_FIRST_TOUCH) {
    node = memory_manager->nb_nodes;
  }

  ptr = ma_memory_allocate_on_node(memory_manager, size, pagesize, node, with_huge_pages);

  MAMI_LOG_OUT();
  return ptr;
}

void* marcel_memory_calloc(marcel_memory_manager_t *memory_manager, size_t nmemb, size_t size) {
  void *ptr;

  MAMI_LOG_IN();

  ptr = marcel_memory_malloc(memory_manager, nmemb*size);
  ptr = memset(ptr, 0, nmemb*size);

  MAMI_LOG_OUT();
  return ptr;
}

static
int ma_memory_locate(marcel_memory_manager_t *memory_manager, marcel_memory_tree_t *memory_tree, void *buffer, size_t size, int *node, marcel_memory_data_t **data) {
 MAMI_LOG_IN();
  if (memory_tree==NULL) {
    mdebug_mami("The interval [%p:%p] is not managed by MAMI.\n", buffer, buffer+size);
    errno = EINVAL;
    return -errno;
  }
  else if (buffer >= memory_tree->data->startaddress && buffer+size <= memory_tree->data->endaddress) {
    // the address is stored on the current memory_data
    mdebug_mami("Found interval [%p:%p] in [%p:%p]\n", buffer, buffer+size, memory_tree->data->startaddress, memory_tree->data->endaddress);
    *node = memory_tree->data->node;
    mdebug_mami("Interval [%p:%p] is located on node %d\n", buffer, buffer+size, *node);
    if (data) *data = memory_tree->data;
    return 0;
  }
  else if (buffer <= memory_tree->data->startaddress) {
    return ma_memory_locate(memory_manager, memory_tree->leftchild, buffer, size, node, data);
  }
  else if (buffer >= memory_tree->data->endaddress) {
    return ma_memory_locate(memory_manager, memory_tree->rightchild, buffer, size, node, data);
  }
  else {
    errno = EINVAL;
    return -errno;
  }
  MAMI_LOG_OUT();
}

static
int ma_memory_get_pages_location(void **pageaddrs, int nbpages, int *node) {
  int nbpages_query;
  int statuses[nbpages];
  int err=0;

#warning todo: check location for all pages and what if pages are on different nodes
  if (nbpages == 1) nbpages_query = 1; else nbpages_query = 2;
  ma_memory_move_pages(pageaddrs, nbpages_query, NULL, statuses);
  if (statuses[0] == -ENOENT) {
    mdebug_mami("Could not locate pages\n");
    *node = -1;
    err = ENOENT;
  }
  else {
    if (nbpages_query == 2) {
      if (statuses[0] != statuses[1]) {
        marcel_fprintf(stderr, "MaMI Warning: Memory located on different nodes (%d != %d)\n", statuses[0], statuses[1]);
      }
      *node = statuses[1];
    }
    else {
      *node = statuses[0];
    }
  }
  return err;
}

void ma_memory_register(marcel_memory_manager_t *memory_manager,
                        void *buffer,
                        size_t size,
                        int mami_allocated,
                        marcel_memory_data_t **data) {
  void **pageaddrs;
  int nbpages, protection, with_huge_pages;
  int i, node;

  mdebug_mami("Registering address interval [%p:%p]\n", buffer, buffer+size);

  with_huge_pages = 0;
  protection = PROT_READ|PROT_WRITE;

  // Count the number of pages
  nbpages = size / memory_manager->normalpagesize;
  if (nbpages * memory_manager->normalpagesize != size) nbpages++;

  // Set the page addresses
  pageaddrs = malloc(nbpages * sizeof(void *));
  for(i=0; i<nbpages ; i++) pageaddrs[i] = buffer + i*memory_manager->normalpagesize;

  // Find out where the pages are
  ma_memory_get_pages_location(pageaddrs, nbpages, &node);

  ma_memory_register_pages(memory_manager, &(memory_manager->root), pageaddrs, nbpages, size, node, protection, with_huge_pages, mami_allocated, data);
}

int marcel_memory_register(marcel_memory_manager_t *memory_manager,
			   void *buffer,
			   size_t size) {
#warning todo vérifier que buffer et size sont alignés sur une page

  MAMI_LOG_IN();
  marcel_spin_lock(&(memory_manager->lock));
  ma_memory_register(memory_manager, buffer, size, 0, NULL);
  marcel_spin_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
  return 0;
}

int marcel_memory_unregister(marcel_memory_manager_t *memory_manager,
			     void *buffer) {
  marcel_memory_data_t *data = NULL;
  int err=0;
  int source;
  void *aligned_buffer;

  MAMI_LOG_IN();
  marcel_spin_lock(&(memory_manager->lock));

  aligned_buffer = ALIGN_ON_PAGE(buffer, memory_manager->normalpagesize);
  err = ma_memory_locate(memory_manager, memory_manager->root, buffer, 1, &source, &data);
  if (err >= 0) {
    mdebug_mami("Unregistering [%p:%p]\n", buffer,buffer+data->size);
    ma_memory_unregister(memory_manager, &(memory_manager->root), aligned_buffer);
  }
  marcel_spin_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
  return err;
}

int marcel_memory_split(marcel_memory_manager_t *memory_manager,
                        void *buffer,
                        unsigned int subareas,
                        void **newbuffers) {
  marcel_memory_data_t *data = NULL;
  int err=0;
  int source;

#warning todo vérifier que subareas donne des sous-buffers alignés sur une page

  MAMI_LOG_IN();
  marcel_spin_lock(&(memory_manager->lock));

  err = ma_memory_locate(memory_manager, memory_manager->root, buffer, 1, &source, &data);
  if (err >= 0) {
    size_t subsize;
    int subpages, i;
    void **pageaddrs;

    subpages = data->nbpages / subareas;
    if (!subpages) {
      mdebug_mami("Memory area from %p not big enough to be split in %d subareas\n", data->startaddress, subareas);
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
	ma_memory_register_pages(memory_manager, &(memory_manager->root), pageaddrs, subpages, subsize, data->node, data->protection,
                                 data->with_huge_pages, data->mami_allocated, NULL);
	pageaddrs += subpages;
      }

      // Modify initial memory area
      newbuffers[0] = buffer;
      data->nbpages = subpages;
      data->endaddress = data->startaddress + subsize;
      data->size = subsize;
    }
  }
  marcel_spin_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
  return err;
}

int marcel_memory_membind(marcel_memory_manager_t *memory_manager,
                          marcel_memory_membind_policy_t policy,
                          int node) {
  int err=0;

  MAMI_LOG_IN();
  if (tbx_unlikely(node >= memory_manager->nb_nodes)) {
    mdebug_mami("Node #%d invalid\n", node);
    errno = EINVAL;
    err = -errno;
  }
  else if (policy == MARCEL_MEMORY_MEMBIND_POLICY_HUGE_PAGES && memory_manager->hugepagesize == 0) {
    mdebug("Huge pages are not available. Cannot set memory policy.\n");
    errno = EINVAL;
    err = -errno;
  }
  else {
    mdebug_mami("Set the current membind policy to %d (node %d)\n", policy, node);
    memory_manager->membind_policy = policy;
    memory_manager->membind_node = node;
  }
  MAMI_LOG_OUT();
  return -err;
}

void marcel_memory_free(marcel_memory_manager_t *memory_manager, void *buffer) {
  MAMI_LOG_IN();
  mdebug_mami("Freeing [%p]\n", buffer);
  marcel_spin_lock(&(memory_manager->lock));
  ma_memory_unregister(memory_manager, &(memory_manager->root), buffer);
  marcel_spin_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
}

int ma_memory_move_pages(void **pageaddrs, int pages, int *nodes, int *status) {
  int err=0;

  if (nodes) mdebug_mami("binding on numa node #%d\n", nodes[0]);

#if defined (X86_64_ARCH) && defined (X86_ARCH)
  err = syscall6(__NR_move_pages, 0, pages, pageaddrs, nodes, status, MPOL_MF_MOVE);
#else
  err = syscall(__NR_move_pages, 0, pages, pageaddrs, nodes, status, MPOL_MF_MOVE);
#endif
  if (err < 0) perror("move_pages (set_bind)");
  return err;
}

int ma_memory_check_pages_location(void **pageaddrs, int pages, int node) {
  int *pagenodes;
  int i;
  int err=0;

  mdebug_mami("check location is #%d\n", node);

  pagenodes = tmalloc(pages * sizeof(int));
  err = move_pages(0, pages, pageaddrs, NULL, pagenodes, 0);
  if (err < 0) perror("move_pages (check_pages_location)");

  for(i=0; i<pages; i++) {
    if (pagenodes[i] != node) {
      marcel_printf("MaMI Warning: page #%d is not located on node #%d\n", i, node);
      exit(-1);
    }
  }
  tfree(pagenodes);
  return -err;
}

int marcel_memory_locate(marcel_memory_manager_t *memory_manager, void *buffer, size_t size, int *node) {
  marcel_memory_data_t *data = NULL;
  mdebug_mami("Trying to locate address %p\n", buffer);
  if (!size) size=1;
  return ma_memory_locate(memory_manager, memory_manager->root, buffer, size, node, &data);
}

static
void ma_memory_print(FILE *stream, marcel_memory_tree_t *memory_tree, int indent) {
  if (memory_tree) {
    int x;
    ma_memory_print(stream, memory_tree->leftchild, indent+2);
    for(x=0 ; x<indent ; x++) fprintf(stream, " ");
    fprintf(stream, "[%p, %p]\n", memory_tree->data->startaddress, memory_tree->data->endaddress);
    ma_memory_print(stream, memory_tree->rightchild, indent+2);
  }
}

void marcel_memory_print(marcel_memory_manager_t *memory_manager) {
  MAMI_LOG_IN();
  mdebug_mami("******************** TREE BEGIN *********************************\n");
  ma_memory_print(stdout, memory_manager->root, 0);
  mdebug_mami("******************** TREE END *********************************\n");
  MAMI_LOG_OUT();
}

void marcel_memory_fprint(FILE *stream, marcel_memory_manager_t *memory_manager) {
  LOG_IN();
  mdebug_mami("******************** TREE BEGIN *********************************\n");
  ma_memory_print(stream, memory_manager->root, 0);
  mdebug_mami("******************** TREE END *********************************\n");
  LOG_OUT();
}

void marcel_memory_migration_cost(marcel_memory_manager_t *memory_manager,
                                  int source,
                                  int dest,
                                  size_t size,
                                  float *cost) {
  p_tbx_slist_t migration_costs;

  MAMI_LOG_IN();
  *cost = -1;
  if (tbx_unlikely(source >= memory_manager->nb_nodes || dest >= memory_manager->nb_nodes)) {
    mdebug_mami("Invalid node id\n");
  }
  else {
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
  }
  MAMI_LOG_OUT();
}

void marcel_memory_writing_access_cost(marcel_memory_manager_t *memory_manager,
                                       int source,
                                       int dest,
                                       size_t size,
                                       float *cost) {
  marcel_access_cost_t access_cost;
  MAMI_LOG_IN();
  if (tbx_unlikely(source >= memory_manager->nb_nodes || dest >= memory_manager->nb_nodes)) {
    mdebug_mami("Invalid node id\n");
    *cost = -1;
  }
  else {
    access_cost = memory_manager->writing_access_costs[source][dest];
    *cost = (size/memory_manager->cache_line_size) * access_cost.cost;
  }
  MAMI_LOG_OUT();
}

void marcel_memory_reading_access_cost(marcel_memory_manager_t *memory_manager,
                                       int source,
                                       int dest,
                                       size_t size,
                                       float *cost) {
  marcel_access_cost_t access_cost;
  MAMI_LOG_IN();
  if (tbx_unlikely(source >= memory_manager->nb_nodes || dest >= memory_manager->nb_nodes)) {
    mdebug_mami("Invalid node id\n");
    *cost = -1;
  }
  else {
    access_cost = memory_manager->reading_access_costs[source][dest];
    *cost = ((float)size/(float)memory_manager->cache_line_size) * access_cost.cost;
  }
  MAMI_LOG_OUT();
}

static
void ma_memory_get_free_space(marcel_memory_manager_t *memory_manager,
                              int node,
                              int *space) {
  marcel_memory_area_t *heap = memory_manager->heaps[node];
  MAMI_LOG_IN();
  *space = 0;
  while (heap != NULL) {
    *space += heap->nbpages;
    heap = heap->next;
  }
  MAMI_LOG_OUT();
}

void marcel_memory_select_node(marcel_memory_manager_t *memory_manager,
                               marcel_memory_node_selection_policy_t policy,
                               int *node) {
  MAMI_LOG_IN();
  marcel_spin_lock(&(memory_manager->lock));

  if (policy == MARCEL_MEMORY_LEAST_LOADED_NODE) {
    int i, space, maxspace;
    maxspace = 0;
    for(i=0 ; i<memory_manager->nb_nodes ; i++) {
      ma_memory_get_free_space(memory_manager, i, &space);
      if (space > maxspace) {
        maxspace = space;
        *node = i;
      }
    }
  }

  marcel_spin_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
}

static
int ma_memory_migrate_pages(marcel_memory_manager_t *memory_manager,
                            void *buffer, marcel_memory_data_t *data, int source, int dest) {
  int i, *dests, *status;
  int err=0;

  MAMI_LOG_IN();
  if (source == -1) {
    mdebug_mami("The address %p is not managed by MAMI.\n", buffer);
    err = EINVAL;
  }
  else if (source == dest) {
    mdebug_mami("The address %p is already located at the required node.\n", buffer);
    err = EALREADY;
  }
  else {
    mdebug_mami("Migrating %d page(s) to node #%d\n", data->nbpages, dest);
    dests = tmalloc(data->nbpages * sizeof(int));
    status = tmalloc(data->nbpages * sizeof(int));
    for(i=0 ; i<data->nbpages ; i++) dests[i] = dest;
    err = ma_memory_move_pages(data->pageaddrs, data->nbpages, dests, status);
#ifdef PM2DEBUG
    ma_memory_check_pages_location(data->pageaddrs, data->nbpages, dest);
#endif /* PM2DEBUG */
    if (!tbx_slist_is_nil(data->owners)) {
          tbx_slist_ref_to_head(data->owners);
          do {
            marcel_entity_t *object = NULL;
            object = tbx_slist_ref_get(data->owners);

            ((long *) ma_stats_get (object, ma_stats_memnode_offset))[data->node] -= data->size;
            ((long *) ma_stats_get (object, ma_stats_memnode_offset))[dest] += data->size;
          } while (tbx_slist_ref_forward(data->owners));
    }
    data->node = dest;
  }

  MAMI_LOG_OUT();
  errno = err;
  return -err;
}

int marcel_memory_migrate_pages(marcel_memory_manager_t *memory_manager,
                                void *buffer, int dest) {
  int source, ret;
  marcel_memory_data_t *data = NULL;

  MAMI_LOG_IN();
  marcel_spin_lock(&(memory_manager->lock));
  ret = ma_memory_locate(memory_manager, memory_manager->root, buffer, 1, &source, &data);
  if (ret >= 0) {
    ret = ma_memory_migrate_pages(memory_manager, buffer, data, source, dest);
  }
  marcel_spin_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
  return ret;
}

static
void ma_memory_segv_handler(int sig, siginfo_t *info, void *_context) {
  ucontext_t *context = _context;
  void *addr;
  int err, source=0, dest;
  marcel_memory_data_t *data = NULL;

  marcel_spin_lock(&(g_memory_manager->lock));

#ifdef __x86_64__
  addr = (void *)(context->uc_mcontext.gregs[REG_CR2]);
#elif __i386__
  addr = (void *)(context->uc_mcontext.cr2);
#else
#error Unsupported architecture
#endif

  err = ma_memory_locate(g_memory_manager, g_memory_manager->root, addr, 1, &source, &data);
  if (err < 0) {
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
  marcel_spin_unlock(&(g_memory_manager->lock));
}

int marcel_memory_migrate_on_next_touch(marcel_memory_manager_t *memory_manager, void *buffer) {
  int err=0, source;
  static int handler_set = 0;
  marcel_memory_data_t *data = NULL;

  marcel_spin_lock(&(memory_manager->lock));
  MAMI_LOG_IN();

  if (!handler_set) {
    struct sigaction act;
    handler_set = 1;
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = ma_memory_segv_handler;
    err = sigaction(SIGSEGV, &act, NULL);
    if (err < 0) {
      perror("sigaction");
      marcel_spin_unlock(&(memory_manager->lock));
      MAMI_LOG_OUT();
      return -errno;
    }
  }

  g_memory_manager = memory_manager;
  err = ma_memory_locate(memory_manager, memory_manager->root, buffer, 1, &source, &data);
  if (err >= 0) {
    data->status = MARCEL_MEMORY_INITIAL_STATUS;
    err = mprotect(data->startaddress, data->size, PROT_NONE);
    if (err < 0) {
      perror("mprotect");
    }
  }
  marcel_spin_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
  return err;
}

static
int ma_memory_entity_attach(marcel_memory_manager_t *memory_manager,
                            void *buffer,
                            size_t size,
                            marcel_entity_t *owner) {
  int err=0, source;
  marcel_memory_data_t *data;

  marcel_spin_lock(&(memory_manager->lock));
  MAMI_LOG_IN();

  mdebug_mami("Attaching [%p:%p] to entity %p\n", buffer, buffer+size, owner);

  if (!buffer) {
    mdebug_mami("Cannot attach NULL buffer\n");
    errno = EINVAL;
    err = -errno;
  }
  else {
    void *aligned_buffer = ALIGN_ON_PAGE(buffer, memory_manager->normalpagesize);
    void *aligned_endbuffer = ALIGN_ON_PAGE(buffer+size, memory_manager->normalpagesize);
    size_t aligned_size = aligned_endbuffer-aligned_buffer;

    if (!aligned_size) {
      aligned_endbuffer = aligned_buffer + memory_manager->normalpagesize;
      aligned_size = memory_manager->normalpagesize;
    }

    err = ma_memory_locate(memory_manager, memory_manager->root, buffer, size, &source, &data);
    if (err < 0) {
      mdebug_mami("The address interval [%p:%p] is not managed by MAMI. Let's register it\n", aligned_buffer, aligned_endbuffer);
      ma_memory_register(memory_manager, aligned_buffer, aligned_size, 0, &data);
      err = 0;
    }
    else {
      if (source == -1) {
        mdebug_mami("Need to find out the location of the memory area\n");
        ma_memory_get_pages_location(data->pageaddrs, data->nbpages, &(data->node));
      }
      if (size < data->size) {
        size_t newsize;
        marcel_memory_data_t *next_data;

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

    if (err >= 0) {
      marcel_memory_data_link_t *area;

      mdebug_mami("Adding entity %p to data %p\n", owner, data);
      tbx_slist_push(data->owners, owner);
      mdebug_mami("Adding %lu bits to memnode offset for node #%d\n", (long unsigned)data->size, data->node);
      ((long *) ma_stats_get (owner, ma_stats_memnode_offset))[data->node] += data->size;

      area = tmalloc(sizeof(marcel_memory_data_link_t));
      area->data = data;
      INIT_LIST_HEAD(&(area->list));
      ma_spin_lock(&(owner->memory_areas_lock));
      list_add(&(area->list), &(owner->memory_areas));
      ma_spin_unlock(&(owner->memory_areas_lock));
    }
  }
  marcel_spin_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
  return err;
}

static
int ma_memory_entity_unattach(marcel_memory_manager_t *memory_manager,
                              void *buffer,
                              marcel_entity_t *owner) {
  int err=0, source;
  marcel_memory_data_t *data = NULL;

  marcel_spin_lock(&(memory_manager->lock));
  MAMI_LOG_IN();

  err = ma_memory_locate(memory_manager, memory_manager->root, buffer, 1, &source, &data);
  if (err >= 0) {
    marcel_entity_t *res;

    mdebug_mami("Removing entity %p from data %p\n", owner, data);

    if (tbx_slist_is_nil(data->owners)) {
      mdebug_mami("The entity %p is not attached to the memory area %p(1).\n", owner, buffer);
      errno = ENOENT;
      err = -errno;
    }
    else {
      res = tbx_slist_search_and_extract(data->owners, NULL, owner);
      if (res == owner) {
        marcel_memory_data_link_t *area;
        mdebug_mami("Removing %lu bits from memnode offset for node #%d\n", (long unsigned)data->size, data->node);
        ((long *) ma_stats_get (owner, ma_stats_memnode_offset))[data->node] -= data->size;

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
        mdebug_mami("The entity %p is not attached the memory area %p.\n", owner, buffer);
        errno = ENOENT;
        err = -errno;
      }
    }
  }
  marcel_spin_unlock(&(memory_manager->lock));
  MAMI_LOG_OUT();
  return err;
}

static
int ma_memory_entity_unattach_all(marcel_memory_manager_t *memory_manager,
                                  marcel_entity_t *owner) {
  marcel_memory_data_link_t *area, *narea;

  MAMI_LOG_IN();
  mdebug_mami("Unattaching all memory areas from entity %p\n", owner);
  //ma_spin_lock(&(owner->memory_areas_lock));
  list_for_each_entry_safe(area, narea, &(owner->memory_areas), list) {
    ma_memory_entity_unattach(memory_manager, area->data->startaddress, owner);
  }
  //ma_spin_unlock(&(owner->memory_areas_lock));
  MAMI_LOG_OUT();
  return 0;
}

int marcel_memory_task_attach(marcel_memory_manager_t *memory_manager,
                              void *buffer,
                              size_t size,
                              marcel_t owner) {
  marcel_entity_t *entity;
  entity = ma_entity_task(owner);
  return ma_memory_entity_attach(memory_manager, buffer, size, entity);
}

int marcel_memory_task_unattach(marcel_memory_manager_t *memory_manager,
                                void *buffer,
                                marcel_t owner) {
  marcel_entity_t *entity;
  entity = ma_entity_task(owner);
  return ma_memory_entity_unattach(memory_manager, buffer, entity);
}

int marcel_memory_bubble_attach(marcel_memory_manager_t *memory_manager,
                                void *buffer,
                                size_t size,
                                marcel_bubble_t *owner) {
  marcel_entity_t *entity;
  entity = ma_entity_bubble(owner);
  return ma_memory_entity_attach(memory_manager, buffer, size, entity);
}

int marcel_memory_bubble_unattach(marcel_memory_manager_t *memory_manager,
                                  void *buffer,
                                  marcel_bubble_t *owner) {
  marcel_entity_t *entity;
  entity = ma_entity_bubble(owner);
  return ma_memory_entity_unattach(memory_manager, buffer, entity);
}

int marcel_memory_task_unattach_all(marcel_memory_manager_t *memory_manager,
                                    marcel_t owner) {
  marcel_entity_t *entity;
  entity = ma_entity_bubble(owner);
  return ma_memory_entity_unattach_all(memory_manager, entity);
}

int marcel_memory_huge_pages_available(marcel_memory_manager_t *memory_manager) {
  return (memory_manager->hugepagesize != 0);
}

#endif /* MARCEL_MAMI_ENABLED */
