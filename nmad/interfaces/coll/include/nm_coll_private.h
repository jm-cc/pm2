/*
 * NewMadeleine
 * Copyright (C) 2014-2018 (see AUTHORS file)
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

#include "nm_coll_interface.h"


/* ********************************************************* */

/** maximum tag usable by enduser for p2p */
#define NM_COLL_TAG_MASK_P2P      0x7FFFFFFF
/** base tag used for collectives */
#define NM_COLL_TAG_BASE          0xFF000000
/** barrier */
#define NM_COLL_TAG_BARRIER       ( NM_COLL_TAG_BASE | 0x01 )
/** bcast */
#define NM_COLL_TAG_BCAST         ( NM_COLL_TAG_BASE | 0x02 )
/** gather */
#define NM_COLL_TAG_GATHER        ( NM_COLL_TAG_BASE | 0x03 )
/** comm_create phase 1 */
#define NM_COLL_TAG_COMM_CREATE_1 ( NM_COLL_TAG_BASE | 0x04 )
/** comm_create phase 2 */
#define NM_COLL_TAG_COMM_CREATE_2 ( NM_COLL_TAG_BASE | 0x05 )


/* ********************************************************* */

/* ** trees */

/** floor(log2(X)) */
#define LOG2(X) ((unsigned) (8*sizeof (unsigned long long) - __builtin_clzll((X)) - 1))

/** number of communications steps in the tree (0 for a single node) */
static inline int nm_coll_binomial_height(int n)
{
  assert(n > 0);
  const int l = (LOG2(n)) ;
  return l + (!!(n & (n - 1))); /* = ceil(log2(n)) */
}

/** walk to the leafs of the tree */
static inline int nm_coll_binomial_child(int i, int n, int level)
{
  const int skip = (1 << level);
  if((i % (2*skip) == 0) && (i + skip < n))
    return (i + skip);
  else
    return -1;
}

/** walk to the root of the tree */
static inline int nm_coll_binomial_parent(int i, int n, int level)
{
  const int skip = (1 << level);
  if(i % (2*skip) == skip)
    return (i - skip);
  else
    return -1;
}
