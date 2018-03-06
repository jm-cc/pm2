/*
 * NewMadeleine
 * Copyright (C) 2014 (see AUTHORS file)
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

#include <nm_public.h>
#include <nm_sendrecv_interface.h>
#include <Padico/Puk.h>
#include <nm_launcher_interface.h>
#include "nm_coll_private.h"

/* ********************************************************* */

void nm_group_free(nm_group_t group)
{
  if(group != NULL)
    {
      nm_gate_vect_delete(group);
    }
}

int nm_group_size(nm_group_t group)
{
  if(group != NULL)
    {
      return nm_gate_vect_size(group);
    }
  return -1;
}

int nm_group_rank(nm_group_t group)
{
  if(group != NULL)
    {
      nm_gate_t p_gate = nm_launcher_self_gate();
      nm_gate_vect_itor_t pp_self = nm_gate_vect_find(group, p_gate);
      if(pp_self != NULL)
	{
	  const int rank = nm_gate_vect_rank(group, pp_self);
	  return rank;
	}
    }
  return -1;
}

nm_gate_t nm_group_get_gate(nm_group_t p_group, int rank)
{
  if(rank >= 0 && rank < nm_group_size(p_group))
    {
      return nm_gate_vect_at(p_group, rank);
    }
  else
    {
      return NULL;
    }
}

int nm_group_get_dest(nm_group_t p_group, nm_gate_t p_gate)
{
  nm_gate_vect_itor_t i = nm_gate_vect_find(p_group, p_gate);
  return nm_gate_vect_rank(p_group, i);
}

nm_group_t nm_group_dup(nm_group_t group)
{
  nm_group_t newgroup = nm_gate_vect_copy(group);
  return newgroup;
}

int nm_group_compare(nm_group_t group1, nm_group_t group2)
{
  if(group1 == NULL && group2 == NULL)
    {
      return NM_GROUP_IDENT;
    }
  else if((group1 == NULL) || (group2 == NULL) || (nm_gate_vect_size(group1) != nm_gate_vect_size(group2)))
    {
      return NM_GROUP_UNEQUAL;
    }
  else
    {
      nm_gate_vect_itor_t i1 = nm_gate_vect_begin(group1), i2 = nm_gate_vect_begin(group2);
      while(i1 != nm_gate_vect_end(group1) && i2 != nm_gate_vect_end(group2))
	{
	  nm_gate_t p_gate1 = *i1, p_gate2 = *i2;
	  if(p_gate1 != p_gate2)
	    break;
	  i1 = nm_gate_vect_next(i1);
	  i2 = nm_gate_vect_next(i2);
	}
      if(i1 == nm_gate_vect_end(group1) && i2 == nm_gate_vect_end(group2))
	{
	  return NM_GROUP_IDENT;
	}
      /* note: this is an n^2 algorithm. n.log n should be possible */
      puk_vect_foreach(i1, nm_gate, group1)
	{
	  nm_gate_t p_gate = *i1;
	  if(nm_gate_vect_find(group2, p_gate) == NULL)
	    {
	      return NM_GROUP_UNEQUAL;
	    }
	}
      return NM_GROUP_SIMILAR;
    }
}

nm_group_t nm_group_incl(nm_group_t group, int n, const int ranks[])
{
  nm_group_t newgroup = nm_gate_vect_new();
  int i;
  for(i = 0; i < n; i++)
    {
      int r = ranks[i];
      if(r >= 0 && r < nm_gate_vect_size(group))
	{
	  nm_gate_t p_gate = nm_gate_vect_at(group, r);
	  nm_gate_vect_push_back(newgroup, p_gate);
	}
    }
  return newgroup;
}

nm_group_t nm_group_excl(nm_group_t group, int n, const int ranks[])
{
  nm_group_t dupgroup = nm_gate_vect_copy(group);
  /* prune excluded ranks */
  int i;
  for(i = 0; i < n; i++)
    {
      int r = ranks[i];
      if(r >= 0 && r < nm_gate_vect_size(dupgroup))
	{
	  nm_gate_t*pp_gate = nm_gate_vect_ptr(dupgroup, r);
	  *pp_gate = NULL;
	}
    }
  /* compact vector */
  nm_gate_vect_itor_t v1, v2;
  for(v1  = nm_gate_vect_begin(dupgroup), v2 = nm_gate_vect_begin(dupgroup);
      v1 != nm_gate_vect_end(dupgroup);
      v1  = nm_gate_vect_next(v1))
    {
      if((*v1) != NULL)
	{
	  if(v1 != v2)
	    {
	      *v2 = *v1;
	    }
	  v2 = nm_gate_vect_next(v2);
	}
    }
  nm_gate_vect_resize(dupgroup, nm_gate_vect_size(group) - n);
  return dupgroup;
}

nm_group_t nm_group_union(nm_group_t group1, nm_group_t group2)
{
  nm_group_t newgroup = nm_gate_vect_copy(group1);
  nm_gate_vect_itor_t i;
  puk_vect_foreach(i, nm_gate, group2)
    {
      const nm_gate_t p_gate = *i;
      if(nm_gate_vect_find(group1, p_gate) == NULL)
	{
	  nm_gate_vect_push_back(newgroup, p_gate);
	}
    }
  return newgroup;
}

nm_group_t nm_group_intersection(nm_group_t group1, nm_group_t group2)
{
  nm_group_t newgroup = nm_gate_vect_new();
  nm_gate_vect_itor_t i;
  puk_vect_foreach(i, nm_gate, group1)
    {
      const nm_gate_t p_gate = *i;
      if(nm_gate_vect_find(group2, p_gate) != NULL)
	{
	  nm_gate_vect_push_back(newgroup, p_gate);
	}
    }
  return newgroup;
}

nm_group_t nm_group_difference(nm_group_t group1, nm_group_t group2)
{
  nm_group_t newgroup = nm_gate_vect_new();
  nm_gate_vect_itor_t i;
  puk_vect_foreach(i, nm_gate, group1)
    {
      const nm_gate_t p_gate = *i;
      if(nm_gate_vect_find(group2, p_gate) == NULL)
	{
	  nm_gate_vect_push_back(newgroup, p_gate);
	}
    }
  return newgroup;
}

int nm_group_translate_ranks(nm_group_t p_group1, int n, int*ranks1, nm_group_t p_group2, int*ranks2)
{
  int i;
  for(i = 0; i < n; i++)
    {
      ranks2[i] = -1;
      nm_gate_t p_gate = nm_group_get_gate(p_group1, i);
      if(p_gate != NULL)
	{
	  nm_gate_vect_itor_t pp_gate = nm_gate_vect_find(p_group2, p_gate);
	  if(pp_gate != NULL)
	    {
	      const int rank = nm_gate_vect_rank(p_group2, pp_gate);
	      ranks2[i] = rank;
	    }
	}
    }
  return 0;
}
