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

int mpi_comm_size(MPI_Comm comm, int *size)
{
  MPI_NMAD_LOG_IN();
  nm_mpi_communicator_t *mpir_communicator = nm_mpi_communicator_get(comm);
  *size = mpir_communicator->size;
  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int mpi_comm_rank(MPI_Comm comm, int *rank)
{
  MPI_NMAD_LOG_IN();
  nm_mpi_communicator_t *mpir_communicator = nm_mpi_communicator_get(comm);
  *rank = mpir_communicator->rank;
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
  nm_mpi_communicator_t *mpir_communicator;
  nm_mpi_communicator_t *mpir_newcommunicator;

  MPI_NMAD_LOG_IN();

  mpir_communicator = nm_mpi_communicator_get(comm);

  sendbuf = malloc(3*mpir_communicator->size*sizeof(int));
  for(i=0 ; i<mpir_communicator->size*3 ; i+=3) {
    sendbuf[i] = color;
    sendbuf[i+1] = key;
    sendbuf[i+2] = mpir_communicator->global_ranks[mpir_communicator->rank];
  }
  recvbuf = malloc(3*mpir_communicator->size*sizeof(int));

#ifdef DEBUG
  MPI_NMAD_TRACE("[%d] Sending: ", mpir_communicator->rank);
  for(i=0 ; i<mpir_communicator->size*3 ; i++) {
    MPI_NMAD_TRACE("%d ", sendbuf[i]);
  }
  MPI_NMAD_TRACE("\n");
#endif /* DEBUG */

  mpi_alltoall(sendbuf, 3, MPI_INT, recvbuf, 3, MPI_INT, comm);

#ifdef DEBUG
  MPI_NMAD_TRACE("[%d] Received: ", mpir_communicator->rank);
  for(i=0 ; i<mpir_communicator->size*3 ; i++) {
    MPI_NMAD_TRACE("%d ", recvbuf[i]);
  }
  MPI_NMAD_TRACE("\n");
#endif /* DEBUG */

  // Counting how many nodes have the same color
  nb_conodes=0;
  for(i=0 ; i<mpir_communicator->size*3 ; i+=3) {
    if (recvbuf[i] == color) {
      nb_conodes++;
    }
  }

  // Accumulating the nodes with the same color into an array
  conodes = malloc(nb_conodes*sizeof(int*));
  j=0;
  for(i=0 ; i<mpir_communicator->size*3 ; i+=3) {
    if (recvbuf[i] == color) {
      conodes[j] = &recvbuf[i];
      j++;
    }
  }

  // Sorting the nodes with the same color according to their key
  qsort(conodes, nb_conodes, sizeof(int *), &nodecmp);
  
#ifdef DEBUG
  MPI_NMAD_TRACE("[%d] Conodes: ", mpir_communicator->rank);
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
    mpir_newcommunicator = nm_mpi_communicator_get(*newcomm);
    mpir_newcommunicator->size = nb_conodes;
    FREE_AND_SET_NULL(mpir_newcommunicator->global_ranks);
    mpir_newcommunicator->global_ranks = malloc(nb_conodes*sizeof(int));
    for(i=0 ; i<nb_conodes ; i++) {
      int *ptr = conodes[i];
      if ((*(ptr+1) == key) && (*(ptr+2) == mpir_communicator->global_ranks[mpir_communicator->rank])) {
        mpir_newcommunicator->rank = i;
      }
      if (*ptr == color) {
        mpir_newcommunicator->global_ranks[i] = *(ptr+2);
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
  if (puk_int_vect_empty(nm_mpi_internal_data.communicators_pool))
    {
      ERROR("Maximum number of communicators created");
      return MPI_ERR_INTERN;
    }
  else if (nm_mpi_internal_data.communicators[comm] == NULL)
    {
      ERROR("Communicator %d is not valid", comm);
      return MPI_ERR_OTHER;
    }
  else
    {
    int i;
    *newcomm = puk_int_vect_pop_back(nm_mpi_internal_data.communicators_pool);

    nm_mpi_internal_data.communicators[*newcomm] = malloc(sizeof(nm_mpi_communicator_t));
    nm_mpi_internal_data.communicators[*newcomm]->communicator_id = *newcomm;
    nm_mpi_internal_data.communicators[*newcomm]->size = nm_mpi_internal_data.communicators[comm]->size;
    nm_mpi_internal_data.communicators[*newcomm]->rank = nm_mpi_internal_data.communicators[comm]->rank;
    nm_mpi_internal_data.communicators[*newcomm]->global_ranks = malloc(nm_mpi_internal_data.communicators[*newcomm]->size * sizeof(int));
    for(i=0 ; i<nm_mpi_internal_data.communicators[*newcomm]->size ; i++) {
      nm_mpi_internal_data.communicators[*newcomm]->global_ranks[i] = nm_mpi_internal_data.communicators[comm]->global_ranks[i];
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
  else if (*comm <= 0 || *comm >= NUMBER_OF_COMMUNICATORS || nm_mpi_internal_data.communicators[*comm] == NULL)
    {
      ERROR("Communicator %d unknown\n", *comm);
      return MPI_ERR_OTHER;
    }
  else 
    {
      free(nm_mpi_internal_data.communicators[*comm]->global_ranks);
      free(nm_mpi_internal_data.communicators[*comm]);
      nm_mpi_internal_data.communicators[*comm] = NULL;
      puk_int_vect_push_back(nm_mpi_internal_data.communicators_pool, *comm);
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
  nm_mpi_communicator_t *mpir_communicator;
  nm_mpi_communicator_t *mpir_communicator2;

  MPI_NMAD_LOG_IN();

  mpir_communicator = nm_mpi_communicator_get(group1);
  mpir_communicator2 = nm_mpi_communicator_get(group2);
  for(i=0 ; i<n ; i++) {
    ranks2[i] = MPI_UNDEFINED;
    if (ranks1[i] < mpir_communicator->size) {
      x = mpir_communicator->global_ranks[ranks1[i]];
      for(j=0 ; j<mpir_communicator2->size ; j++) {
        if (mpir_communicator2->global_ranks[j] == x) {
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
      if (nm_mpi_internal_data.communicators[comm] == NULL) 
	{
	  ERROR("Communicator %d invalid", comm);
	  return NULL;
	}
      else 
	{
	  return nm_mpi_internal_data.communicators[comm];
	}
    }
  else
    {
      ERROR("Communicator %d unknown", comm);
      return NULL;
    }
}

nm_tag_t mpir_comm_and_tag(nm_mpi_communicator_t *mpir_communicator, int tag)
{
  /*
   * The communicator is represented on 5 bits, we left shift the tag
   * value by 5 bits to add the communicator id.
   * Let's suppose no significant bit is going to get lost in the tag
   * value.
   */
  nm_tag_t newtag = tag << 5;
  newtag += (mpir_communicator->communicator_id);
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
