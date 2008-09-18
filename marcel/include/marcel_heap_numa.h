/*
 * include/numasis_mem.h
 *
 * Reservation of memory adresses
 * and NUMA binding of physical pages
 *
 * Author: 	Jean-Francois Mehaut
 * 		Martinasso Maxime
 *
 * (C) Copyright 2007 INRIA
 * Projet: MESCAL / ANR NUMASIS
 *
 */

/** \addtogroup marcel_heap */
/* @{ */

#section common
#ifdef MA__NUMA_MEMORY
//#ifndef __NUMASIS_MEM_H
//#define __NUMASIS_MEM_H

#include <numa.h>
#include <numaif.h>
#include <stddef.h>
#include <sys/mman.h>

/* --- function prototypes --- */
#section functions
#if defined LINUX_SYS

/**
 * This function reserve a memory adresses range and bind the physical pages following the distribution
 * passed in arguments.
 * The binding can be done in two ways:
 *      - CYCLIC : memory is mapped by bloc following a cyclic placement
 *      - LESS_LOADED : map to the node with the most free memory from nodemask
 *      - SMALL_ACCESSED : map to the node with the lowest number of hits from nodemask
 * Note that for each mapping the size of bloc are aligned to page size.
 * If flags is not correctly set, by default CYCLIC is used.
 */
int ma_maparea(void *ptr, size_t size, int mempolicy, unsigned long *nodemask, unsigned long maxnode);

/**
 *
 */
long long ma_free_mem_node(int node);

/**
 *
 */
long long ma_hits_mem_node(int node);

#endif /* LINUX_SYS */

#section common
#endif /* MA__NUMA_MEMORY */

/* @} */
