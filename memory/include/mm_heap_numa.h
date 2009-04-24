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

#ifdef MM_HEAP_ENABLED

#if defined LINUX_SYS

#ifndef MM_HEAP_NUMA_H
#define MM_HEAP_NUMA_H

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

#endif /* MM_HEAP_NUMA_H */
#endif /* LINUX_SYS */
#endif /* MM_HEAP_ENABLED */

/* @} */
