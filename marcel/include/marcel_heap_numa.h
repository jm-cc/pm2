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

//#ifndef __NUMASIS_MEM_H
//#define __NUMASIS_MEM_H

#include <numa.h>
#include <numaif.h>
#include <stddef.h>
#include <sys/mman.h>
#include <linux/mempolicy.h>

/* --- function prototypes --- */
#section functions
#if defined LINUX_SYS
int ma_maparea(void *ptr, size_t size, int mempolicy, unsigned long *nodemask, unsigned long maxnode);
long long ma_free_mem_node(int node);
long long ma_hits_mem_node(int node);

#endif /* LINUX_SYS */
