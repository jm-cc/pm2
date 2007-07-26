/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

/*
 * tbx_darray_inline.h
 * :::::::::::::::::::::::////////////////////////////////////////////////////
 */
/** \addtogroup darray_interface
 *
 * @{
 */



/*
 * Macros
 * ___________________________________________________________________________
 */
#define TBX_DARRAY_MIN_SIZE 16

/*
 *  Functions
 * ___________________________________________________________________________
 */


static
__inline__
p_tbx_darray_t
tbx_darray_init(void)
{
  p_tbx_darray_t darray = NULL;

  LOG_IN();
  darray = (p_tbx_darray_t)TBX_CALLOC(1, sizeof(tbx_darray_t));
  CTRL_ALLOC(darray);

  TBX_INIT_SHARED(darray);
  darray->length           =    0;
  darray->allocated_length =    0;
  darray->data             = NULL;
  LOG_OUT();

  return darray;
}

static
__inline__
void
tbx_darray_free(p_tbx_darray_t darray)
{
  LOG_IN();
  tbx_darray_reset(darray);
  TBX_FREE(darray);
  LOG_OUT();
}

static
__inline__
size_t
tbx_darray_length(p_tbx_darray_t darray)
{
  size_t length = 0;

  LOG_IN();
  length = darray->length;
  LOG_OUT();

  return length;
}

static
__inline__
void
tbx_darray_reset(p_tbx_darray_t darray)
{
  LOG_IN();
  if (darray->allocated_length)
    {
      TBX_FREE(darray->data);
    }

  darray->length           =    0;
  darray->allocated_length =    0;
  darray->data             = NULL;
  LOG_OUT();
}

static
__inline__
void
__tbx_darray_grow(p_tbx_darray_t     darray,
		  tbx_darray_index_t idx)
{
  LOG_IN();
  idx++;

  if (darray->allocated_length)
    {
      if (idx > darray->allocated_length)
	{
	  darray->allocated_length =
	    tbx_max(2 * (darray->allocated_length + 1), idx);

	  darray->data =
	    (void**)TBX_REALLOC(darray->data,
			darray->allocated_length * sizeof(void *));
	  CTRL_ALLOC(darray->data);
	}
    }
  else
    {
      darray->allocated_length =  tbx_max(idx, TBX_DARRAY_MIN_SIZE);

      darray->data = (void**)TBX_CALLOC(darray->allocated_length, sizeof(void *));
      CTRL_ALLOC(darray->data);
    }
  LOG_OUT();
}

static
__inline__
void *
tbx_darray_get(p_tbx_darray_t     darray,
	       tbx_darray_index_t idx)
{
  void *result = NULL;

  LOG_IN();
  if (idx < 0)
    {
      idx = darray->length - idx;
    }

  if (idx < 0)
    TBX_FAILURE("out-of-bounds index");

  if (idx < darray->length)
    {
      result = darray->data[idx];
    }
  LOG_OUT();

  return result;
}

static
__inline__
void
tbx_darray_set(p_tbx_darray_t      darray,
	       tbx_darray_index_t  idx,
	       void               *object)
{
  LOG_IN();
  if (idx < 0)
    {
      idx = darray->length - idx;
    }

  if (idx < 0 || idx >= darray->length)
    TBX_FAILURE("out-of-bounds index");

  darray->data[idx] = object;
  LOG_OUT();
}

static
__inline__
void
tbx_darray_expand_and_set(p_tbx_darray_t      darray,
			  tbx_darray_index_t  idx,
			  void               *object)
{
  LOG_IN();
  if (idx < 0)
    TBX_FAILURE("invalid index");

  if (idx >= darray->length)
    {
      if (idx >= darray->allocated_length)
	{
	  __tbx_darray_grow(darray, idx);
	}

      memset(darray->data + darray->length, 0,
	     sizeof(void *) * ((idx + 1) - darray->length));

      darray->length = idx + 1;
    }

  darray->data[idx] = object;
  LOG_OUT();
}

static
__inline__
void *
tbx_darray_expand_and_get(p_tbx_darray_t     darray,
			  tbx_darray_index_t idx)
{
  void *object = NULL;

  LOG_IN();
  if (idx < 0)
    TBX_FAILURE("invalid index");

  if (idx >= darray->length)
    {
      if (idx >= darray->allocated_length)
	{
	  __tbx_darray_grow(darray, idx);
	}

      memset(darray->data + darray->length, 0,
	     sizeof(void *) * ((idx + 1) - darray->length));

      darray->length = idx + 1;
    }

  object = darray->data[idx];
  LOG_OUT();

  return object;
}

static
__inline__
void
tbx_darray_append(p_tbx_darray_t  darray,
		  void           *object)
{
  LOG_IN();
  if (darray->length == darray->allocated_length)
    {
      __tbx_darray_grow(darray, darray->length);
    }
  darray->data[darray->length] = object;
  darray->length++;
  LOG_OUT();
}

static
__inline__
void *
tbx_darray_first_idx(p_tbx_darray_t       darray,
		     p_tbx_darray_index_t idx)
{
  void *result = NULL;

  LOG_IN();
  if (darray->length)
    {
      *idx = 0;

      do
	{
	  if (darray->data[*idx])
	    {
	      result = darray->data[*idx];
	      break;
	    }

	  (*idx)++;
	}
      while (*idx < darray->length);
    }

  LOG_OUT();

  return result;
}

static
__inline__
void *
tbx_darray_next_idx(p_tbx_darray_t       darray,
		    p_tbx_darray_index_t idx)
{
  void *result = NULL;

  LOG_IN();
  if (darray->length)
    {
      (*idx)++;

      while (*idx < darray->length)
	{
	  if (darray->data[*idx])
	    {
	      result = darray->data[*idx];
	      break;
	    }

	  (*idx)++;
	}
    }
  LOG_OUT();

  return result;
}

/* @} */
