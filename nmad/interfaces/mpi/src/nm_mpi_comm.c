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


/** subroutine types for FORTRAN binding */
typedef void (nm_mpi_copy_subroutine_t)(MPI_Comm*oldcomm, int*keyval, void*extra_state, void*attribute_val_in, void*attribute_val_out, int*flag, int*ierr);
typedef void (nm_mpi_delete_subroutine_t)(MPI_Comm*comm, int*keyval, void*attribute_val, void*extra_state, int*ierr);

struct nm_mpi_keyval_s
{
  int id;
  MPI_Copy_function*copy_fn;
  MPI_Delete_function*delete_fn;
  nm_mpi_copy_subroutine_t*copy_subroutine;
  nm_mpi_delete_subroutine_t*delete_subroutine;
  void*extra_state;
};

NM_MPI_HANDLE_TYPE(communicator, nm_mpi_communicator_t, _NM_MPI_COMM_OFFSET, 16); 
NM_MPI_HANDLE_TYPE(group, nm_mpi_group_t, _NM_MPI_GROUP_OFFSET, 16); 
NM_MPI_HANDLE_TYPE(keyval, struct nm_mpi_keyval_s, _NM_MPI_ATTR_OFFSET, 16);

static struct nm_mpi_handle_communicator_s nm_mpi_communicators;
static struct nm_mpi_handle_group_s nm_mpi_groups;
static struct nm_mpi_handle_keyval_s nm_mpi_keyvals;

/* ********************************************************* */

NM_MPI_ALIAS(MPI_Comm_size,             mpi_comm_size);
NM_MPI_ALIAS(MPI_Comm_rank,             mpi_comm_rank);
NM_MPI_ALIAS(MPI_Keyval_create,         mpi_keyval_create);
NM_MPI_ALIAS(MPI_Keyval_free,           mpi_keyval_free);
NM_MPI_ALIAS(MPI_Attr_put,              mpi_attr_put);
NM_MPI_ALIAS(MPI_Attr_get,              mpi_attr_get);
NM_MPI_ALIAS(MPI_Attr_delete,           mpi_attr_delete);
NM_MPI_ALIAS(MPI_Comm_group,            mpi_comm_group);
NM_MPI_ALIAS(MPI_Comm_create,           mpi_comm_create);
NM_MPI_ALIAS(MPI_Comm_split,            mpi_comm_split);
NM_MPI_ALIAS(MPI_Comm_dup,              mpi_comm_dup);
NM_MPI_ALIAS(MPI_Comm_free,             mpi_comm_free);
NM_MPI_ALIAS(MPI_Comm_compare,          mpi_comm_compare);
NM_MPI_ALIAS(MPI_Comm_test_inter,       mpi_comm_test_inter);
NM_MPI_ALIAS(MPI_Comm_set_name,         mpi_comm_set_name);
NM_MPI_ALIAS(MPI_Comm_get_name,         mpi_comm_get_name);
NM_MPI_ALIAS(MPI_Cart_create,           mpi_cart_create);
NM_MPI_ALIAS(MPI_Cart_coords,           mpi_cart_coords)
NM_MPI_ALIAS(MPI_Cart_rank,             mpi_cart_rank);
NM_MPI_ALIAS(MPI_Cart_shift,            mpi_cart_shift);
NM_MPI_ALIAS(MPI_Cart_get,              mpi_cart_get);
NM_MPI_ALIAS(MPI_Group_size,            mpi_group_size);
NM_MPI_ALIAS(MPI_Group_rank,            mpi_group_rank);
NM_MPI_ALIAS(MPI_Group_union,           mpi_group_union);
NM_MPI_ALIAS(MPI_Group_intersection,    mpi_group_intersection);
NM_MPI_ALIAS(MPI_Group_difference,      mpi_group_difference);
NM_MPI_ALIAS(MPI_Group_compare,         mpi_group_compare);
NM_MPI_ALIAS(MPI_Group_incl,            mpi_group_incl);
NM_MPI_ALIAS(MPI_Group_range_incl,      mpi_group_range_incl);
NM_MPI_ALIAS(MPI_Group_excl,            mpi_group_excl);
NM_MPI_ALIAS(MPI_Group_range_excl,      mpi_group_range_excl);
NM_MPI_ALIAS(MPI_Group_free,            mpi_group_free);
NM_MPI_ALIAS(MPI_Group_translate_ranks, mpi_group_translate_ranks);

/* ********************************************************* */

__PUK_SYM_INTERNAL
void nm_mpi_comm_init(void)
{
  /* handles allocator */
  nm_mpi_handle_communicator_init(&nm_mpi_communicators);
  nm_mpi_handle_group_init(&nm_mpi_groups);
  nm_mpi_handle_keyval_init(&nm_mpi_keyvals);

  /* built-in communicators */
  nm_mpi_communicator_t*p_comm_world = nm_mpi_handle_communicator_store(&nm_mpi_communicators, MPI_COMM_WORLD);
  p_comm_world->p_comm = nm_comm_world();
  p_comm_world->attrs = NULL;
  p_comm_world->p_errhandler = nm_mpi_errhandler_get(MPI_ERRORS_ARE_FATAL);
  p_comm_world->name = strdup("MPI_COMM_WORLD");
  nm_mpi_communicator_t*p_comm_self = nm_mpi_handle_communicator_store(&nm_mpi_communicators, MPI_COMM_SELF);
  p_comm_self->p_comm = nm_comm_self();
  p_comm_self->attrs = NULL;
  p_comm_self->p_errhandler = nm_mpi_errhandler_get(MPI_ERRORS_ARE_FATAL);
  p_comm_self->name = strdup("MPI_COMM_SELF");

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

  nm_mpi_handle_keyval_finalize(&nm_mpi_keyvals);
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

static inline nm_mpi_communicator_t*nm_mpi_communicator_alloc(nm_comm_t p_nm_comm, struct nm_mpi_errhandler_s*p_errhandler)
{
  nm_mpi_communicator_t*p_new_comm = nm_mpi_handle_communicator_alloc(&nm_mpi_communicators);
  p_new_comm->p_comm = p_nm_comm;
  p_new_comm->attrs = NULL;
  p_new_comm->p_errhandler = p_errhandler;
  p_new_comm->name = NULL;
  return p_new_comm;
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

/** return new attribute table from old communicator */
static int nm_mpi_comm_attrs_copy(nm_mpi_communicator_t*p_old_comm, puk_hashtable_t*p_attrs)
{
  if(p_old_comm->attrs)
    {
      puk_hashtable_enumerator_t e = puk_hashtable_enumerator_new(p_old_comm->attrs);
      struct nm_mpi_keyval_s*p_keyval = NULL;
      void*p_value = NULL;
      do
	{
	  puk_hashtable_enumerator_next2(e, &p_keyval, &p_value);
	  if((p_keyval != NULL) && (p_keyval->copy_fn != MPI_NULL_COPY_FN))
	    { 
	      int flag = 0;
	      void*v = puk_hashtable_lookup(p_old_comm->attrs, p_keyval);
	      void*out = NULL;
	      int err = MPI_SUCCESS;
	      if(p_keyval->copy_fn != NULL)
		{
		  err = (p_keyval->copy_fn)(p_old_comm->id, p_keyval->id, p_keyval->extra_state, &v, &out, &flag);
		}
	      else if(p_keyval->copy_subroutine != NULL)
		{
		  (p_keyval->copy_subroutine)(&p_old_comm->id, &p_keyval->id, p_keyval->extra_state, &v, &out, &flag, &err);
		}
	      if(err != MPI_SUCCESS)
		{
		  fprintf(stderr, "# madmpi: error while copying attribute.\n");
		  return err;
		}
	      if(flag)
		{
		  if(*p_attrs == NULL)
		    *p_attrs = puk_hashtable_new_ptr();
		  puk_hashtable_insert(*p_attrs, p_keyval, out);
		}
	    }
	}
      while(p_keyval != NULL);
      puk_hashtable_enumerator_delete(e);
    }
  return MPI_SUCCESS;
}

static int nm_mpi_comm_attr_delete(nm_mpi_communicator_t*p_comm, struct nm_mpi_keyval_s*p_keyval)
{
  int err = MPI_SUCCESS;
  if(p_comm->attrs != NULL && puk_hashtable_probe(p_comm->attrs, p_keyval))
    {
      void*v = puk_hashtable_lookup(p_comm->attrs, p_keyval);
      if(p_keyval->delete_fn != NULL)
	{
	  err = (p_keyval->delete_fn)(p_comm->id, p_keyval->id, &v, p_keyval->extra_state);
	}
      else if(p_keyval->delete_subroutine != NULL)
	{
	  (p_keyval->delete_subroutine)(&p_comm->id, &p_keyval->id, &v, p_keyval->extra_state, &err);
	}
      if(err == MPI_SUCCESS)
	{
	  puk_hashtable_remove(p_comm->attrs, p_keyval);
	}
    }
  return err;
}

/** FORTRAN binding for MPI_Keyval_create */
void mpi_keyval_create_(void*copy_subroutine, void*delete_subroutine, int*keyval, void*extra_state, int*ierr)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_handle_keyval_alloc(&nm_mpi_keyvals);
  p_keyval->copy_fn = NULL;;
  p_keyval->delete_fn = NULL;
  p_keyval->copy_subroutine = copy_subroutine;
  p_keyval->delete_subroutine = delete_subroutine;
  p_keyval->extra_state = extra_state;
  *keyval = p_keyval->id;
  *ierr = MPI_SUCCESS;
}

int mpi_keyval_create(MPI_Copy_function*copy_fn, MPI_Delete_function*delete_fn, int*keyval, void*extra_state)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_handle_keyval_alloc(&nm_mpi_keyvals);
  p_keyval->copy_fn = copy_fn;
  p_keyval->delete_fn = delete_fn;
  p_keyval->copy_subroutine = NULL;
  p_keyval->delete_subroutine = NULL;
  p_keyval->extra_state = extra_state;
  *keyval = p_keyval->id;
  return MPI_SUCCESS;
}

int mpi_keyval_free(int*keyval)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_handle_keyval_get(&nm_mpi_keyvals, *keyval);
  if(p_keyval == NULL)
    return MPI_ERR_KEYVAL;
  nm_mpi_handle_keyval_free(&nm_mpi_keyvals, p_keyval);
  *keyval = MPI_KEYVAL_INVALID;
  return MPI_SUCCESS;
}

int mpi_attr_put(MPI_Comm comm, int keyval, void*attr_value)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_handle_keyval_get(&nm_mpi_keyvals, keyval);
  if(p_keyval == NULL)
    return MPI_ERR_KEYVAL;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    return MPI_ERR_COMM;
  if(p_comm->attrs == NULL)
    {
      p_comm->attrs = puk_hashtable_new_ptr();
    }
  if(puk_hashtable_probe(p_comm->attrs, p_keyval))
    {
      int err = nm_mpi_comm_attr_delete(p_comm, p_keyval);
      if(err != MPI_SUCCESS)
	return err;
    }
  puk_hashtable_insert(p_comm->attrs, p_keyval, attr_value);
  return MPI_SUCCESS;
}

int mpi_attr_get(MPI_Comm comm, int keyval, void*attr_value, int*flag)
{
  int err = MPI_SUCCESS;
  const static int nm_mpi_tag_ub = NM_MPI_TAG_MAX;
  const static int nm_mpi_wtime_is_global = 0;
  const static int nm_mpi_host = MPI_PROC_NULL;
#ifdef NM_MPI_ENABLE_ROMIO
  const static int nm_mpi_attr_io = 1;
#else
  const static int nm_mpi_attr_io = 0;
#endif
  switch(keyval)
    {
    case MPI_TAG_UB:
      *(const int**)attr_value = &nm_mpi_tag_ub;
      *flag = 1;
      break;
    case MPI_HOST:
      *(const int**)attr_value = &nm_mpi_host;
      *flag = 1;
      break;
    case MPI_IO:
      *(const int**)attr_value = &nm_mpi_attr_io;
      *flag = 1;
      break;
    case MPI_WTIME_IS_GLOBAL:
      *(const int**)attr_value = &nm_mpi_wtime_is_global;
      *flag = 1;
      break;
    default:
      {
	struct nm_mpi_keyval_s*p_keyval = nm_mpi_handle_keyval_get(&nm_mpi_keyvals, keyval);
	if(p_keyval == NULL)
	  return MPI_ERR_KEYVAL;
	nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
	if(p_comm == NULL)
	  return MPI_ERR_COMM;
	*flag = 0;
	if(p_comm->attrs != NULL)
	  {
	    void*v = puk_hashtable_lookup(p_comm->attrs, p_keyval);
	    if(v != NULL)
	      {
		void**_value = attr_value;
		*_value = v;
		*flag = 1;
	      }
	  }
      }
      break;
    }
  return err;
}

int mpi_attr_delete(MPI_Comm comm, int keyval)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_handle_keyval_get(&nm_mpi_keyvals, keyval);
  if(p_keyval == NULL)
    return MPI_ERR_KEYVAL;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    return MPI_ERR_COMM;
  int err = nm_mpi_comm_attr_delete(p_comm, p_keyval);
  return err;
}

int mpi_comm_set_name(MPI_Comm comm, char*comm_name)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    return MPI_ERR_COMM;
  if(p_comm->name != NULL)
    {
      free(p_comm->name);
    }
  p_comm->name = strndup(comm_name, MPI_MAX_OBJECT_NAME);
  return MPI_SUCCESS;
}

int mpi_comm_get_name(MPI_Comm comm, char*comm_name, int*resultlen)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    return MPI_ERR_COMM;
  if(p_comm->name != NULL)
    {
      strncpy(comm_name, p_comm->name, MPI_MAX_OBJECT_NAME);
    }
  else
    {
      comm_name[0] = '\0';
    }
  *resultlen = strlen(comm_name);
  return MPI_SUCCESS;
}

int mpi_comm_test_inter(MPI_Comm comm, int*flag)
{
  /* intercommunicator not supported yet. */
  *flag = 0;
  return MPI_SUCCESS;
}

int mpi_comm_compare(MPI_Comm comm1, MPI_Comm comm2, int*result)
{
  nm_mpi_communicator_t*p_comm1 = nm_mpi_communicator_get(comm1);
  nm_mpi_communicator_t*p_comm2 = nm_mpi_communicator_get(comm2);
  if(p_comm1 == NULL || p_comm2 == NULL)
    return MPI_ERR_COMM;
  if(comm1 == comm2)
    {
      *result = MPI_IDENT;
      return MPI_SUCCESS;
    }
  int r = nm_group_compare(nm_comm_group(p_comm1->p_comm), nm_comm_group(p_comm2->p_comm));
  if(r == NM_GROUP_IDENT)
    *result = MPI_CONGRUENT;
  else if(r == NM_GROUP_SIMILAR)
    *result = MPI_SIMILAR;
  else
    *result = MPI_UNEQUAL;
  return MPI_SUCCESS;
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
  if(group == MPI_GROUP_NULL)
    return MPI_ERR_GROUP;
  nm_mpi_group_t*p_new_group = nm_mpi_handle_group_get(&nm_mpi_groups, group);
  if(p_new_group == NULL)
    return MPI_ERR_GROUP;
  if(nm_group_size(p_new_group->p_nm_group) == 0)
    {
      *newcomm = MPI_COMM_NULL;
      return MPI_SUCCESS;
    }
  nm_comm_t p_nm_comm = nm_comm_create(p_old_comm->p_comm, p_new_group->p_nm_group);
  if(p_nm_comm == NULL)
    return MPI_ERR_COMM;
  nm_mpi_communicator_t*p_new_comm = nm_mpi_communicator_alloc(p_nm_comm, p_old_comm->p_errhandler);
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
	      nm_mpi_communicator_t*p_new_comm = nm_mpi_communicator_alloc(p_nm_comm, p_old_comm->p_errhandler);
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
      nm_mpi_communicator_t*p_new_comm = nm_mpi_communicator_alloc(nm_comm_dup(p_old_comm->p_comm), p_old_comm->p_errhandler);
      int err = nm_mpi_comm_attrs_copy(p_old_comm, &p_new_comm->attrs);
      if(err)
	{
	  *newcomm = MPI_COMM_NULL;
	  return err;
	}
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
  nm_mpi_communicator_t*p_new_comm = nm_mpi_communicator_alloc(nm_comm_create(p_old_comm->p_comm, cart_group), p_old_comm->p_errhandler);
  p_new_comm->cart_topology = cart;
  *newcomm = p_new_comm->id;
  nm_group_free(cart_group);
  return MPI_SUCCESS;
}

int mpi_cart_coords(MPI_Comm comm, int rank, int ndims, int*coords)
{
  nm_mpi_communicator_t*p_comm_cart = nm_mpi_communicator_get(comm);
  struct nm_mpi_cart_topology_s*cart = &p_comm_cart->cart_topology;
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
  nm_mpi_communicator_t*p_comm_cart = nm_mpi_communicator_get(comm);
  struct nm_mpi_cart_topology_s*cart = &p_comm_cart->cart_topology;
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

int mpi_cart_get(MPI_Comm comm, int maxdims, int*dims, int*periods, int*coords)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  const struct nm_mpi_cart_topology_s*cart = &p_comm->cart_topology;
  if(maxdims < cart->ndims)
    return MPI_ERR_DIMS;
  int i;
  for(i = 0; i < cart->ndims; i++)
    {
      dims[i] = cart->dims[i];
      periods[i] = cart->periods[i];
    }
  const int rank = nm_comm_rank(p_comm->p_comm);
  mpi_cart_coords(comm, rank, maxdims, coords);
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

int MPI_Intercomm_create(MPI_Comm local_comm,
			 int local_leader,
			 MPI_Comm peer_comm,
			 int remote_leader,
			 int tag,
			 MPI_Comm *newintercomm)
{
  ERROR("<%s> not implemented\n", __FUNCTION__);
  return MPI_ERR_UNKNOWN;
}
