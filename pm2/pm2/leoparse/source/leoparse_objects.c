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
 * leoparse_objects.c
 * ------------------
 */
#include "leoparse.h"

p_leoparse_object_t
leoparse_get_object(p_leoparse_htable_entry_t entry)
{
  if (entry->type == leoparse_e_object)
    {
      return entry->object;
    }
  else
    FAILURE("parse error : object expected");
}

p_tbx_slist_t
leoparse_get_slist(p_leoparse_htable_entry_t entry)
{
  if (entry->type == leoparse_e_slist)
    {
      return entry->slist;
    }
  else
    FAILURE("parse error : list expected");
}

p_tbx_htable_t
leoparse_get_htable(p_leoparse_object_t object)
{
  if (object->type == leoparse_o_htable)
    {
      return object->htable;
    }
  else
    FAILURE("parse error : table expected");
}

char *
leoparse_get_string(p_leoparse_object_t object)
{
  if (object->type == leoparse_o_string)
    {
      return object->string;
    }
  else
    FAILURE("parse error : string expected");
}

char *
leoparse_get_id(p_leoparse_object_t object)
{
  if (object->type == leoparse_o_id)
    {
      return object->id;
    }
  else
    FAILURE("parse error : id expected");
}

int
leoparse_get_val(p_leoparse_object_t object)
{
  if (object->type == leoparse_o_integer)
    {
      return object->val;
    }
  else
    FAILURE("parse error : integer expected");
}

p_leoparse_range_t
leoparse_get_range(p_leoparse_object_t object)
{
  if (object->type == leoparse_o_range)
    {
      return object->range;
    }
  else
    FAILURE("parse error : range expected");
}

p_tbx_slist_t
leoparse_read_slist(p_tbx_htable_t  htable,
		    const char     *key)
{
  p_leoparse_htable_entry_t entry  = NULL;
  p_tbx_slist_t             result = NULL;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      result = leoparse_get_slist(entry);
    }
  
  return result;
}

char *
leoparse_read_id(p_tbx_htable_t  htable,
		 const char     *key)
{
  p_leoparse_htable_entry_t  entry  = NULL;
  p_leoparse_object_t        object = NULL;
  char                      *result = NULL;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      object = leoparse_get_object(entry);
      result = leoparse_get_id(object);
    }

  return result;
}

char *
leoparse_read_string(p_tbx_htable_t  htable,
		     const char     *key)
{
  p_leoparse_htable_entry_t  entry  = NULL;
  p_leoparse_object_t        object = NULL;
  char                      *result = NULL;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      object = leoparse_get_object(entry);
      result = leoparse_get_string(object);
    }

  return result;
}

p_tbx_htable_t
leoparse_read_htable(p_tbx_htable_t  htable,
		     const char     *key)
{
  p_leoparse_htable_entry_t entry  = NULL;
  p_leoparse_object_t       object = NULL;
  p_tbx_htable_t            result = NULL;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      object = leoparse_get_object(entry);
      result = leoparse_get_htable(object);
    }

  return result;
}

int
leoparse_read_val(p_tbx_htable_t  htable,
		  const char     *key)
{
  p_leoparse_htable_entry_t entry  = NULL;
  p_leoparse_object_t       object = NULL;
  int                       result = 0;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      object = leoparse_get_object(entry);
      result = leoparse_get_val(object);
    }

  return result;
}

p_leoparse_range_t
leoparse_read_range(p_tbx_htable_t  htable,
		    const char     *key)
{
  p_leoparse_htable_entry_t  entry  = NULL;
  p_leoparse_object_t        object = NULL;
  p_leoparse_range_t         result = NULL;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      object = leoparse_get_object(entry);
      result = leoparse_get_range(object);
    }

  return result;
}


p_leoparse_object_t
leoparse_try_get_object(p_leoparse_htable_entry_t entry)
{
  if (entry->type == leoparse_e_object)
    {
      return entry->object;
    }
  else
    {
      return NULL;
    }
}

p_tbx_slist_t
leoparse_try_get_slist(p_leoparse_htable_entry_t entry)
{
  if (entry->type == leoparse_e_slist)
    {
      return entry->slist;
    }
  else
    {
      return NULL;
    }
}

p_tbx_htable_t
leoparse_try_get_htable(p_leoparse_object_t object)
{
  if (object->type == leoparse_o_htable)
    {
      return object->htable;
    }
  else
    {
      return NULL;
    }
}

char *
leoparse_try_get_string(p_leoparse_object_t object)
{
  if (object->type == leoparse_o_string)
    {
      return object->string;
    }
  else
    {
      return NULL;
    }
}

char *
leoparse_try_get_id(p_leoparse_object_t object)
{
  if (object->type == leoparse_o_id)
    {
      return object->id;
    }
  else
    {
      return NULL;
    }
}

int
leoparse_try_get_val(p_leoparse_object_t object,
		     int                 default_value)
{
  if (object->type == leoparse_o_integer)
    {
      return object->val;
    }
  else
    {
      return default_value;
    }
}

p_leoparse_range_t
leoparse_try_get_range(p_leoparse_object_t object)
{
  if (object->type == leoparse_o_range)
    {
      return object->range;
    }
  else
    {
      return NULL;
    }
}

p_tbx_slist_t
leoparse_try_read_slist(p_tbx_htable_t  htable,
			const char     *key)
{
  p_leoparse_htable_entry_t entry  = NULL;
  p_tbx_slist_t             result = NULL;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      result = leoparse_try_get_slist(entry);
    }
  
  return result;
}

char *
leoparse_try_read_id(p_tbx_htable_t  htable,
		     const char     *key)
{
  p_leoparse_htable_entry_t  entry  = NULL;
  p_leoparse_object_t        object = NULL;
  char                      *result = NULL;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      object = leoparse_try_get_object(entry);
      result = leoparse_try_get_id(object);
    }

  return result;
}

char *
leoparse_try_read_string(p_tbx_htable_t  htable,
			 const char     *key)
{
  p_leoparse_htable_entry_t  entry  = NULL;
  p_leoparse_object_t        object = NULL;
  char                      *result = NULL;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      object = leoparse_try_get_object(entry);
      result = leoparse_try_get_string(object);
    }

  return result;
}

p_tbx_htable_t
leoparse_try_read_htable(p_tbx_htable_t  htable,
			 const char     *key)
{
  p_leoparse_htable_entry_t entry  = NULL;
  p_leoparse_object_t       object = NULL;
  p_tbx_htable_t            result = NULL;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      object = leoparse_try_get_object(entry);
      result = leoparse_try_get_htable(object);
    }

  return result;
}

int
leoparse_try_read_val(p_tbx_htable_t  htable,
		      const char     *key,
		      int             default_value)
{
  p_leoparse_htable_entry_t entry  = NULL;
  p_leoparse_object_t       object = NULL;
  int                       result = default_value;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      object = leoparse_try_get_object(entry);
      result = leoparse_try_get_val(object, result);
    }

  return result;
}

p_leoparse_range_t
leoparse_try_read_range(p_tbx_htable_t  htable,
			const char     *key)
{
  p_leoparse_htable_entry_t  entry  = NULL;
  p_leoparse_object_t        object = NULL;
  p_leoparse_range_t         result = NULL;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      object = leoparse_try_get_object(entry);
      result = leoparse_try_get_range(object);
    }

  return result;
}
