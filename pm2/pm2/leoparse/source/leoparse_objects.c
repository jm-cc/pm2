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

p_tbx_slist_t
leoparse_get_slist(p_leoparse_object_t object)
{
  p_tbx_slist_t result = NULL;
  
  LOG_IN();
  if (object->type == leoparse_o_slist)
    {
      result = object->slist;
    }
  else
    FAILURE("parse error: list expected");

  LOG_OUT();

  return result;
}

p_tbx_slist_t
leoparse_get_as_slist(p_leoparse_object_t object)
{
  p_tbx_slist_t result = NULL;

  LOG_IN();
  if (object->type == leoparse_o_slist)
    {
      result = object->slist;
    }
  else
    {
      result = tbx_slist_nil();
      tbx_slist_append(result, object);
    }
  LOG_OUT();
  
  return result;
}

p_tbx_htable_t
leoparse_get_htable(p_leoparse_object_t object)
{
  p_tbx_htable_t result = NULL;
  
  LOG_IN();
  if (object->type == leoparse_o_htable)
    {
      result = object->htable;
    }
  else
    FAILURE("parse error: table expected");

  LOG_OUT();
  
  return result;
}

char *
leoparse_get_string(p_leoparse_object_t object)
{
  char *result = NULL;
  
  LOG_IN();
  if (object->type == leoparse_o_string)
    {
      result = object->string;
    }
  else
    FAILURE("parse error: string expected");

  LOG_OUT();
  
  return result;
}

char *
leoparse_get_id(p_leoparse_object_t object)
{
  char *result = NULL;

  LOG_IN();
  if (object->type == leoparse_o_id)
    {
      result = object->id;
    }
  else
    FAILURE("parse error: id expected");

  LOG_OUT();

  return result;
}

int
leoparse_get_val(p_leoparse_object_t object)
{
  int result = 0;

  LOG_IN();
  if (object->type == leoparse_o_integer)
    {
      result = object->val;
    }
  else
    FAILURE("parse error: integer expected");

  LOG_OUT();

  return result;
}

p_leoparse_range_t
leoparse_get_range(p_leoparse_object_t object)
{
  p_leoparse_range_t result = NULL;
  
  LOG_IN();
  if (object->type == leoparse_o_range)
    {
      result = object->range;
    }
  else
    FAILURE("parse error: range expected");

  LOG_OUT();

  return result;
}

p_tbx_slist_t
leoparse_read_slist(p_tbx_htable_t  htable,
		    const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_slist_t       result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_get_slist(object);
    }  
  LOG_OUT();
  
  return result;
}

p_tbx_slist_t
leoparse_read_as_slist(p_tbx_htable_t  htable,
		       const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_slist_t       result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_get_as_slist(object);
    }
  LOG_OUT();
  
  return result;
}

char *
leoparse_read_id(p_tbx_htable_t  htable,
		 const char     *key)
{
  p_leoparse_object_t  object = NULL;
  char                *result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_get_id(object);
    }
  LOG_OUT();

  return result;
}

char *
leoparse_read_string(p_tbx_htable_t  htable,
		     const char     *key)
{
  p_leoparse_object_t  object = NULL;
  char                *result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_get_string(object);
    }
  LOG_OUT();

  return result;
}

p_tbx_htable_t
leoparse_read_htable(p_tbx_htable_t  htable,
		     const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_htable_t      result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_get_htable(object);
    }
  LOG_OUT();
  
  return result;
}

int
leoparse_read_val(p_tbx_htable_t  htable,
		  const char     *key)
{
  p_leoparse_object_t object = NULL;
  int                 result =    0;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_get_val(object);
    }
  LOG_OUT();
  
  return result;
}

p_leoparse_range_t
leoparse_read_range(p_tbx_htable_t  htable,
		    const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_leoparse_range_t  result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_get_range(object);
    }
  LOG_OUT();

  return result;
}

p_tbx_slist_t
leoparse_try_get_slist(p_leoparse_object_t object)
{
  p_tbx_slist_t result = NULL;

  LOG_IN();
  if (object->type == leoparse_o_slist)
    {
      result = object->slist;
    }
  LOG_OUT();
  
  return result;
}

p_tbx_slist_t
leoparse_try_get_as_slist(p_leoparse_object_t object)
{
  p_tbx_slist_t result = NULL;

  LOG_IN();
  result = leoparse_get_as_slist(object);
  LOG_OUT();

  return result;
}

p_tbx_htable_t
leoparse_try_get_htable(p_leoparse_object_t object)
{
  p_tbx_htable_t result = NULL;

  LOG_IN();
  if (object->type == leoparse_o_htable)
    {
      result = object->htable;
    }
  LOG_OUT();

  return result;
}

char *
leoparse_try_get_string(p_leoparse_object_t object)
{
  char *result = NULL;

  LOG_IN();
  if (object->type == leoparse_o_string)
    {
      result = object->string;
    }
  LOG_OUT();
  
  return result;
}

char *
leoparse_try_get_id(p_leoparse_object_t object)
{
  char *result = NULL;
  
  LOG_IN();
  if (object->type == leoparse_o_id)
    {
      result = object->id;
    }
  LOG_OUT();

  return result;
}

int
leoparse_try_get_val(p_leoparse_object_t object,
		     int                 default_value)
{
  int result = 0;

  LOG_IN();
  if (object->type == leoparse_o_integer)
    {
      result = object->val;
    }
  LOG_OUT();

  return result;
}

p_leoparse_range_t
leoparse_try_get_range(p_leoparse_object_t object)
{
  p_leoparse_range_t result = NULL;
  
  LOG_IN();
  if (object->type == leoparse_o_range)
    {
      result = object->range;
    }
  LOG_OUT();

  return result;
}

p_tbx_slist_t
leoparse_try_read_slist(p_tbx_htable_t  htable,
			const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_slist_t       result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_try_get_slist(object);
    }
  LOG_OUT();
  
  return result;
}

p_tbx_slist_t
leoparse_try_read_as_slist(p_tbx_htable_t  htable,
			const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_slist_t       result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_try_get_as_slist(object);
    }
  LOG_OUT();
  
  return result;
}

char *
leoparse_try_read_id(p_tbx_htable_t  htable,
		     const char     *key)
{
  p_leoparse_object_t  object = NULL;
  char                *result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_try_get_id(object);
    }
  LOG_OUT();
  
  return result;
}

char *
leoparse_try_read_string(p_tbx_htable_t  htable,
			 const char     *key)
{
  p_leoparse_object_t  object = NULL;
  char                *result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_try_get_string(object);
    }
  LOG_OUT();

  return result;
}

p_tbx_htable_t
leoparse_try_read_htable(p_tbx_htable_t  htable,
			 const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_htable_t      result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_try_get_htable(object);
    }
  LOG_OUT();

  return result;
}

int
leoparse_try_read_val(p_tbx_htable_t  htable,
		      const char     *key,
		      int             default_value)
{
  p_leoparse_object_t object = NULL;
  int                 result = default_value;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_try_get_val(object, result);
    }
  LOG_OUT();
  
  return result;
}

p_leoparse_range_t
leoparse_try_read_range(p_tbx_htable_t  htable,
			const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_leoparse_range_t  result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_try_get_range(object);
    }
  LOG_OUT();
  
  return result;
}
