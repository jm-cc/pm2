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
#include <nm_private.h>

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

enum nm_coll_tree_kind_e
  {
    NM_COLL_TREE_NONE = 0,
    NM_COLL_TREE_FLAT,
    NM_COLL_TREE_CHAIN,
    NM_COLL_TREE_BINOMIAL,
    _NM_COLL_TREE_MAX
  };
struct nm_coll_tree_info_s
{
  enum nm_coll_tree_kind_e kind;
  int max_arity;  /**< maximum number of children in tree */
  int n;          /**< number of nodes invovled in collective */
  int self;       /**< rank of local node */
  int height;     /**< tree height: number of communications steps in the tree (0 for a single node) */
};

/** floor(log2(x)) */
static inline int nm_coll_log2_floor(int x)
{
  assert(x > 0);
  return ((unsigned) (8*sizeof (unsigned long long) - __builtin_clzll((x)) - 1));
}

/** ceil(log2(x)) */
static inline int nm_coll_log2_ceil(int x)
{
  assert(x > 0);
  return nm_coll_log2_floor(x) + (!!(x & (x - 1)));
}

/* ** binomial tree **************************************** */

static inline void nm_coll_binomial_init(struct nm_coll_tree_info_s*p_tree, int n, int self)
{
  p_tree->max_arity = 1;
  p_tree->n = n;
  p_tree->self = self;
  p_tree->height = nm_coll_log2_ceil(n);
}

/** walk to the leafs of the tree, level numbered from the leafs */
static inline int nm_coll_binomial_child(int i, int n, int level)
{
  const int skip = (1 << level);
  if((i % (2*skip) == 0) && (i + skip < n))
    return (i + skip);
  else
    return -1;
}

/** walk to the root of the tree, level numbered from the leafs */
static inline int nm_coll_binomial_parent(int i, int n, int level)
{
  const int skip = (1 << level);
  if(i % (2*skip) == skip)
    return (i - skip);
  else
    return -1;
}

static inline void nm_coll_binomial_step(const struct nm_coll_tree_info_s*p_tree, int step, int*p_parent, int*p_children, int*n_children)
{
  const int child = nm_coll_binomial_child(p_tree->self, p_tree->n, p_tree->height - step - 1);
  const int parent = nm_coll_binomial_parent(p_tree->self, p_tree->n, p_tree->height - step - 1);
  *p_parent = parent;
  p_children[0] = child;
  *n_children = 1;
}

/* ** flat tree ******************************************** */

static inline void nm_coll_flat_init(struct nm_coll_tree_info_s*p_tree, int n, int self)
{
  p_tree->max_arity = (self == 0) ? n : 0;
  p_tree->n = n;
  p_tree->self = self;
  p_tree->height = 1;
}

static inline void nm_coll_flat_step(const struct nm_coll_tree_info_s*p_tree, int step, int*p_parent, int*p_children, int*n_children)
{
  if(p_tree->self == 0)
    {
      int k;
      for(k = 0; k < p_tree->n - 1; k++)
        {
          p_children[k] = k + 1;
        }
      p_children[p_tree->n - 1] = -1;
      *n_children = p_tree->n - 1;
      *p_parent = -1;
    }
  else
    {
      p_children[0] = -1;
      *n_children = 0;
      *p_parent = 0;
    }
}

/* ** chain ************************************************ */

static inline void nm_coll_chain_init(struct nm_coll_tree_info_s*p_tree, int n, int self)
{
  p_tree->max_arity = 1;
  p_tree->n = n;
  p_tree->self = self;
  p_tree->height = n - 1;
}

static inline void nm_coll_chain_step(const struct nm_coll_tree_info_s*p_tree, int step, int*p_parent, int*p_children, int*n_children)
{
  if((p_tree->self == step) && (p_tree->self < p_tree->n - 1))
    {
      p_children[0] = p_tree->self + 1;
      *n_children = 1;
    }
  else
    {
      p_children[0] = -1;
      *n_children = 0;
    }
  if((p_tree->self - 1 == step) && (p_tree->self > 0))
    {
      *p_parent = p_tree->self - 1;
    }
  else
    {
      *p_parent = -1;
    }
}
  
/* ** generic trees **************************************** */
  
static inline void nm_coll_tree_init(struct nm_coll_tree_info_s*p_tree, enum nm_coll_tree_kind_e kind, int n, int self)
{
  p_tree->kind = kind;
  switch(kind)
    {
    case NM_COLL_TREE_FLAT:
      nm_coll_flat_init(p_tree, n, self);
      break;
    case NM_COLL_TREE_CHAIN:
      nm_coll_chain_init(p_tree, n, self);
      break;
    case NM_COLL_TREE_BINOMIAL:
      nm_coll_binomial_init(p_tree, n, self);
      break;
    default:
      NM_FATAL("coll- unexpected kind of tree %d.\n", kind);
      break;
    }
}

static inline void nm_coll_tree_step(const struct nm_coll_tree_info_s*p_tree, int step, int*p_parent, int*p_children, int*n_children)
{
  switch(p_tree->kind)
    {
    case NM_COLL_TREE_FLAT:
      nm_coll_flat_step(p_tree, step, p_parent, p_children, n_children);
      break;
    case NM_COLL_TREE_CHAIN:
      nm_coll_chain_step(p_tree, step, p_parent, p_children, n_children);
      break;
    case NM_COLL_TREE_BINOMIAL:
      nm_coll_binomial_step(p_tree, step, p_parent, p_children, n_children);
      break;
    default:
      NM_FATAL("coll- unexpected kind of tree %d.\n", p_tree->kind);
      break;
    }
}

