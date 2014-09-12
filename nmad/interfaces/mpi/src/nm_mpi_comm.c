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


/* ********************************************************* */

int MPI_Comm_size(MPI_Comm comm,
		  int *size) __attribute__ ((alias ("mpi_comm_size")));

int MPI_Comm_rank(MPI_Comm comm,
                  int *rank) __attribute__ ((alias ("mpi_comm_rank")));

int MPI_Attr_get(MPI_Comm comm,
		 int keyval,
		 void *attr_value,
		 int *flag ) __attribute__ ((alias ("mpi_attr_get")));

int MPI_Comm_group(MPI_Comm comm,
		   MPI_Group *group) __attribute__ ((alias ("mpi_comm_group")));


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

int MPI_Group_translate_ranks(MPI_Group group1,
			      int n,
			      int *ranks1,
			      MPI_Group group2,
			      int *ranks2) __attribute__ ((alias ("mpi_group_translate_ranks")));

/* ********************************************************* */

static struct
{
  /** all the defined communicators */
  nm_mpi_communicator_t *communicators[NUMBER_OF_COMMUNICATORS];
  /** pool of ids of communicators that can be created by end-users */
  puk_int_vect_t communicators_pool;
} nm_mpi_communicators;

__PUK_SYM_INTERNAL
void nm_mpi_comm_init(void)
{
  int global_size  = -1;
  int process_rank = -1;
  nm_launcher_get_size(&global_size);
  nm_launcher_get_rank(&process_rank);

    /** Initialise data for communicators */
  nm_mpi_communicators.communicators[MPI_COMM_NULL] = NULL;

  nm_mpi_communicators.communicators[MPI_COMM_WORLD] = malloc(sizeof(nm_mpi_communicator_t));
  nm_mpi_communicators.communicators[MPI_COMM_WORLD]->communicator_id = MPI_COMM_WORLD;
  nm_mpi_communicators.communicators[MPI_COMM_WORLD]->size = global_size;
  nm_mpi_communicators.communicators[MPI_COMM_WORLD]->rank = process_rank;
  nm_mpi_communicators.communicators[MPI_COMM_WORLD]->global_ranks = malloc(global_size * sizeof(int));
  int i;
  for(i=0 ; i<global_size ; i++)
    {
      nm_mpi_communicators.communicators[MPI_COMM_WORLD]->global_ranks[i] = i;
    }

  nm_mpi_communicators.communicators[MPI_COMM_SELF] = malloc(sizeof(nm_mpi_communicator_t));
  nm_mpi_communicators.communicators[MPI_COMM_SELF]->communicator_id = MPI_COMM_SELF;
  nm_mpi_communicators.communicators[MPI_COMM_SELF]->size = 1;
  nm_mpi_communicators.communicators[MPI_COMM_SELF]->rank = process_rank;
  nm_mpi_communicators.communicators[MPI_COMM_SELF]->global_ranks = malloc(1 * sizeof(int));
  nm_mpi_communicators.communicators[MPI_COMM_SELF]->global_ranks[0] = process_rank;

  nm_mpi_communicators.communicators_pool = puk_int_vect_new();
  for(i = _MPI_COMM_OFFSET; i < NUMBER_OF_COMMUNICATORS; i++)
    {
      puk_int_vect_push_back(nm_mpi_communicators.communicators_pool, i);
    }
}

__PUK_SYM_INTERNAL
void nm_mpi_comm_exit(void)
{
  FREE_AND_SET_NULL(nm_mpi_communicators.communicators[MPI_COMM_WORLD]->global_ranks);
  FREE_AND_SET_NULL(nm_mpi_communicators.communicators[MPI_COMM_WORLD]);
  FREE_AND_SET_NULL(nm_mpi_communicators.communicators[MPI_COMM_SELF]->global_ranks);
  FREE_AND_SET_NULL(nm_mpi_communicators.communicators[MPI_COMM_SELF]);

  puk_int_vect_delete(nm_mpi_communicators.communicators_pool);
  nm_mpi_communicators.communicators_pool = NULL;
}

__PUK_SYM_INTERNAL
nm_gate_t nm_mpi_communicator_get_gate(nm_mpi_communicator_t*p_comm, int node)
{
  return nm_mpi_internal_data.gates[node];
}

__PUK_SYM_INTERNAL
int nm_mpi_communicator_get_dest(nm_mpi_communicator_t*p_comm, nm_gate_t gate)
{
  intptr_t rank_as_ptr = (intptr_t)puk_hashtable_lookup(nm_mpi_internal_data.dests, gate);
  return (rank_as_ptr - 1);
}

int mpi_comm_size(MPI_Comm comm, int *size)
{
  MPI_NMAD_LOG_IN();
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  *size = p_comm->size;
  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int mpi_comm_rank(MPI_Comm comm, int *rank)
{
  MPI_NMAD_LOG_IN();
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  *rank = p_comm->rank;
  MPI_NMAD_TRACE("My comm rank is %d\n", *rank);

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int mpi_attr_get(MPI_Comm comm, int keyval, void *attr_value, int *flag)
{
  int err = MPI_SUCCESS;
  switch(keyval)
    {
    case MPI_TAG_UB:
      *(int*)attr_value = MPI_NMAD_MAX_VALUE_TAG;
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
  MPI_NMAD_LOG_IN();
  *group = comm;
  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

static int nodecmp(const void *v1, const void *v2) 
{
  int **node1 = (int **)v1;
  int **node2 = (int **)v2;
  return ((*node1)[1] - (*node2)[1]);
}

int mpi_comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
  int *sendbuf, *recvbuf;
  int i, j, nb_conodes;
  int **conodes;
  nm_mpi_communicator_t *p_comm;
  nm_mpi_communicator_t *p_new_comm;

  MPI_NMAD_LOG_IN();

  p_comm = nm_mpi_communicator_get(comm);

  sendbuf = malloc(3*p_comm->size*sizeof(int));
  for(i=0 ; i<p_comm->size*3 ; i+=3) {
    sendbuf[i] = color;
    sendbuf[i+1] = key;
    sendbuf[i+2] = p_comm->global_ranks[p_comm->rank];
  }
  recvbuf = malloc(3*p_comm->size*sizeof(int));

#ifdef DEBUG
  MPI_NMAD_TRACE("[%d] Sending: ", p_comm->rank);
  for(i=0 ; i<p_comm->size*3 ; i++) {
    MPI_NMAD_TRACE("%d ", sendbuf[i]);
  }
  MPI_NMAD_TRACE("\n");
#endif /* DEBUG */

  mpi_alltoall(sendbuf, 3, MPI_INT, recvbuf, 3, MPI_INT, comm);

#ifdef DEBUG
  MPI_NMAD_TRACE("[%d] Received: ", p_comm->rank);
  for(i=0 ; i<p_comm->size*3 ; i++) {
    MPI_NMAD_TRACE("%d ", recvbuf[i]);
  }
  MPI_NMAD_TRACE("\n");
#endif /* DEBUG */

  // Counting how many nodes have the same color
  nb_conodes=0;
  for(i=0 ; i<p_comm->size*3 ; i+=3) {
    if (recvbuf[i] == color) {
      nb_conodes++;
    }
  }

  // Accumulating the nodes with the same color into an array
  conodes = malloc(nb_conodes*sizeof(int*));
  j=0;
  for(i=0 ; i<p_comm->size*3 ; i+=3) {
    if (recvbuf[i] == color) {
      conodes[j] = &recvbuf[i];
      j++;
    }
  }

  // Sorting the nodes with the same color according to their key
  qsort(conodes, nb_conodes, sizeof(int *), &nodecmp);
  
#ifdef DEBUG
  MPI_NMAD_TRACE("[%d] Conodes: ", p_comm->rank);
  for(i=0 ; i<nb_conodes ; i++) {
    int *ptr = conodes[i];
    MPI_NMAD_TRACE("[%d %d %d] ", *ptr, *(ptr+1), *(ptr+2));
  }
  MPI_NMAD_TRACE("\n");
#endif /* DEBUG */

  // Create the new communicator
  if (color == MPI_UNDEFINED) {
    *newcomm = MPI_COMM_NULL;
  }
  else {
    mpi_comm_dup(comm, newcomm);
    p_new_comm = nm_mpi_communicator_get(*newcomm);
    p_new_comm->size = nb_conodes;
    FREE_AND_SET_NULL(p_new_comm->global_ranks);
    p_new_comm->global_ranks = malloc(nb_conodes*sizeof(int));
    for(i=0 ; i<nb_conodes ; i++) {
      int *ptr = conodes[i];
      if ((*(ptr+1) == key) && (*(ptr+2) == p_comm->global_ranks[p_comm->rank])) {
        p_new_comm->rank = i;
      }
      if (*ptr == color) {
        p_new_comm->global_ranks[i] = *(ptr+2);
      }
    }
  }

  // free the allocated area
  FREE_AND_SET_NULL(conodes);
  FREE_AND_SET_NULL(sendbuf);
  FREE_AND_SET_NULL(recvbuf);

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int mpi_comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
  if (puk_int_vect_empty(nm_mpi_communicators.communicators_pool))
    {
      ERROR("Maximum number of communicators created");
      return MPI_ERR_INTERN;
    }
  else if (nm_mpi_communicators.communicators[comm] == NULL)
    {
      ERROR("Communicator %d is not valid", comm);
      return MPI_ERR_OTHER;
    }
  else
    {
    int i;
    *newcomm = puk_int_vect_pop_back(nm_mpi_communicators.communicators_pool);

    nm_mpi_communicators.communicators[*newcomm] = malloc(sizeof(nm_mpi_communicator_t));
    nm_mpi_communicators.communicators[*newcomm]->communicator_id = *newcomm;
    nm_mpi_communicators.communicators[*newcomm]->size = nm_mpi_communicators.communicators[comm]->size;
    nm_mpi_communicators.communicators[*newcomm]->rank = nm_mpi_communicators.communicators[comm]->rank;
    nm_mpi_communicators.communicators[*newcomm]->global_ranks = malloc(nm_mpi_communicators.communicators[*newcomm]->size * sizeof(int));
    for(i=0 ; i<nm_mpi_communicators.communicators[*newcomm]->size ; i++) {
      nm_mpi_communicators.communicators[*newcomm]->global_ranks[i] = nm_mpi_communicators.communicators[comm]->global_ranks[i];
    }

    return MPI_SUCCESS;
  }
}

int mpi_comm_free(MPI_Comm *comm)
{
  if (*comm == MPI_COMM_WORLD)
    {
      ERROR("Cannot free communicator MPI_COMM_WORLD");
      return MPI_ERR_OTHER;
    }
  else if (*comm == MPI_COMM_SELF)
    {
      ERROR("Cannot free communicator MPI_COMM_SELF");
      return MPI_ERR_OTHER;
    }
  else if (*comm <= 0 || *comm >= NUMBER_OF_COMMUNICATORS || nm_mpi_communicators.communicators[*comm] == NULL)
    {
      ERROR("Communicator %d unknown\n", *comm);
      return MPI_ERR_OTHER;
    }
  else 
    {
      free(nm_mpi_communicators.communicators[*comm]->global_ranks);
      free(nm_mpi_communicators.communicators[*comm]);
      nm_mpi_communicators.communicators[*comm] = NULL;
      puk_int_vect_push_back(nm_mpi_communicators.communicators_pool, *comm);
      *comm = MPI_COMM_NULL;
      return MPI_SUCCESS;
    }
}

int mpi_cart_create(MPI_Comm comm_old, int ndims, int*dims, int*periods, int reorder, MPI_Comm*_comm_cart)
{
  MPI_Comm comm_cart = MPI_COMM_NULL;
  int err = MPI_Comm_dup(comm_old, &comm_cart);
  if(err != MPI_SUCCESS)
    {
      return err;
    }
  nm_mpi_communicator_t*mpir_comm_cart = nm_mpi_communicator_get(comm_cart);
  struct nm_mpi_cart_topology_s*cart = &mpir_comm_cart->cart_topology;
  if(ndims < 0)
    {
      return MPI_ERR_DIMS;
    }
  cart->ndims = ndims;
  cart->dims = malloc(ndims * sizeof(int));
  cart->periods = malloc(ndims * sizeof(int));
  cart->size = 1;
  int d;
  for(d = 0; d < ndims; d++)
    {
      if(dims[d] <= 0)
	{
	  return MPI_ERR_DIMS;
	}
      cart->dims[d] = dims[d];
      cart->periods[d] = periods[d];
      cart->size *= dims[d];
    }
  if(cart->size > mpir_comm_cart->size)
    return MPI_ERR_TOPOLOGY;
  if(mpir_comm_cart->rank >= cart->size)
    mpir_comm_cart->rank = MPI_PROC_NULL;
  *_comm_cart = comm_cart;
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
  nm_mpi_communicator_t*mpir_comm_cart = nm_mpi_communicator_get(comm);
  struct nm_mpi_cart_topology_s*cart = &mpir_comm_cart->cart_topology;
  const int rank = mpir_comm_cart->rank;
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

int mpi_group_translate_ranks(MPI_Group group1, int n, int *ranks1, MPI_Group group2, int *ranks2)
{
  int i, j, x;
  nm_mpi_communicator_t *p_comm;
  nm_mpi_communicator_t *p_comm2;

  MPI_NMAD_LOG_IN();

  p_comm = nm_mpi_communicator_get(group1);
  p_comm2 = nm_mpi_communicator_get(group2);
  for(i=0 ; i<n ; i++) {
    ranks2[i] = MPI_UNDEFINED;
    if (ranks1[i] < p_comm->size) {
      x = p_comm->global_ranks[ranks1[i]];
      for(j=0 ; j<p_comm2->size ; j++) {
        if (p_comm2->global_ranks[j] == x) {
          ranks2[i] = j;
          break;
        }
      }
    }
    else {
      return MPI_ERR_RANK;
    }
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}



nm_mpi_communicator_t*nm_mpi_communicator_get(MPI_Comm comm)
{
  if(comm > 0 && comm < NUMBER_OF_COMMUNICATORS)
    {
      if (nm_mpi_communicators.communicators[comm] == NULL) 
	{
	  ERROR("Communicator %d invalid", comm);
	  return NULL;
	}
      else 
	{
	  return nm_mpi_communicators.communicators[comm];
	}
    }
  else
    {
      ERROR("Communicator %d unknown", comm);
      return NULL;
    }
}

nm_tag_t mpir_comm_and_tag(nm_mpi_communicator_t *p_comm, int tag)
{
  /*
   * The communicator is represented on 5 bits, we left shift the tag
   * value by 5 bits to add the communicator id.
   * Let's suppose no significant bit is going to get lost in the tag
   * value.
   */
  nm_tag_t newtag = tag << 5;
  newtag += (p_comm->communicator_id);
  return newtag;
}

/* ********************************************************* */

int MPI_Comm_spawn(char *command,
		   char *argv[],
		   int maxprocs,
		   MPI_Info info,
		   int root,
		   MPI_Comm comm,
		   MPI_Comm *intercomm,
		   int array_of_errcodes[])
{
  ERROR("<%s> not implemented\n", __FUNCTION__);
  return MPI_ERR_UNKNOWN;
}


int MPI_Comm_get_parent(MPI_Comm *parent)
{
  if(parent)
    *parent = MPI_COMM_NULL;
  return MPI_SUCCESS;
}


int MPI_Comm_remote_size(MPI_Comm comm, int *size)
{
  ERROR("<%s> not implemented\n", __FUNCTION__);
  return MPI_ERR_UNKNOWN;
}
