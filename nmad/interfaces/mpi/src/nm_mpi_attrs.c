/*
 * NewMadeleine
 * Copyright (C) 2014-2016 (see AUTHORS file)
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
#include <Padico/Module.h>

PADICO_MODULE_HOOK(MadMPI);

NM_MPI_HANDLE_TYPE(keyval, struct nm_mpi_keyval_s, _NM_MPI_ATTR_OFFSET, 16);

static struct nm_mpi_handle_keyval_s nm_mpi_keyvals;

static void nm_mpi_keyval_destroy(struct nm_mpi_keyval_s*p_keyval);

/* ********************************************************* */

/* ********************************************************* */

__PUK_SYM_INTERNAL
void nm_mpi_attrs_init(void)
{
  nm_mpi_handle_keyval_init(&nm_mpi_keyvals);
}

__PUK_SYM_INTERNAL
void nm_mpi_attrs_exit(void)
{
  nm_mpi_handle_keyval_finalize(&nm_mpi_keyvals, &nm_mpi_keyval_destroy);
}

static void nm_mpi_keyval_destroy(struct nm_mpi_keyval_s*p_keyval)
{
  if(p_keyval != NULL)
    {
      nm_mpi_handle_keyval_free(&nm_mpi_keyvals, p_keyval);
    }
}

static void nm_mpi_keyval_ref_inc(struct nm_mpi_keyval_s*p_keyval)
{
  __sync_fetch_and_add(&p_keyval->refcount, 1);
}

static void nm_mpi_keyval_ref_dec(struct nm_mpi_keyval_s**p_keyval)
{
  const int count = __sync_sub_and_fetch(&(*p_keyval)->refcount, 1);
  if(count == 0)
    {
      nm_mpi_handle_keyval_free(&nm_mpi_keyvals, *p_keyval);
      *p_keyval = NULL;
    }
}

static inline void nm_mpi_attr_remove(puk_hashtable_t p_attrs, struct nm_mpi_keyval_s*p_keyval)
{
  puk_hashtable_remove(p_attrs, p_keyval);
  nm_mpi_keyval_ref_dec(&p_keyval);
}

/* ********************************************************* */

__PUK_SYM_INTERNAL
struct nm_mpi_keyval_s*nm_mpi_keyval_get(int id)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_handle_keyval_get(&nm_mpi_keyvals, id);
  return p_keyval;
}

/** allocate an empty keyval */
__PUK_SYM_INTERNAL
struct nm_mpi_keyval_s*nm_mpi_keyval_new(void)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_handle_keyval_alloc(&nm_mpi_keyvals);
  p_keyval->copy_fn = NULL;;
  p_keyval->delete_fn = NULL;
  p_keyval->copy_subroutine = NULL;
  p_keyval->delete_subroutine = NULL;
  p_keyval->extra_state = NULL;
  p_keyval->refcount = 1;
  return p_keyval;
}

__PUK_SYM_INTERNAL
void nm_mpi_keyval_delete(struct nm_mpi_keyval_s*p_keyval)
{
  nm_mpi_keyval_ref_dec(&p_keyval);
}

__PUK_SYM_INTERNAL
void nm_mpi_attr_get(puk_hashtable_t p_attrs, struct nm_mpi_keyval_s*p_keyval, void**p_attr_value, int*flag)
{
  if((p_attrs != NULL) && puk_hashtable_probe(p_attrs, p_keyval))
    {
      *p_attr_value = puk_hashtable_lookup(p_attrs, p_keyval);
      *flag = 1;
    }
  else
    {
      *flag = 0;
    }
}

__PUK_SYM_INTERNAL
int nm_mpi_attr_put(int id, puk_hashtable_t p_attrs, struct nm_mpi_keyval_s*p_keyval, void*attr_value)
{
  if(puk_hashtable_probe(p_attrs, p_keyval))
    {
      int err = nm_mpi_attr_delete(id, p_attrs, p_keyval);
      if(err != MPI_SUCCESS)
	return err;
    }
  nm_mpi_keyval_ref_inc(p_keyval);
  puk_hashtable_insert(p_attrs, p_keyval, attr_value);
  return MPI_SUCCESS;
}

/** fill the new attributes table with content from old attrs */
__PUK_SYM_INTERNAL
int nm_mpi_attrs_copy(int id, puk_hashtable_t p_old_attrs, puk_hashtable_t*p_new_attrs)
{
  puk_hashtable_enumerator_t e = puk_hashtable_enumerator_new(p_old_attrs);
  struct nm_mpi_keyval_s*p_keyval = NULL;
  void*p_value = NULL;
  do
    {
      puk_hashtable_enumerator_next2(e, &p_keyval, &p_value);
      if(p_keyval != NULL)
	{ 
	  int flag = 0;
	  void*out = NULL;
	  int err = MPI_SUCCESS;
	  if((p_keyval->copy_fn == MPI_NULL_COPY_FN) && (p_keyval->copy_subroutine == NULL))
	    {
	      err = MPI_SUCCESS;
	      flag = 0;
	    }
	  else if((p_keyval->copy_fn == MPI_DUP_FN) || (p_keyval->copy_subroutine == (nm_mpi_copy_subroutine_t*)MPI_DUP_FN))
	    {
	      err = MPI_SUCCESS;
	      out = p_value;
	      flag = 1;
	    }
	  else if(p_keyval->copy_fn != NULL)
	    {
	      err = (*p_keyval->copy_fn)(id, p_keyval->id, p_keyval->extra_state, p_value, &out, &flag);
	    }
	  else if(p_keyval->copy_subroutine != NULL)
	    {
	      (*p_keyval->copy_subroutine)(&id, &p_keyval->id, p_keyval->extra_state, &p_value, &out, &flag, &err);
	    }
	  if(err != MPI_SUCCESS)
	    {
	      NM_MPI_WARNING("copy_fn returned and error while copying attribute.\n");
	      return err;
	    }
	  if(flag)
	    {
	      if(*p_new_attrs == NULL)
		*p_new_attrs = puk_hashtable_new_ptr();
	      nm_mpi_attr_put(id, *p_new_attrs, p_keyval, out);
	    }
	}
    }
  while(p_keyval != NULL);
  puk_hashtable_enumerator_delete(e);
  return MPI_SUCCESS;
}

/** destroy an attributes table */
__PUK_SYM_INTERNAL
void nm_mpi_attrs_destroy(int id, puk_hashtable_t*p_old_attrs)
{
  if(*p_old_attrs != NULL)
    {
      struct nm_mpi_keyval_s*p_keyval = NULL;
      void*p_value = NULL;
      do
	{
	  puk_hashtable_getentry(*p_old_attrs, &p_keyval, &p_value);
	  if(p_keyval != NULL)
	    {
	      nm_mpi_attr_delete(id, *p_old_attrs, p_keyval);
	    }
	}
      while(p_keyval != NULL);
      puk_hashtable_delete(*p_old_attrs);
      *p_old_attrs = NULL;
    }
}

/** delete one attribute from table */
__PUK_SYM_INTERNAL
int nm_mpi_attr_delete(int id, puk_hashtable_t p_attrs, struct nm_mpi_keyval_s*p_keyval)
{
  int err = MPI_SUCCESS;
  if(p_attrs != NULL && puk_hashtable_probe(p_attrs, p_keyval))
    {
      void*v = puk_hashtable_lookup(p_attrs, p_keyval);
      if(p_keyval->delete_fn != NULL)
	{
	  err = (p_keyval->delete_fn)(id, p_keyval->id, v, p_keyval->extra_state);
	}
      else if(p_keyval->delete_subroutine != NULL)
	{
	  (p_keyval->delete_subroutine)(&id, &p_keyval->id, &v, p_keyval->extra_state, &err);
	}
      if(err == MPI_SUCCESS)
	{
	  nm_mpi_attr_remove(p_attrs, p_keyval);
	}
      else
	{
	  NM_MPI_WARNING("delete_fn returned an error while deleting attribute.\n");
	}
    }
  return err;
}


