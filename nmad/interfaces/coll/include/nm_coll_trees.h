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

#include <nm_log.h>

/* **********************************************************/
/* trees description for collective operations */

/** kind of tree */
enum nm_coll_tree_kind_e
  {
    NM_COLL_TREE_NONE = 0,
    NM_COLL_TREE_FLAT,
    NM_COLL_TREE_CHAIN,
    NM_COLL_TREE_BINOMIAL,
    _NM_COLL_TREE_MAX
  };

/** description of an instanciated tree */
struct nm_coll_tree_info_s
{
  enum nm_coll_tree_kind_e kind;
  int max_arity;  /**< maximum number of children in tree */
  int n;          /**< number of nodes involved in collective */
  int self;       /**< rank of local node */
  int root;       /**< root node of the collective */
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

static inline void nm_coll_binomial_init(struct nm_coll_tree_info_s*p_tree)
{
  p_tree->max_arity = 1;
  p_tree->height = nm_coll_log2_ceil(p_tree->n);
}

/** walk to the leafs of the tree
 * @param i rank of local node
 * @param n number of leafs
 * @param level level in tree, numbered from the leafs
 */
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

/** number of leafs below the vertex */
static inline int nm_coll_binomial_weight(int i, int n, int level)
{
  const int skip = (1 << (level - 1));
  if(level <= 0)
    return 0;
  else if(i + skip > n)
    return (n - i);
  else
    return skip;
}

static inline void nm_coll_binomial_step(const struct nm_coll_tree_info_s*p_tree, int step, int*p_parent, int*p_children, int*n_children)
{
  const int child = nm_coll_binomial_child((p_tree->n + p_tree->self - p_tree->root) % p_tree->n, p_tree->n, p_tree->height - step - 1);
  const int parent = nm_coll_binomial_parent((p_tree->n + p_tree->self - p_tree->root) % p_tree->n, p_tree->n, p_tree->height - step - 1);
  if(parent != -1)
    {
      *p_parent = (parent + p_tree->root) % p_tree->n;
    }
  else
    {
      *p_parent = -1;
    }
  if(child != -1)
    {
      p_children[0] = (child + p_tree->root) % p_tree->n;
      *n_children = 1;
    }
  else
    {
      *n_children = 0;
    }
}

/* ** flat tree ******************************************** */

static inline void nm_coll_flat_init(struct nm_coll_tree_info_s*p_tree)
{
  p_tree->max_arity = (p_tree->self == p_tree->root) ? p_tree->n : 0;
  p_tree->height = 1;
}

static inline void nm_coll_flat_step(const struct nm_coll_tree_info_s*p_tree, int step, int*p_parent, int*p_children, int*n_children)
{
  if(p_tree->self == p_tree->root)
    {
      int j, k;
      for(k = 0, j = 0; k < p_tree->n; k++)
        {
          if(k != p_tree->root)
            {
              p_children[j] = k;
              j++;
            }
        }
      p_children[p_tree->n - 1] = -1;
      *n_children = p_tree->n - 1;
      *p_parent = -1;
    }
  else
    {
      *n_children = 0;
      *p_parent = 0;
    }
}

/* ** chain ************************************************ */

static inline void nm_coll_chain_init(struct nm_coll_tree_info_s*p_tree)
{
  p_tree->max_arity = 1;
  p_tree->height = p_tree->n - 1;
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
  
static inline void nm_coll_tree_init(struct nm_coll_tree_info_s*p_tree, enum nm_coll_tree_kind_e kind, int n, int self, int root)
{
  p_tree->kind = kind;
  p_tree->root = root;
  p_tree->n = n;
  p_tree->self = self;
  switch(kind)
    {
    case NM_COLL_TREE_FLAT:
      nm_coll_flat_init(p_tree);
      break;
    case NM_COLL_TREE_CHAIN:
      nm_coll_chain_init(p_tree);
      break;
    case NM_COLL_TREE_BINOMIAL:
      nm_coll_binomial_init(p_tree);
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

