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

#include "nm_mpi_private.h"
#include <assert.h>
#include <errno.h>

#include <Padico/Module.h>
PADICO_MODULE_HOOK(MadMPI);


NM_MPI_HANDLE_TYPE(communicator, nm_mpi_communicator_t, _NM_MPI_COMM_OFFSET, 16); 
NM_MPI_HANDLE_TYPE(group, nm_mpi_group_t, _NM_MPI_GROUP_OFFSET, 16); 

static struct nm_mpi_handle_communicator_s nm_mpi_communicators;
static struct nm_mpi_handle_group_s nm_mpi_groups;

static void nm_mpi_communicator_destroy(nm_mpi_communicator_t*p_comm);
static void nm_mpi_group_destroy(nm_mpi_group_t*p_group);

static nm_mpi_communicator_t*nm_mpi_intercomm_create(nm_mpi_communicator_t*p_parent_comm, nm_group_t p_local_group,
						     nm_gate_t p_local_leader, nm_gate_t p_remote_leader, nm_tag_t tag);

/* ********************************************************* */

NM_MPI_ALIAS(MPI_Comm_size,             mpi_comm_size);
NM_MPI_ALIAS(MPI_Comm_rank,             mpi_comm_rank);
NM_MPI_ALIAS(MPI_Keyval_create,         mpi_keyval_create);
NM_MPI_ALIAS(MPI_Keyval_free,           mpi_keyval_free);
NM_MPI_ALIAS(MPI_Attr_put,              mpi_attr_put);
NM_MPI_ALIAS(MPI_Attr_get,              mpi_attr_get);
NM_MPI_ALIAS(MPI_Attr_delete,           mpi_attr_delete);
NM_MPI_ALIAS(MPI_Comm_create_keyval,    mpi_comm_create_keyval);
NM_MPI_ALIAS(MPI_Comm_free_keyval,      mpi_comm_free_keyval);
NM_MPI_ALIAS(MPI_Comm_get_attr,         mpi_comm_get_attr);
NM_MPI_ALIAS(MPI_Comm_set_attr,         mpi_comm_set_attr);
NM_MPI_ALIAS(MPI_Comm_delete_attr,      mpi_comm_delete_attr);
NM_MPI_ALIAS(MPI_Comm_group,            mpi_comm_group);
NM_MPI_ALIAS(MPI_Comm_create,           mpi_comm_create);
NM_MPI_ALIAS(MPI_Comm_create_group,     mpi_comm_create_group);
NM_MPI_ALIAS(MPI_Comm_split,            mpi_comm_split);
NM_MPI_ALIAS(MPI_Comm_split_type,       mpi_comm_split_type);
NM_MPI_ALIAS(MPI_Comm_dup,              mpi_comm_dup);
NM_MPI_ALIAS(MPI_Comm_free,             mpi_comm_free);
NM_MPI_ALIAS(MPI_Comm_compare,          mpi_comm_compare);
NM_MPI_ALIAS(MPI_Comm_test_inter,       mpi_comm_test_inter);
NM_MPI_ALIAS(MPI_Comm_set_name,         mpi_comm_set_name);
NM_MPI_ALIAS(MPI_Comm_get_name,         mpi_comm_get_name);
NM_MPI_ALIAS(MPI_Comm_get_parent,       mpi_comm_get_parent);
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
NM_MPI_ALIAS(MPI_Comm_remote_size,      mpi_comm_remote_size);
NM_MPI_ALIAS(MPI_Comm_remote_group,     mpi_comm_remote_group);
NM_MPI_ALIAS(MPI_Intercomm_create,      mpi_intercomm_create);
NM_MPI_ALIAS(MPI_Intercomm_merge,       mpi_intercomm_merge);

/* ********************************************************* */

__PUK_SYM_INTERNAL
void nm_mpi_comm_init(void)
{
  /* handles allocator */
  nm_mpi_handle_communicator_init(&nm_mpi_communicators);
  nm_mpi_handle_group_init(&nm_mpi_groups);

  /* built-in communicators */
  nm_mpi_communicator_t*p_comm_world = nm_mpi_handle_communicator_store(&nm_mpi_communicators, MPI_COMM_WORLD);
  p_comm_world->p_nm_comm = nm_comm_world("madmpi");
  p_comm_world->kind = NM_MPI_COMMUNICATOR_INTRA;
  p_comm_world->attrs = NULL;
  p_comm_world->p_errhandler = nm_mpi_errhandler_get(MPI_ERRORS_ARE_FATAL);
  p_comm_world->name = strdup("MPI_COMM_WORLD");
  nm_mpi_communicator_t*p_comm_self = nm_mpi_handle_communicator_store(&nm_mpi_communicators, MPI_COMM_SELF);
  p_comm_self->p_nm_comm = nm_comm_self("madmpi");
  p_comm_self->kind = NM_MPI_COMMUNICATOR_INTRA;
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
  nm_mpi_handle_communicator_finalize(&nm_mpi_communicators, &nm_mpi_communicator_destroy);
  nm_mpi_handle_group_finalize(&nm_mpi_groups, &nm_mpi_group_destroy);
}

__PUK_SYM_INTERNAL
nm_mpi_communicator_t*nm_mpi_communicator_get(MPI_Comm comm)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_handle_communicator_get(&nm_mpi_communicators, comm);
  return p_comm;
}

static inline nm_mpi_communicator_t*nm_mpi_communicator_alloc(nm_comm_t p_nm_comm, struct nm_mpi_errhandler_s*p_errhandler,
							      enum nm_mpi_communicator_kind_e kind)
{
  nm_mpi_communicator_t*p_new_comm = nm_mpi_handle_communicator_alloc(&nm_mpi_communicators);
  p_new_comm->p_nm_comm = p_nm_comm;
  p_new_comm->attrs = NULL;
  p_new_comm->p_errhandler = p_errhandler;
  p_new_comm->name = NULL;
  p_new_comm->kind = kind;
  return p_new_comm;
}

static void nm_mpi_communicator_destroy(nm_mpi_communicator_t*p_comm)
{
  if(p_comm != NULL)
    {
      nm_mpi_attrs_destroy(p_comm->id, &p_comm->attrs);
      if(p_comm->name)
	free(p_comm->name);
      if(p_comm->kind == NM_MPI_COMMUNICATOR_INTER)
	{
	  nm_group_free(p_comm->intercomm.p_local_group);
	  nm_group_free(p_comm->intercomm.p_remote_group);
	}
      nm_comm_destroy(p_comm->p_nm_comm);
      nm_mpi_handle_communicator_free(&nm_mpi_communicators, p_comm);
    }
}

__PUK_SYM_INTERNAL
nm_mpi_group_t*nm_mpi_group_get(MPI_Group group)
{
  nm_mpi_group_t*p_group = nm_mpi_handle_group_get(&nm_mpi_groups, group);
  return p_group;
}

__PUK_SYM_INTERNAL
nm_mpi_group_t*nm_mpi_group_alloc(void)
{
  nm_mpi_group_t*p_group = nm_mpi_handle_group_alloc(&nm_mpi_groups);
  p_group->p_nm_group = NULL;
  return p_group;
}

static void nm_mpi_group_destroy(nm_mpi_group_t*p_group)
{
  if(p_group != NULL)
    {
      nm_group_free(p_group->p_nm_group);
      nm_mpi_handle_group_free(&nm_mpi_groups, p_group);
    }
}

/* ********************************************************* */

int mpi_comm_size(MPI_Comm comm, int*size)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(size)
    {
      if(p_comm->kind == NM_MPI_COMMUNICATOR_INTER)
	{
	  *size = nm_group_size(p_comm->intercomm.p_local_group);
	}
      else if(p_comm->kind == NM_MPI_COMMUNICATOR_INTRA)
	{
	  *size = nm_comm_size(p_comm->p_nm_comm);
	}
      else
	{
	  *size = MPI_UNDEFINED;
	  return MPI_ERR_COMM;
	}
    }
  return MPI_SUCCESS;
}

int mpi_comm_rank(MPI_Comm comm, int *rank)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(rank)
    {
      if(p_comm->kind == NM_MPI_COMMUNICATOR_INTER)
	{
	  *rank = p_comm->intercomm.rank;
	}
      else if(p_comm->kind == NM_MPI_COMMUNICATOR_INTRA)
	{
	  *rank = nm_comm_rank(p_comm->p_nm_comm);
	}
      else
	{
	  *rank = MPI_UNDEFINED;
	  return MPI_ERR_COMM;
	}
    }
  return MPI_SUCCESS;
}

/** FORTRAN binding for MPI_Keyval_create */
void mpi_keyval_create_(void*copy_subroutine, void*delete_subroutine, int*keyval, void*extra_state, int*ierr)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_new();
  p_keyval->copy_subroutine = copy_subroutine;
  p_keyval->delete_subroutine = delete_subroutine;
  p_keyval->extra_state = extra_state;
  *keyval = p_keyval->id;
  *ierr = MPI_SUCCESS;
}

int mpi_keyval_create(MPI_Copy_function*copy_fn, MPI_Delete_function*delete_fn, int*keyval, void*extra_state)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_new();
  p_keyval->copy_fn = copy_fn;
  p_keyval->delete_fn = delete_fn;
  p_keyval->extra_state = extra_state;
  *keyval = p_keyval->id;
  return MPI_SUCCESS;
}

int mpi_keyval_free(int*keyval)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_get(*keyval);
  if(p_keyval == NULL)
    return MPI_ERR_KEYVAL;
  nm_mpi_keyval_delete(p_keyval);
  *keyval = MPI_KEYVAL_INVALID;
  return MPI_SUCCESS;
}

int mpi_attr_put(MPI_Comm comm, int keyval, void*attr_value)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_get(keyval);
  if(p_keyval == NULL)
    return MPI_ERR_KEYVAL;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    return MPI_ERR_COMM;
  if(p_comm->attrs == NULL)
    {
      p_comm->attrs = puk_hashtable_new_ptr();
    }
  int err = nm_mpi_attr_put(p_comm->id, p_comm->attrs, p_keyval, attr_value);
  return err;
}

int mpi_attr_get(MPI_Comm comm, int keyval, void*attr_value, int*flag)
{
  int err = MPI_SUCCESS;
  const static int nm_mpi_tag_ub = NM_MPI_TAG_MAX;
  const static int nm_mpi_wtime_is_global = 0;
  const static int nm_mpi_host = MPI_PROC_NULL;
  static int nm_mpi_universe_size = 0;
  static int nm_mpi_lastusedcode = MPI_ERR_LASTCODE;
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
    case MPI_UNIVERSE_SIZE:
      nm_launcher_get_size(&nm_mpi_universe_size);
      *(const int**)attr_value = &nm_mpi_universe_size;
      *flag = 1;
      break;
    case MPI_APPNUM:
      *flag = 0;
      break;
    case MPI_LASTUSEDCODE:
#warning TODO- check dyanmic value
      *(int**)attr_value = &nm_mpi_lastusedcode;
      *flag = 1;
      break;
    default:
      {
	struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_get(keyval);
	if(p_keyval == NULL)
	  return MPI_ERR_KEYVAL;
	nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
	if(p_comm == NULL)
	  return MPI_ERR_COMM;
	*flag = 0;
	nm_mpi_attr_get(p_comm->attrs, p_keyval, attr_value, flag);
      }
      break;
    }
  return err;
}

int mpi_attr_delete(MPI_Comm comm, int keyval)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_get(keyval);
  if(p_keyval == NULL)
    return MPI_ERR_KEYVAL;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    return MPI_ERR_COMM;
  int err = nm_mpi_attr_delete(p_comm->id, p_comm->attrs, p_keyval);
  return err;
}


int mpi_comm_create_keyval(MPI_Comm_copy_attr_function*comm_copy_attr_fn, MPI_Comm_delete_attr_function*comm_delete_attr_fn, int*comm_keyval, void*extra_state)
{
  int err = mpi_keyval_create(comm_copy_attr_fn, comm_delete_attr_fn, comm_keyval, extra_state);
  return err;
}

int mpi_comm_free_keyval(int*comm_keyval)
{
  int err = mpi_keyval_free(comm_keyval);
  return err;
}
  
int mpi_comm_get_attr(MPI_Comm comm, int comm_keyval, void*attr_value, int*flag)
{
  int err = mpi_attr_get(comm, comm_keyval, attr_value, flag);
  return err;
}

int mpi_comm_set_attr(MPI_Comm comm, int comm_keyval, void*attr_value)
{
  int err = mpi_attr_put(comm, comm_keyval, attr_value);
  return err;
}

int mpi_comm_delete_attr(MPI_Comm comm, int comm_keyval)
{
  int err = mpi_attr_delete(comm, comm_keyval);
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
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    return MPI_ERR_COMM;
  *flag = (p_comm->kind == NM_MPI_COMMUNICATOR_INTER);
  return MPI_SUCCESS;
}

int mpi_comm_compare(MPI_Comm comm1, MPI_Comm comm2, int*result)
{
  if(comm1 == comm2)
    {
      *result = MPI_IDENT;
      return MPI_SUCCESS;
    }
  nm_mpi_communicator_t*p_comm1 = nm_mpi_communicator_get(comm1);
  nm_mpi_communicator_t*p_comm2 = nm_mpi_communicator_get(comm2);
  if(p_comm1 == NULL || p_comm2 == NULL)
    return MPI_ERR_COMM;
  if((p_comm1->kind != p_comm2->kind) || (p_comm1->kind == NM_MPI_COMMUNICATOR_INTER && p_comm1->intercomm.remote_offset != p_comm2->intercomm.remote_offset))
    {
      *result = MPI_UNEQUAL;
      return MPI_SUCCESS;
    }
  int r = nm_group_compare(nm_comm_group(p_comm1->p_nm_comm), nm_comm_group(p_comm2->p_nm_comm));
  if(r == NM_GROUP_IDENT)
    *result = MPI_CONGRUENT;
  else if(r == NM_GROUP_SIMILAR)
    *result = MPI_SIMILAR;
  else
    *result = MPI_UNEQUAL;
  return MPI_SUCCESS;
}

int mpi_comm_group(MPI_Comm comm, MPI_Group*group)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    return MPI_ERR_COMM;
  if(group != NULL)
    {
      nm_mpi_group_t*p_new_group = nm_mpi_handle_group_alloc(&nm_mpi_groups);
      p_new_group->p_nm_group = (p_comm->kind == NM_MPI_COMMUNICATOR_INTER) ?
	nm_group_dup(p_comm->intercomm.p_local_group) :
	nm_group_dup(nm_comm_group(p_comm->p_nm_comm));
      MPI_Group new_id = p_new_group->id;
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
  if(p_old_comm->kind == NM_MPI_COMMUNICATOR_INTER)
    {
      nm_session_t p_session = nm_comm_get_session(p_old_comm->p_nm_comm);
      const nm_tag_t tag = NM_MPI_TAG_PRIVATE_COMMCREATE;
      nm_group_t p_local_group = nm_group_intersection(p_new_group->p_nm_group, p_old_comm->intercomm.p_local_group);
      const int local_size = nm_group_size(p_local_group);
      const int local_rank = nm_group_rank(p_local_group);
      int remote_size = nm_group_size(p_old_comm->intercomm.p_remote_group);
      int*remote_ranks = malloc(sizeof(int) * remote_size);
      nm_sr_request_t rreq;
      nm_sr_irecv(p_session, NM_GATE_NONE, tag, remote_ranks, sizeof(int) * remote_size, &rreq);
      if((local_rank == 0) || (local_size == 0 && p_old_comm->intercomm.rank == 0))
	{
	  /* local leader */
	  int*local_ranks = malloc(sizeof(int) * local_size);
	  int i;
	  for(i = 0; i < local_size; i++)
	    {
	      const nm_gate_t p_gate = nm_group_get_gate(p_local_group, i);
	      local_ranks[i] = nm_group_get_dest(p_old_comm->intercomm.p_local_group, p_gate);
	    }
	  for(i = 0; i < remote_size; i++)
	    {
	      nm_sr_send(p_session, nm_group_get_gate(p_old_comm->intercomm.p_remote_group, i),
			 tag, local_ranks, sizeof(int) * local_size);
	    }
	  free(local_ranks);
	}
      nm_sr_rwait(p_session, &rreq);
      nm_len_t rlen = -1;
      nm_sr_request_get_size(&rreq, &rlen);
      remote_size = rlen / sizeof(int);
      if((local_rank == -1) || (remote_size == 0))
	{
	  /* node not in new comm; discard remote group */
	  free(remote_ranks);
	  *newcomm = MPI_COMM_NULL;
	  return MPI_SUCCESS;
	}
      nm_group_t p_remote_group = nm_group_new();
      int i;
      for(i = 0; i < remote_size; i++)
	{
	  nm_gate_t p_gate = nm_group_get_gate(p_old_comm->intercomm.p_remote_group, remote_ranks[i]);
	  nm_group_add_node(p_remote_group, p_gate);
	}
      free(remote_ranks);
      const int group_first = (p_old_comm->intercomm.remote_offset != 0);
      nm_group_t p_overlay_group = group_first ? nm_group_union(p_local_group, p_remote_group) : nm_group_union(p_remote_group, p_local_group);
      nm_mpi_communicator_t*p_new_comm = nm_mpi_communicator_alloc(nm_comm_create_group(p_old_comm->p_nm_comm, p_overlay_group, p_overlay_group),
								   nm_mpi_errhandler_get(MPI_ERRORS_ARE_FATAL),
								   NM_MPI_COMMUNICATOR_INTER);
      p_new_comm->intercomm.p_remote_group = p_remote_group;
      p_new_comm->intercomm.p_local_group  = p_local_group;
      p_new_comm->intercomm.rank           = local_rank;
      p_new_comm->intercomm.remote_offset  = group_first ? local_size : 0;
      *newcomm = p_new_comm->id;
    }
  else if(p_old_comm->kind == NM_MPI_COMMUNICATOR_INTRA)
    {
      if(nm_group_size(p_new_group->p_nm_group) == 0)
	{
	  *newcomm = MPI_COMM_NULL;
	  return MPI_SUCCESS;
	}
      nm_comm_t p_nm_comm = nm_comm_create(p_old_comm->p_nm_comm, p_new_group->p_nm_group);
      if(p_nm_comm == NULL)
	{
	  /* node not in group*/
	  *newcomm = MPI_COMM_NULL;
	  return MPI_SUCCESS;
	}
      nm_mpi_communicator_t*p_new_comm = nm_mpi_communicator_alloc(p_nm_comm, p_old_comm->p_errhandler, NM_MPI_COMMUNICATOR_INTRA);
      *newcomm = p_new_comm->id;
    }
  return MPI_SUCCESS;
}

int mpi_comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm*newcomm)
{
  nm_mpi_communicator_t*p_old_comm = nm_mpi_communicator_get(comm);
  if(p_old_comm == NULL)
    return MPI_ERR_COMM;
  if(p_old_comm->kind != NM_MPI_COMMUNICATOR_INTRA)
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
  nm_comm_t p_nm_comm = nm_comm_create_group(p_old_comm->p_nm_comm, p_new_group->p_nm_group, p_new_group->p_nm_group);
  if(p_nm_comm == NULL)
    {
      *newcomm = MPI_COMM_NULL;
      return MPI_SUCCESS;
    }
  nm_mpi_communicator_t*p_new_comm = nm_mpi_communicator_alloc(p_nm_comm, p_old_comm->p_errhandler, NM_MPI_COMMUNICATOR_INTRA);
  *newcomm = p_new_comm->id;
  return MPI_SUCCESS;
}
  
/** a node participating in a mpi_comm_split */
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

int mpi_comm_split(MPI_Comm oldcomm, int color, int key, MPI_Comm*newcomm)
{
  nm_mpi_communicator_t*p_old_comm = nm_mpi_communicator_get(oldcomm);
  const int intra = (p_old_comm->kind == NM_MPI_COMMUNICATOR_INTRA);
  nm_group_t p_old_group = intra ? nm_comm_group(p_old_comm->p_nm_comm) : p_old_comm->intercomm.p_local_group;
  const int group_size   = nm_group_size(p_old_group);
  const int rank         = intra ? nm_comm_rank(p_old_comm->p_nm_comm) : p_old_comm->intercomm.rank;
  const struct nm_mpi_comm_split_node_s local_node = { .color = color, .key = key, .rank = rank};
  struct nm_mpi_comm_split_node_s*all_nodes = malloc(group_size * sizeof(struct nm_mpi_comm_split_node_s));

  const nm_tag_t tag = NM_MPI_TAG_PRIVATE_COMMSPLIT;
  const int self = nm_group_rank(p_old_group);
  const int root = 0;
  nm_coll_group_gather(nm_comm_get_session(p_old_comm->p_nm_comm), p_old_group, root, self,
		       &local_node, sizeof(local_node), all_nodes, sizeof(local_node), tag);
  if(self == root)
    {
      qsort(all_nodes, group_size, sizeof(struct nm_mpi_comm_split_node_s), &nodecmp);
    }
  nm_coll_group_bcast(nm_comm_get_session(p_old_comm->p_nm_comm), p_old_group, root, self,
		      all_nodes, group_size * sizeof(struct nm_mpi_comm_split_node_s), tag);
  nm_group_t p_newgroup = nm_group_new();
  int i;
  for(i = 0; i < group_size; i++)
    {
      if(all_nodes[i].color == color)
	{
	  const nm_gate_t p_gate = nm_group_get_gate(p_old_group, all_nodes[i].rank);
	  nm_group_add_node(p_newgroup, p_gate);
	}
    }
  if(intra)
    {
      nm_comm_t p_nm_comm = nm_comm_create(p_old_comm->p_nm_comm, p_newgroup);
      if(color == MPI_UNDEFINED)
	{
	  *newcomm = MPI_COMM_NULL;
	}
      else if(p_nm_comm != NULL)
	{
	  nm_mpi_communicator_t*p_new_comm = nm_mpi_communicator_alloc(p_nm_comm, p_old_comm->p_errhandler, NM_MPI_COMMUNICATOR_INTRA);
	  *newcomm = p_new_comm->id;
	}
      else
	{
	  NM_MPI_FATAL_ERROR("# MPI_Comm_split()- internal error (node in group, not in new communicator).\n");
	}
    }
  else
    {
      /* find new group leaders */
      const int parent_size = nm_comm_size(p_old_comm->p_nm_comm);
      int*all_colors = malloc(sizeof(int) * parent_size);
      nm_coll_gather(p_old_comm->p_nm_comm, 1, &color, sizeof(int), all_colors, sizeof(int), tag);
      nm_coll_bcast(p_old_comm->p_nm_comm, 1, all_colors, sizeof(int) * parent_size, tag);
      const int local_size      = nm_group_size(p_old_comm->intercomm.p_local_group);
      const int remote_size     = nm_group_size(p_old_comm->intercomm.p_remote_group);
      const int local_offset    = (p_old_comm->intercomm.remote_offset == 0) ? remote_size : 0;
      const int remote_offset   = p_old_comm->intercomm.remote_offset;
      nm_gate_t p_local_leader  = NULL;
      nm_gate_t p_remote_leader = NULL;
      for(i = 0; i < local_size; i++)
	{
	  if(all_colors[i + local_offset] == color)
	    {
	      p_local_leader = nm_comm_get_gate(p_old_comm->p_nm_comm, i + local_offset);
	      break;
	    }
	}
      assert(p_local_leader != NULL);
      for(i = 0; i < remote_size; i++)
	{
	  if(all_colors[i + remote_offset] == color)
	    {
	      p_remote_leader = nm_comm_get_gate(p_old_comm->p_nm_comm, i + remote_offset);
	      break;
	    }
	}
      free(all_colors);
      if(p_remote_leader == NULL)
	{
	  /* remote group is empty */
	  *newcomm = MPI_COMM_NULL;
	}
      else
	{
	  nm_mpi_communicator_t*p_new_intercomm = nm_mpi_intercomm_create(p_old_comm, p_newgroup, p_local_leader, p_remote_leader, tag);
	  if(p_new_intercomm == NULL)
	    {
	      *newcomm = MPI_COMM_NULL;
	    }
	  else
	    {
	      *newcomm = p_new_intercomm->id;
	    }
	}
    }
  nm_group_free(p_newgroup);
  free(all_nodes);
  return MPI_SUCCESS;
}

#warning THIS HARDLY WORKS, AND IS USED ONLY FOR RMA TESTS. IT HAS TO BE REDONE !
/** a node participating in a mpi_comm_split_type */
struct nm_mpi_comm_split_shared_node_s
{
  char hostname[1024];
  int key;
  int rank;
};
/** sort by color major, key minor */
static int nodesharedcmp(const void*v1, const void*v2) 
{
  const struct nm_mpi_comm_split_shared_node_s*n1 = v1;
  const struct nm_mpi_comm_split_shared_node_s*n2 = v2;
  int res = strcmp(n1->hostname, n2->hostname);
  if(0 == res)
    return (n1->key - n2->key);
  else
    return res;
}

int mpi_comm_split_type(MPI_Comm oldcomm, int split_type, int key, MPI_Info info, MPI_Comm*newcomm)
{
  NM_MPI_WARNING("# This functions (%s) is not implemented yet.\n",__FUNCTION__);
  nm_mpi_communicator_t*p_old_comm = nm_mpi_communicator_get(oldcomm);
  const int intra = (p_old_comm->kind == NM_MPI_COMMUNICATOR_INTRA);
  nm_group_t p_old_group = intra ? nm_comm_group(p_old_comm->p_nm_comm) : p_old_comm->intercomm.p_local_group;
  const int group_size   = nm_group_size(p_old_group);
  const int rank         = intra ? nm_comm_rank(p_old_comm->p_nm_comm) : p_old_comm->intercomm.rank;
  const struct nm_mpi_comm_split_shared_node_s local_node = { .key = key, .rank = rank};
  char hostname[1024];
  gethostname(hostname, 1024);
  hostname[1023] = '\0';
  memcpy((void*)local_node.hostname, hostname, 1024 * sizeof(char));
  struct nm_mpi_comm_split_shared_node_s*all_nodes = malloc(group_size * sizeof(struct nm_mpi_comm_split_shared_node_s));

  const nm_tag_t tag = NM_MPI_TAG_PRIVATE_COMMSPLIT;
  const int self = nm_group_rank(p_old_group);
  const int root = 0;
  nm_coll_group_gather(nm_comm_get_session(p_old_comm->p_nm_comm), p_old_group, root, self,
		       &local_node, sizeof(local_node), all_nodes, sizeof(local_node), tag);
  if(self == root)
    {
      qsort(all_nodes, group_size, sizeof(struct nm_mpi_comm_split_shared_node_s), &nodesharedcmp);
    }
  nm_coll_group_bcast(nm_comm_get_session(p_old_comm->p_nm_comm), p_old_group, root, self,
		      all_nodes, group_size * sizeof(struct nm_mpi_comm_split_shared_node_s), tag);
  nm_group_t p_newgroup = nm_group_new();
  int i;
  for(i = 0; i < group_size; i++)
    {
      if(!strcmp(all_nodes[i].hostname, local_node.hostname))
	{
	  const nm_gate_t p_gate = nm_group_get_gate(p_old_group, all_nodes[i].rank);
	  nm_group_add_node(p_newgroup, p_gate);
	}
    }
  if(intra)
    {
      nm_comm_t p_nm_comm = nm_comm_create(p_old_comm->p_nm_comm, p_newgroup);
      if(split_type == MPI_UNDEFINED)
	{
	  *newcomm = MPI_COMM_NULL;
	}
      else if(p_nm_comm != NULL)
	{
	  nm_mpi_communicator_t*p_new_comm = nm_mpi_communicator_alloc(p_nm_comm, p_old_comm->p_errhandler, NM_MPI_COMMUNICATOR_INTRA);
	  *newcomm = p_new_comm->id;
	}
      else
	{
	  NM_MPI_FATAL_ERROR("# MPI_Comm_split()- internal error (node in group, not in new communicator).\n");
	}
    }
  else
    {
      /* find new group leaders */
      const int parent_size = nm_comm_size(p_old_comm->p_nm_comm);
      char*all_hostnames = (char*)malloc(1024 * sizeof(char) * parent_size);
      nm_coll_gather(p_old_comm->p_nm_comm, 1, local_node.hostname, 1024 * sizeof(char), all_hostnames, 1024 * sizeof(char), tag);
      nm_coll_bcast(p_old_comm->p_nm_comm, 1, all_hostnames, 1024 * sizeof(char) * parent_size, tag);
      const int local_size      = nm_group_size(p_old_comm->intercomm.p_local_group);
      const int remote_size     = nm_group_size(p_old_comm->intercomm.p_remote_group);
      const int local_offset    = (p_old_comm->intercomm.remote_offset == 0) ? remote_size : 0;
      const int remote_offset   = p_old_comm->intercomm.remote_offset;
      nm_gate_t p_local_leader  = NULL;
      nm_gate_t p_remote_leader = NULL;
      for(i = 0; i < local_size; i++)
	{
	  if(!strcmp(&all_hostnames[1024 * (i + local_offset)], local_node.hostname))
	    {
	      p_local_leader = nm_comm_get_gate(p_old_comm->p_nm_comm, i + local_offset);
	      break;
	    }
	}
      assert(p_local_leader != NULL);
      for(i = 0; i < remote_size; i++)
	{
	  if(!strcmp(&all_hostnames[1024 * (i + remote_offset)], local_node.hostname))
	    {
	      p_remote_leader = nm_comm_get_gate(p_old_comm->p_nm_comm, i + remote_offset);
	      break;
	    }
	}
      free(all_hostnames);
      if(p_remote_leader == NULL)
	{
	  /* remote group is empty */
	  *newcomm = MPI_COMM_NULL;
	}
      else
	{
	  nm_mpi_communicator_t*p_new_intercomm = nm_mpi_intercomm_create(p_old_comm, p_newgroup, p_local_leader, p_remote_leader, tag);
	  if(p_new_intercomm == NULL)
	    {
	      *newcomm = MPI_COMM_NULL;
	    }
	  else
	    {
	      *newcomm = p_new_intercomm->id;
	    }
	}
    }
  nm_group_free(p_newgroup);
  free(all_nodes);
  return MPI_SUCCESS;
}

int mpi_comm_dup(MPI_Comm oldcomm, MPI_Comm *newcomm)
{
  nm_mpi_communicator_t*p_old_comm = nm_mpi_communicator_get(oldcomm);
  if(p_old_comm == NULL)
    return MPI_ERR_COMM;
  nm_mpi_communicator_t*p_new_comm = nm_mpi_communicator_alloc(nm_comm_dup(p_old_comm->p_nm_comm),
							       p_old_comm->p_errhandler, p_old_comm->kind);
  if(p_old_comm->attrs)
    {
      int err = nm_mpi_attrs_copy(p_old_comm->id, p_old_comm->attrs, &p_new_comm->attrs);
      if(err)
	{
	  nm_mpi_communicator_destroy(p_new_comm);
	  *newcomm = MPI_COMM_NULL;
	  return err;
	}
    }
  if(p_old_comm->kind == NM_MPI_COMMUNICATOR_INTER)
    {
      p_new_comm->intercomm.p_local_group  = nm_group_dup(p_old_comm->intercomm.p_local_group);
      p_new_comm->intercomm.p_remote_group = nm_group_dup(p_old_comm->intercomm.p_remote_group);
      p_new_comm->intercomm.rank           = p_old_comm->intercomm.rank;
      p_new_comm->intercomm.remote_offset  = p_old_comm->intercomm.remote_offset;
    }
  if(p_old_comm->name != NULL)
    {
      p_new_comm->name = strndup(p_old_comm->name, MPI_MAX_OBJECT_NAME);
    }
  *newcomm = p_new_comm->id;
  return MPI_SUCCESS;
}

int mpi_comm_free(MPI_Comm *comm)
{
  if(*comm == MPI_COMM_WORLD)
    {
      NM_MPI_FATAL_ERROR("Cannot free communicator MPI_COMM_WORLD");
      return MPI_ERR_OTHER;
    }
  else if(*comm == MPI_COMM_SELF)
    {
      NM_MPI_FATAL_ERROR("Cannot free communicator MPI_COMM_SELF");
      return MPI_ERR_OTHER;
    }
  nm_mpi_communicator_t*p_comm = nm_mpi_handle_communicator_get(&nm_mpi_communicators, *comm);
  if(p_comm == NULL)
    {
      NM_MPI_FATAL_ERROR("Communicator %d unknown\n", *comm);
      return MPI_ERR_OTHER;
    }
  nm_mpi_communicator_destroy(p_comm);
  *comm = MPI_COMM_NULL;
  return MPI_SUCCESS;
}

int mpi_comm_get_parent(MPI_Comm*parent)
{
  if(parent != NULL)
    *parent = MPI_COMM_NULL;
  return MPI_SUCCESS;
}

int mpi_cart_create(MPI_Comm comm_old, int ndims, const int*dims, const int*periods, int reorder, MPI_Comm*newcomm)
{
  MPI_Comm comm_cart = MPI_COMM_NULL;
  int err = mpi_comm_dup(comm_old, &comm_cart);
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
  if(cart.size > nm_comm_size(p_old_comm->p_nm_comm))
    return MPI_ERR_TOPOLOGY;
  nm_group_t cart_group = nm_gate_vect_new();
  int i;
  for(i = 0; i < nm_comm_size(p_old_comm->p_nm_comm); i++)
    {
      nm_gate_vect_push_back(cart_group, nm_comm_get_gate(p_old_comm->p_nm_comm, i));
    }
  nm_mpi_communicator_t*p_new_comm = nm_mpi_communicator_alloc(nm_comm_create(p_old_comm->p_nm_comm, cart_group), p_old_comm->p_errhandler, NM_MPI_COMMUNICATOR_INTRA);
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
  const int rank = nm_comm_rank(p_comm_cart->p_nm_comm);
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
  const int rank = nm_comm_rank(p_comm->p_nm_comm);
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
	NM_MPI_FATAL_ERROR("stride = 0");
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
      nm_group_t p_nm_group = nm_group_excl(p_group->p_nm_group, n, ranks);
      if(nm_group_size(p_nm_group) == 0)
	{
	  *newgroup = MPI_GROUP_EMPTY;
	}
      else
	{
	  nm_mpi_group_t*p_new_group = nm_mpi_handle_group_alloc(&nm_mpi_groups);
	  p_new_group->p_nm_group = p_nm_group;
	  MPI_Group new_id = p_new_group->id;
	  *newgroup = new_id;
	}
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
	NM_MPI_FATAL_ERROR("stride = 0");
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
  if(id != MPI_GROUP_NULL && id != MPI_GROUP_EMPTY)
    {
      nm_mpi_group_t*p_group = nm_mpi_handle_group_get(&nm_mpi_groups, id);
      nm_mpi_group_destroy(p_group);
    }
  *group = MPI_GROUP_NULL;
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
      if(ranks1[i] == MPI_PROC_NULL)
	ranks2[i] = MPI_PROC_NULL;
      else if(ranks2[i] == -1)
	ranks2[i] = MPI_UNDEFINED;
    }
  return MPI_SUCCESS;
}


int mpi_comm_remote_size(MPI_Comm comm, int*size)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    return MPI_ERR_COMM;
  if(p_comm->kind != NM_MPI_COMMUNICATOR_INTER)
    {
      *size = MPI_UNDEFINED;
      return MPI_ERR_COMM;
    }
  if(size)
    {
      *size = nm_group_size(p_comm->intercomm.p_remote_group);
    }
  return MPI_SUCCESS;
}

int mpi_comm_remote_group(MPI_Comm comm, MPI_Group*group)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    return MPI_ERR_COMM;
  if(p_comm->kind != NM_MPI_COMMUNICATOR_INTER)
    {
      return MPI_ERR_COMM;
    }
  if(group != NULL)
    {
      nm_mpi_group_t*p_new_group = nm_mpi_handle_group_alloc(&nm_mpi_groups);
      p_new_group->p_nm_group = nm_group_dup(p_comm->intercomm.p_remote_group);
      MPI_Group new_id = p_new_group->id;
      *group = new_id;
    }
  return MPI_SUCCESS;
}

static nm_mpi_communicator_t*nm_mpi_intercomm_create(nm_mpi_communicator_t*p_parent_comm, nm_group_t p_local_group,
						     nm_gate_t p_local_leader, nm_gate_t p_remote_leader, nm_tag_t tag)
{
  const int local_size = nm_group_size(p_local_group);
  const int parent_size = nm_comm_size(p_parent_comm->p_nm_comm);
  int*remote_ranks = malloc(sizeof(int) * parent_size);
  int remote_size = -1;
  nm_gate_t p_self_gate = nm_comm_gate_self(p_parent_comm->p_nm_comm);
  nm_session_t p_session = nm_comm_get_session(p_parent_comm->p_nm_comm);
  /* self is local leader */
  if(p_self_gate == p_local_leader)
    {
      /* translate local group into parent comm numbering */
      int*local_ranks = malloc(sizeof(int) * local_size);
      int i;
      for(i = 0; i < local_size; i++)
	{
	  const nm_gate_t p_gate = nm_group_get_gate(p_local_group, i);
	  const int rank = nm_comm_get_dest(p_parent_comm->p_nm_comm, p_gate);
	  if(rank == -1)
	    {
	      /* local group not included in parent communicator -> error */
	      free(local_ranks);
	      return NULL;
	    }
	  local_ranks[i] = rank;
	}
      /* exchange ranks between leaders */
      nm_sr_request_t rreq;
      nm_sr_irecv(p_session, p_remote_leader, tag, remote_ranks, sizeof(int) * remote_size, &rreq);
      nm_sr_send(p_session, p_remote_leader, tag, local_ranks, sizeof(int) * local_size);
      nm_sr_rwait(p_session, &rreq);
      nm_len_t rsize = -1;
      nm_sr_request_get_size(&rreq, &rsize);
      remote_size = rsize / sizeof(int);
      free(local_ranks);
    }  
  /* broadcast groups */
  const int root = nm_group_get_dest(p_local_group, p_local_leader);
  const int self = nm_group_rank(p_local_group);
  nm_coll_group_bcast(p_session, p_local_group, root, self, &remote_size, sizeof(nm_tag_t), tag);
  assert(remote_size <= parent_size);
  nm_coll_group_bcast(p_session, p_local_group, root, self, remote_ranks, sizeof(nm_tag_t) * remote_size, tag);
  nm_group_t p_remote_group = nm_group_incl(nm_comm_group(p_parent_comm->p_nm_comm), remote_size, remote_ranks);
  const int local0_parent  = nm_comm_get_dest(p_parent_comm->p_nm_comm, nm_group_get_gate(p_local_group, 0));
  const int remote0_parent = remote_ranks[0];
  free(remote_ranks);
  const int group_first = (local0_parent > remote0_parent);
  nm_group_t p_group = group_first ? nm_group_union(p_local_group, p_remote_group) : nm_group_union(p_remote_group, p_local_group);
  nm_mpi_communicator_t*p_new_comm = nm_mpi_communicator_alloc(nm_comm_create_group(p_parent_comm->p_nm_comm, p_group, p_group),
							       nm_mpi_errhandler_get(MPI_ERRORS_ARE_FATAL),
							       NM_MPI_COMMUNICATOR_INTER);
  p_new_comm->intercomm.p_remote_group = p_remote_group;
  p_new_comm->intercomm.p_local_group  = nm_group_dup(p_local_group);
  p_new_comm->intercomm.rank           = nm_group_rank(p_local_group);
  p_new_comm->intercomm.remote_offset  = group_first ? local_size : 0;

  return p_new_comm;
}

int mpi_intercomm_create(MPI_Comm local_comm, int local_leader,
			 MPI_Comm parent_comm, int remote_leader,
			 int tag, MPI_Comm*newintercomm)
{
  nm_mpi_communicator_t*p_local_comm  = nm_mpi_communicator_get(local_comm);
  nm_mpi_communicator_t*p_parent_comm = nm_mpi_communicator_get(parent_comm);
  if(p_local_comm == NULL)
    {
      /* empty local communicator */
      *newintercomm = MPI_COMM_NULL;
      return MPI_SUCCESS;
    }
  nm_group_t p_local_group  = p_local_comm ? nm_comm_group(p_local_comm->p_nm_comm) : NULL;
  nm_gate_t p_local_leader  = p_local_comm ? nm_comm_get_gate(p_local_comm->p_nm_comm, local_leader) : NULL;
  nm_gate_t p_remote_leader = nm_comm_get_gate(p_parent_comm->p_nm_comm, remote_leader);
  nm_mpi_communicator_t*p_new_comm = nm_mpi_intercomm_create(p_parent_comm, p_local_group, p_local_leader, p_remote_leader, tag);
  if(p_new_comm == NULL)
    {
      /* node not in new group */
      *newintercomm = MPI_UNDEFINED;
      return MPI_ERR_COMM;
    }
  *newintercomm = p_new_comm->id;
  return MPI_SUCCESS;
}

int mpi_intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm*newcomm)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(intercomm);
  if(p_comm == NULL)
    return MPI_ERR_COMM;
  if(p_comm->kind != NM_MPI_COMMUNICATOR_INTER)
    {
      return MPI_ERR_COMM;
    }
  nm_mpi_communicator_t*p_comm_world = nm_mpi_communicator_get(MPI_COMM_WORLD);
  nm_comm_t p_nm_comm = nm_comm_create(p_comm_world->p_nm_comm, nm_comm_group(p_comm->p_nm_comm));
  nm_mpi_communicator_t*p_new_comm = nm_mpi_communicator_alloc(p_nm_comm, p_comm->p_errhandler, NM_MPI_COMMUNICATOR_INTRA);
  *newcomm = p_new_comm->id;
  return MPI_SUCCESS;
}
