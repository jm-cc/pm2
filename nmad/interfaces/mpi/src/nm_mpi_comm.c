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

#include "nm_mpi_private.h"
#include <assert.h>
#include <errno.h>

#include <Padico/Module.h>
PADICO_MODULE_HOOK(NewMad_Core);



NM_MPI_HANDLE_TYPE(communicator, nm_mpi_communicator_t, _NM_MPI_COMM_OFFSET, 16); 
NM_MPI_HANDLE_TYPE(group, nm_mpi_group_t, _NM_MPI_GROUP_OFFSET, 16); 

static struct nm_mpi_handle_communicator_s nm_mpi_communicators;
static struct nm_mpi_handle_group_s nm_mpi_groups;

/* ********************************************************* */

int MPI_Comm_size(MPI_Comm comm, int*size)
  __attribute__ ((alias ("mpi_comm_size")));

int MPI_Comm_rank(MPI_Comm comm, int*rank)
  __attribute__ ((alias ("mpi_comm_rank")));

int MPI_Attr_get(MPI_Comm comm, int keyval, void*attr_value, int *flag)
  __attribute__ ((alias ("mpi_attr_get")));

int MPI_Comm_group(MPI_Comm comm, MPI_Group*group)
  __attribute__ ((alias ("mpi_comm_group")));

int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm*newcomm)
  __attribute__ ((alias ("mpi_comm_create")));

int MPI_Comm_split(MPI_Comm comm,
		   int color,
		   int key,
		   MPI_Comm *newcomm) __attribute__ ((alias ("mpi_comm_split")));

int MPI_Comm_dup(MPI_Comm comm,
		 MPI_Comm *newcomm) __attribute__ ((alias ("mpi_comm_dup")));

int MPI_Comm_free(MPI_Comm *comm) __attribute__ ((alias ("mpi_comm_free")));


int MPI_Cart_create(MPI_Comm comm_old, int ndims, int*dims, int*periods, int reorder, MPI_Comm*_comm_cart)
  __attribute__ ((alias ("mpi_cart_create")));

int MPI_Cart_coords(MPI_Comm comm, int rank, int ndims, int*coords)
  __attribute__ ((alias ("mpi_cart_coords")));

int MPI_Cart_rank(MPI_Comm comm, int*coords, int*rank)
  __attribute__ ((alias ("mpi_cart_rank")));

int MPI_Cart_shift(MPI_Comm comm, int direction, int displ, int*source, int*dest)
  __attribute__ ((alias ("mpi_cart_shift")));

int MPI_Group_size(MPI_Group group, int*size)
  __attribute__ ((alias ("mpi_group_size")));

int MPI_Group_rank(MPI_Group group, int*rank)
  __attribute__ ((alias ("mpi_group_rank")));

int MPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group*newgroup)
  __attribute__ ((alias ("mpi_group_union")));

int MPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group*newgroup)
  __attribute__ ((alias ("mpi_group_intersection")));

int MPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group*newgroup)
  __attribute__ ((alias ("mpi_group_difference")));

int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int*result)
  __attribute__ ((alias ("mpi_group_compare")));

int MPI_Group_incl(MPI_Group group, int n, int*ranks, MPI_Group*newgroup) 
  __attribute__ ((alias ("mpi_group_incl")));

int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group*newgroup)
  __attribute__ ((alias ("mpi_group_range_incl")));

int MPI_Group_excl(MPI_Group group, int n, int*ranks, MPI_Group*newgroup) 
  __attribute__ ((alias ("mpi_group_excl")));

int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group*newgroup)
  __attribute__ ((alias ("mpi_group_range_excl")));

int MPI_Group_free(MPI_Group*group)
  __attribute__ ((alias ("mpi_group_free")));

int MPI_Group_translate_ranks(MPI_Group group1,
			      int n,
			      int *ranks1,
			      MPI_Group group2,
			      int *ranks2)
  __attribute__ ((alias ("mpi_group_translate_ranks")));

/* ********************************************************* */

__PUK_SYM_INTERNAL
void nm_mpi_comm_init(void)
{
  /* handles allocator */
  nm_mpi_handle_communicator_init(&nm_mpi_communicators);
  nm_mpi_handle_group_init(&nm_mpi_groups);

  /* built-in communicators */
  nm_mpi_communicator_t*p_comm_world = nm_mpi_handle_communicator_store(&nm_mpi_communicators, MPI_COMM_WORLD);
  p_comm_world->p_comm = nm_comm_world();
  nm_mpi_communicator_t*p_comm_self = nm_mpi_handle_communicator_store(&nm_mpi_communicators, MPI_COMM_SELF);
  p_comm_self->p_comm = nm_comm_self();

  /* built-in group */
  nm_mpi_group_t*p_group_empty = nm_mpi_handle_group_store(&nm_mpi_groups, MPI_GROUP_EMPTY);
  p_group_empty->p_nm_group = nm_gate_vect_new();
}

__PUK_SYM_INTERNAL
void nm_mpi_comm_exit(void)
{
  nm_mpi_communicator_t*p_comm_world = nm_mpi_handle_communicator_get(&nm_mpi_communicators, MPI_COMM_WORLD);
  nm_mpi_handle_communicator_free(&nm_mpi_communicators, p_comm_world);
  nm_mpi_communicator_t*p_comm_self = nm_mpi_handle_communicator_get(&nm_mpi_communicators, MPI_COMM_SELF);
  nm_mpi_handle_communicator_free(&nm_mpi_communicators, p_comm_self);
  nm_mpi_handle_communicator_finalize(&nm_mpi_communicators);
  nm_mpi_group_t*p_group_empty = nm_mpi_handle_group_get(&nm_mpi_groups, MPI_GROUP_EMPTY);
  nm_mpi_handle_group_free(&nm_mpi_groups, p_group_empty);
  nm_mpi_handle_group_finalize(&nm_mpi_groups);
}

__PUK_SYM_INTERNAL
nm_mpi_communicator_t*nm_mpi_communicator_get(MPI_Comm comm)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_handle_communicator_get(&nm_mpi_communicators, comm);
  return p_comm;
}

__PUK_SYM_INTERNAL
nm_gate_t nm_mpi_communicator_get_gate(nm_mpi_communicator_t*p_comm, int node)
{
  return nm_comm_get_gate(p_comm->p_comm, node);
}

__PUK_SYM_INTERNAL
int nm_mpi_communicator_get_dest(nm_mpi_communicator_t*p_comm, nm_gate_t p_gate)
{
  return nm_comm_get_dest(p_comm->p_comm, p_gate);
}

/* ********************************************************* */

int mpi_comm_size(MPI_Comm comm, int *size)
{
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  if(size)
    {
      *size = nm_comm_size(p_comm->p_comm);
    }
  return MPI_SUCCESS;
}

int mpi_comm_rank(MPI_Comm comm, int *rank)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(rank)
    {
      *rank = nm_comm_rank(p_comm->p_comm);
    }
  return MPI_SUCCESS;
}

int mpi_attr_get(MPI_Comm comm, int keyval, void *attr_value, int *flag)
{
  int err = MPI_SUCCESS;
  switch(keyval)
    {
    case MPI_TAG_UB:
      *(int*)attr_value = NM_MPI_TAG_MAX;
      *flag = 1;
      break;
    case MPI_HOST:
      *flag = 0;
      break;
    case MPI_IO:
      *flag = 0;
      break;
    case MPI_WTIME_IS_GLOBAL:
      *(int*)attr_value = 0;
      *flag = 1;
      break;
    default:
      *flag = 0;
      break;
    }
  return err;
}

int mpi_comm_group(MPI_Comm comm, MPI_Group *group)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    return MPI_ERR_COMM;
  if(group != NULL)
    {
      nm_mpi_group_t*p_new_group = nm_mpi_handle_group_alloc(&nm_mpi_groups);
      p_new_group->p_nm_group = nm_group_dup(nm_comm_group(p_comm->p_comm));
      MPI_Group new_id = p_new_group->id;;
      *group = new_id;
    }
  return MPI_SUCCESS;
}

int mpi_comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm*newcomm)
{
  nm_mpi_communicator_t*p_old_comm = nm_mpi_communicator_get(comm);
  if(p_old_comm == NULL)
    return MPI_ERR_COMM;
  nm_mpi_group_t*p_new_group = nm_mpi_handle_group_get(&nm_mpi_groups, group);
  if(p_new_group == NULL)
    return MPI_ERR_GROUP;
  nm_comm_t p_nm_comm = nm_comm_create(p_old_comm->p_comm, p_new_group->p_nm_group);
  if(p_nm_comm == NULL)
    return MPI_ERR_COMM;
  nm_mpi_communicator_t*p_new_comm = nm_mpi_handle_communicator_alloc(&nm_mpi_communicators);
  p_new_comm->p_comm = p_nm_comm;
  *newcomm = p_new_comm->id;
  return MPI_SUCCESS;
}

/** a node participating in a mpi_coll_split */
struct nm_mpi_comm_split_node_s
{
  int color;
  int key;
  int rank;
};
/** sort by color major, key minor */
static int nodecmp(const void*v1, const void*v2) 
{
  const struct nm_mpi_comm_split_node_s*n1 = v1;
  const struct nm_mpi_comm_split_node_s*n2 = v2;
  if(n1->color < n2->color)
    return -1;
  else if(n1->color > n2->color)
    return +1;
  else
    return (n1->key - n2->key);
}

int mpi_comm_split(MPI_Comm oldcomm, int color, int key, MPI_Comm *newcomm)
{
  const int root = 0; /* rank 0 in oldcomm is root */
  const int tag = NM_MPI_TAG_PRIVATE_COMMSPLIT;
  int i;
  nm_mpi_communicator_t *p_old_comm = nm_mpi_communicator_get(oldcomm);
  const int comm_size = nm_comm_size(p_old_comm->p_comm);
  const int rank = nm_comm_rank(p_old_comm->p_comm);
  struct nm_mpi_comm_split_node_s local_node = { .color = color, .key = key, .rank = rank};
  struct nm_mpi_comm_split_node_s*all_nodes = malloc(comm_size * sizeof(struct nm_mpi_comm_split_node_s));

  nm_coll_gather(p_old_comm->p_comm, root, &local_node, sizeof(local_node), all_nodes, sizeof(local_node), tag);
  if(rank == root)
    {
      qsort(all_nodes, comm_size, sizeof(struct nm_mpi_comm_split_node_s), &nodecmp);
    }
  nm_coll_bcast(p_old_comm->p_comm, root, all_nodes, comm_size * sizeof(struct nm_mpi_comm_split_node_s), tag);
  int lastcol = all_nodes[0].color;
  nm_group_t newgroup = nm_gate_vect_new();
  for(i = 0; i <= comm_size; i++)
    {
      if((i == comm_size) || (all_nodes[i].color != lastcol))
	{
	  /* new color => create sub-communicator */
	  nm_comm_t p_nm_comm = nm_comm_create(p_old_comm->p_comm, newgroup);
	  if(color == MPI_UNDEFINED)
	    {
	      *newcomm = MPI_COMM_NULL;
	    }
	  else if(p_nm_comm != NULL)
	    {
	      nm_mpi_communicator_t*p_new_comm = nm_mpi_handle_communicator_alloc(&nm_mpi_communicators);
	      p_new_comm->p_comm = p_nm_comm;
	      *newcomm = p_new_comm->id;
	    }
	  nm_group_free(newgroup);
	  newgroup = NULL;
	  if(i < comm_size)
	    {
	      lastcol = all_nodes[i + 1].color;
	      newgroup = nm_gate_vect_new();
	    }
	  else
	    {
	      break;
	    }
	}
      nm_gate_vect_push_back(newgroup, nm_gate_vect_at(nm_comm_group(p_old_comm->p_comm), all_nodes[i].rank));
    }
  FREE_AND_SET_NULL(all_nodes);
  return MPI_SUCCESS;
}

int mpi_comm_dup(MPI_Comm oldcomm, MPI_Comm *newcomm)
{
  nm_mpi_communicator_t*p_old_comm = nm_mpi_communicator_get(oldcomm);
  if(p_old_comm == NULL)
    {
      ERROR("Communicator %d is not valid", oldcomm);
      return MPI_ERR_OTHER;
    }
  else
    {
      nm_mpi_communicator_t*p_new_comm = nm_mpi_handle_communicator_alloc(&nm_mpi_communicators);
      p_new_comm->p_comm = nm_comm_dup(p_old_comm->p_comm);
      *newcomm = p_new_comm->id;
    }
  return MPI_SUCCESS;
}

int mpi_comm_free(MPI_Comm *comm)
{
  if(*comm == MPI_COMM_WORLD)
    {
      ERROR("Cannot free communicator MPI_COMM_WORLD");
      return MPI_ERR_OTHER;
    }
  else if(*comm == MPI_COMM_SELF)
    {
      ERROR("Cannot free communicator MPI_COMM_SELF");
      return MPI_ERR_OTHER;
    }
  nm_mpi_communicator_t*p_comm = nm_mpi_handle_communicator_get(&nm_mpi_communicators, *comm);
  if(p_comm == NULL)
    {
      ERROR("Communicator %d unknown\n", *comm);
      return MPI_ERR_OTHER;
    }
  nm_mpi_handle_communicator_free(&nm_mpi_communicators, p_comm);
  *comm = MPI_COMM_NULL;
  return MPI_SUCCESS;
}

int mpi_cart_create(MPI_Comm comm_old, int ndims, int*dims, int*periods, int reorder, MPI_Comm*newcomm)
{
  MPI_Comm comm_cart = MPI_COMM_NULL;
  int err = MPI_Comm_dup(comm_old, &comm_cart);
  if(err != MPI_SUCCESS)
    {
      return err;
    }
  nm_mpi_communicator_t*p_old_comm = nm_mpi_communicator_get(comm_old);
  struct nm_mpi_cart_topology_s cart;
  if(ndims < 0)
    {
      return MPI_ERR_DIMS;
    }
  cart.ndims = ndims;
  cart.dims = malloc(ndims * sizeof(int));
  cart.periods = malloc(ndims * sizeof(int));
  cart.size = 1;
  int d;
  for(d = 0; d < ndims; d++)
    {
      if(dims[d] <= 0)
	{
	  return MPI_ERR_DIMS;
	}
      cart.dims[d] = dims[d];
      cart.periods[d] = periods[d];
      cart.size *= dims[d];
    }
  if(cart.size > nm_comm_size(p_old_comm->p_comm))
    return MPI_ERR_TOPOLOGY;
  nm_group_t cart_group = nm_gate_vect_new();
  int i;
  for(i = 0; i < nm_comm_size(p_old_comm->p_comm); i++)
    {
      nm_gate_vect_push_back(cart_group, nm_comm_get_gate(p_old_comm->p_comm, i));
    }
  nm_mpi_communicator_t*p_new_comm = nm_mpi_handle_communicator_alloc(&nm_mpi_communicators);
  p_new_comm->p_comm = nm_comm_create(p_old_comm->p_comm, cart_group);
  p_new_comm->cart_topology = cart;
  *newcomm = p_new_comm->id;
  nm_group_free(cart_group);
  return MPI_SUCCESS;
}

int mpi_cart_coords(MPI_Comm comm, int rank, int ndims, int*coords)
{
  nm_mpi_communicator_t*mpir_comm_cart = nm_mpi_communicator_get(comm);
  struct nm_mpi_cart_topology_s*cart = &mpir_comm_cart->cart_topology;
  if(ndims < cart->ndims)
    return MPI_ERR_DIMS;
  int d;
  int nnodes = cart->size;
  for(d = 0; d < cart->ndims; d++)
    {
      nnodes    = nnodes / cart->dims[d];
      coords[d] = rank / nnodes;
      rank      = rank % nnodes;
    }
  return MPI_SUCCESS;
}

int mpi_cart_rank(MPI_Comm comm, int*coords, int*rank)
{
  nm_mpi_communicator_t*mpir_comm_cart = nm_mpi_communicator_get(comm);
  struct nm_mpi_cart_topology_s*cart = &mpir_comm_cart->cart_topology;
  const int ndims = cart->ndims;
  *rank = 0;
  int multiplier = 1;
  int i;
  for(i = ndims - 1; i >= 0; i--)
    {
      int coord = coords[i];
      if(cart->periods[i])
	{
	  if(coord >= cart->dims[i])
	    {
	      coord = coord % cart->dims[i];
	    }
	  else if(coord < 0)
	    {
	      coord = coord % cart->dims[i];
	      if(coord)
		coord = cart->dims[i] + coord;
	    }
	}
      else
	if(coord >= cart->dims[i] || coord < 0)
	  {
	    *rank = MPI_PROC_NULL;
	    return MPI_ERR_RANK;
	  }
      *rank += multiplier * coord;
      multiplier *= cart->dims[i];
    }
  return MPI_SUCCESS;
}

int mpi_cart_shift(MPI_Comm comm, int direction, int displ, int*source, int*dest)
{
  nm_mpi_communicator_t*p_comm_cart = nm_mpi_communicator_get(comm);
  struct nm_mpi_cart_topology_s*cart = &p_comm_cart->cart_topology;
  const int rank = nm_comm_rank(p_comm_cart->p_comm);
  if(direction < 0 || direction >= cart->ndims)
    return MPI_ERR_ARG;
  if(displ == 0)
    return MPI_ERR_ARG;
  int*coords = malloc(cart->ndims * sizeof(int));
  int err = mpi_cart_coords(comm, rank, cart->ndims, coords);
  if(err != MPI_SUCCESS)
    return err;
  coords[direction] += displ;
  mpi_cart_rank(comm, coords, dest);
  coords[direction] -= 2 * displ;
  mpi_cart_rank(comm, coords, source);
  free(coords);
  return MPI_SUCCESS;
}

int mpi_group_size(MPI_Group group, int*size)
{
  nm_mpi_group_t*p_group = nm_mpi_handle_group_get(&nm_mpi_groups, group);
  if(p_group == NULL)
    return MPI_ERR_GROUP;
  *size = nm_group_size(p_group->p_nm_group);
  return MPI_SUCCESS;
}

int mpi_group_rank(MPI_Group group, int*rank)
{
  nm_mpi_group_t*p_group = nm_mpi_handle_group_get(&nm_mpi_groups, group);
  if(p_group == NULL)
    return MPI_ERR_GROUP;
  const int r = nm_group_rank(p_group->p_nm_group);
  if(r == -1)
    {
      *rank = MPI_UNDEFINED;
    }
  else
    {
      *rank = r;
    }
  return MPI_SUCCESS;
}

int mpi_group_union(MPI_Group group1, MPI_Group group2, MPI_Group*newgroup)
{
  nm_mpi_group_t*p_group1 = nm_mpi_handle_group_get(&nm_mpi_groups, group1);
  nm_mpi_group_t*p_group2 = nm_mpi_handle_group_get(&nm_mpi_groups, group2);
  if((p_group1 == NULL) || (p_group2 == NULL))
    return MPI_ERR_GROUP;
  nm_mpi_group_t*p_new_group = nm_mpi_handle_group_alloc(&nm_mpi_groups);
  p_new_group->p_nm_group =  nm_group_union(p_group1->p_nm_group, p_group2->p_nm_group);
  MPI_Group new_id = p_new_group->id;
  *newgroup = new_id;
  return MPI_SUCCESS;  
}

int mpi_group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group*newgroup)
{
  nm_mpi_group_t*p_group1 = nm_mpi_handle_group_get(&nm_mpi_groups, group1);
  nm_mpi_group_t*p_group2 = nm_mpi_handle_group_get(&nm_mpi_groups, group2);
  if((p_group1 == NULL) || (p_group2 == NULL))
    return MPI_ERR_GROUP;
  nm_mpi_group_t*p_new_group = nm_mpi_handle_group_alloc(&nm_mpi_groups);
  p_new_group->p_nm_group =  nm_group_intersection(p_group1->p_nm_group, p_group2->p_nm_group);
  MPI_Group new_id = p_new_group->id;;
  *newgroup = new_id;
  return MPI_SUCCESS;  
}

int mpi_group_difference(MPI_Group group1, MPI_Group group2, MPI_Group*newgroup)
{
  nm_mpi_group_t*p_group1 = nm_mpi_handle_group_get(&nm_mpi_groups, group1);
  nm_mpi_group_t*p_group2 = nm_mpi_handle_group_get(&nm_mpi_groups, group2);
  if((p_group1 == NULL) || (p_group2 == NULL))
    return MPI_ERR_GROUP;
  nm_mpi_group_t*p_new_group = nm_mpi_handle_group_alloc(&nm_mpi_groups);
  p_new_group->p_nm_group = nm_group_difference(p_group1->p_nm_group, p_group2->p_nm_group);
  MPI_Group new_id = p_new_group->id;
  *newgroup = new_id;
  return MPI_SUCCESS;  
}

int mpi_group_compare(MPI_Group group1, MPI_Group group2, int*result)
{
  nm_mpi_group_t*p_group1 = nm_mpi_handle_group_get(&nm_mpi_groups, group1);
  nm_mpi_group_t*p_group2 = nm_mpi_handle_group_get(&nm_mpi_groups, group2);
  if((p_group1 == NULL) || (p_group2 == NULL))
    return MPI_ERR_GROUP;
  int r = nm_group_compare(p_group1->p_nm_group, p_group2->p_nm_group);
  if(r == NM_GROUP_IDENT)
    *result = MPI_IDENT;
  else if(r == NM_GROUP_SIMILAR)
    *result = MPI_SIMILAR;
  else
    *result = MPI_UNEQUAL;
  return MPI_SUCCESS;  
}

int mpi_group_incl(MPI_Group group, int n, int*ranks, MPI_Group*newgroup)
{
  if((n == 0) || (group == MPI_GROUP_EMPTY))
    {
      *newgroup = MPI_GROUP_EMPTY;
    }
  else
    {
      nm_mpi_group_t*p_group = nm_mpi_handle_group_get(&nm_mpi_groups, group);
      nm_mpi_group_t*p_new_group = nm_mpi_handle_group_alloc(&nm_mpi_groups);
      p_new_group->p_nm_group = nm_group_incl(p_group->p_nm_group, n, ranks);
      MPI_Group new_id = p_new_group->id;
      *newgroup = new_id;
    }
  return MPI_SUCCESS;
}

int mpi_group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group*newgroup)
{
  nm_mpi_group_t*p_group = nm_mpi_handle_group_get(&nm_mpi_groups, group);
  int size = nm_group_size(p_group->p_nm_group);
  int*ranks = malloc(sizeof(int) * size);
  int k = 0;
  int i;
  for(i = 0; i < n; i++)
    {
      const int first  = ranges[i][0];
      const int last   = ranges[i][1];
      const int stride = ranges[i][2];
      if(stride == 0)
	ERROR("stride = 0");
      const int e = (last - first) / stride;
      int j;
      for(j = 0; j <= e; j++)
	{
	  const int r = first + j * stride;
	  if(r >= 0 && r < size)
	    {
	      ranks[k] = r;
	      k++;
	    }
	}
    }
  int err = mpi_group_incl(group, k, ranks, newgroup);
  free(ranks);
  return err;
}

int mpi_group_excl(MPI_Group group, int n, int*ranks, MPI_Group*newgroup)
{
  if((n == 0) || (group == MPI_GROUP_EMPTY))
    {
      *newgroup = MPI_GROUP_EMPTY;
    }
  else
    {
      nm_mpi_group_t*p_group = nm_mpi_handle_group_get(&nm_mpi_groups, group);
      nm_mpi_group_t*p_new_group = nm_mpi_handle_group_alloc(&nm_mpi_groups);
      p_new_group->p_nm_group = nm_group_excl(p_group->p_nm_group, n, ranks);
      MPI_Group new_id = p_new_group->id;
      *newgroup = new_id;
    }
  return MPI_SUCCESS;
}

int mpi_group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group*newgroup)
{
  nm_mpi_group_t*p_group = nm_mpi_handle_group_get(&nm_mpi_groups, group);
  int size = nm_group_size(p_group->p_nm_group);
  int*ranks = malloc(sizeof(int) * size);
  int k = 0;
  int i;
  for(i = 0; i < n; i++)
    {
      const int first  = ranges[i][0];
      const int last   = ranges[i][1];
      const int stride = ranges[i][2];
      if(stride == 0)
	ERROR("stride = 0");
      const int e = (last - first) / stride;
      int j;
      for(j = 0; j <= e; j++)
	{
	  const int r = first + j * stride;
	  if(r >= 0 && r < size)
	    {
	      ranks[k] = r;
	      k++;
	    }
	}
    }
  int err = mpi_group_excl(group, k, ranks, newgroup);
  free(ranks);
  return err;
}


int mpi_group_free(MPI_Group*group)
{
  const int id = *group;
  if(id != MPI_GROUP_NULL)
    {
      nm_mpi_group_t*p_group = nm_mpi_handle_group_get(&nm_mpi_groups, id);
      nm_group_free(p_group->p_nm_group);
      nm_mpi_handle_group_free(&nm_mpi_groups, p_group);
      *group = MPI_GROUP_NULL;
    }
  return MPI_SUCCESS;
}

int mpi_group_translate_ranks(MPI_Group group1, int n, int *ranks1, MPI_Group group2, int *ranks2)
{
  nm_mpi_group_t*p_group1 = nm_mpi_handle_group_get(&nm_mpi_groups, group1);
  nm_mpi_group_t*p_group2 = nm_mpi_handle_group_get(&nm_mpi_groups, group2);
  int err = nm_group_translate_ranks(p_group1->p_nm_group, n, ranks1, p_group2->p_nm_group, ranks2);
  if(err != 0)
    return MPI_ERR_RANK;
  /* translate undefined ranks */
  int i;
  for(i = 0; i < n; i++)
    {
      if(ranks2[i] == -1)
	ranks2[i] = MPI_UNDEFINED;
    }
  return MPI_SUCCESS;
}



/* ********************************************************* */


int MPI_Comm_get_parent(MPI_Comm *parent)
{
  ERROR("<%s> not implemented\n", __FUNCTION__);
  if(parent)
    *parent = MPI_COMM_NULL;
  return MPI_SUCCESS;
}


int MPI_Comm_remote_size(MPI_Comm comm, int *size)
{
  ERROR("<%s> not implemented\n", __FUNCTION__);
  return MPI_ERR_UNKNOWN;
}
