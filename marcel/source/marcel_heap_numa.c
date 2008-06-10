/*
 * src/maheap_numa.c
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

#include <errno.h>
#include <stdio.h>

#include "marcel.h"

#ifdef MA__NUMA

#if defined LINUX_SYS
#include <numaif.h>
#include <numa.h>
#include <stddef.h>
#include <sys/mman.h>
#include <linux/mempolicy.h>
#include <ctype.h>

int ma_maparea(void *ptr, size_t size, int mempolicy, unsigned long *nodemask, unsigned long maxnode) {
        int err, i, idx_node = 0;
        long long node_free_size, max_free_size = 0;
        long long node_hits, min_hits = LONG_MAX;
        unsigned long newnodemask[MARCEL_NBMAXNODES];
        unsigned long newmaxnode;

        mdebug_heap("ma_maparea size %d, start address %p mask=%ld ", (int)size, ptr,*nodemask);
	switch(mempolicy) {
	case SMALL_ACCESSED:
		/* marcel_nbnodes = numa_max_node() + 1 */
		for(i = 0; i < marcel_nbnodes; ++i) {
			if (mask_isset(nodemask,maxnode,i)) {
				node_hits = ma_hits_mem_node(i);
				//mdebug_heap("node=%d hits=%lld\n",i,node_hits);
				if (min_hits > node_hits) {
					min_hits = node_hits;
					idx_node = i;
				}
			}
		}
		newmaxnode = maxnode;
		mask_zero(newnodemask,newmaxnode);
		mask_set(newnodemask,idx_node);
		err = mbind (ptr, size, MPOL_BIND , newnodemask, newmaxnode, MPOL_MF_MOVE);
		break;
	case LESS_LOADED:
		for(i = 0; i < marcel_nbnodes; ++i) {
			if (mask_isset(nodemask,maxnode,i)) {
				node_free_size = ma_free_mem_node(i);
				//mdebug_heap("node=%d size=%lld\n",i,node_free_size);
				if (node_free_size > max_free_size) {
					max_free_size = node_free_size;
					idx_node = i;
				}
			}
		}
		newmaxnode = maxnode;
		mask_zero(newnodemask,newmaxnode);
		mask_set(newnodemask,idx_node);
		err = mbind (ptr, size, MPOL_BIND , newnodemask, newmaxnode, MPOL_MF_MOVE);
		break;
	case CYCLIC:
	default:
		err = mbind (ptr, size, MPOL_INTERLEAVE , nodemask, maxnode, MPOL_MF_MOVE);
		if (err) {
			perror("mbind error:");
			mdebug_heap("mbind args %p %ld %d %ld %ld 0\n",ptr, size, MPOL_INTERLEAVE|MPOL_MF_MOVE, *nodemask, maxnode);
		} else {
			mdebug_heap("mbind returns %d : Success\n", err);
		}
		break;
        }
        return err;
}

long long ma_free_mem_node(int node) {
        char fn[64];
        long long size = -1;
        size_t len = 0;
        char *line = NULL;
        FILE *f;

        sprintf(fn,"/sys/devices/system/node/node%d/meminfo", node);
        f = fopen(fn, "r");
        if (!f)
                return -1;
        while (getdelim(&line, &len, '\n', f) > 0) {
                char *end;
                char *s = strcasestr(line, "kB");
                if (!s)
                        continue;
                --s;
                while (s > line && isspace(*s))
                        --s;
                while (s > line && isdigit(*s))
                        --s;
                if (strstr(line, "MemFree")) {
                        size = strtoull(s,&end,0) << 10;
                        if (end == s) size= -1;
                }
        }
        fclose(f);
        free(line);
        return size;
}

long long ma_hits_mem_node(int node) {
        char fn[64];
        long long hits = -1;
        size_t len = 0;
        char *line = NULL;
        FILE *f;

        sprintf(fn,"/sys/devices/system/node/node%d/numastat", node);
        f = fopen(fn, "r");
        if (!f)
                return -1;
        while (getdelim(&line, &len, '\n', f) > 0) {
                if (strstr(line, "numa_hit")) {
                        hits = strtoull(line+8,NULL,0);
                        break;
                }
        }
        fclose(f);
        free(line);
        return hits;
}

#endif /* LINUX_SYS */
#endif
