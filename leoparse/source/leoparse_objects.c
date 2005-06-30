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

/*
 * Init
 * ----
 */
p_leoparse_object_t
leoparse_init_object(void)
{
  p_leoparse_object_t result = NULL;

  LOG_IN();
  result = TBX_CALLOC(1, sizeof(leoparse_object_t));
  LOG_OUT();

  return result;
}

p_leoparse_object_t
leoparse_init_slist_object(p_tbx_slist_t slist)
{
  p_leoparse_object_t result = NULL;

  LOG_IN();
  result        = leoparse_init_object();
  result->type  = leoparse_o_slist;
  result->slist = slist;
  LOG_OUT();

  return result;
}

p_leoparse_object_t
leoparse_init_htable_object(p_tbx_htable_t htable)
{
  p_leoparse_object_t result = NULL;

  LOG_IN();
  result         = leoparse_init_object();
  result->type   = leoparse_o_htable;
  result->htable = htable;
  LOG_OUT();

  return result;
}

p_leoparse_object_t
leoparse_init_id_object(char                  *id,
			p_leoparse_modifier_t  modifier)
{
  p_leoparse_object_t result = NULL;

  LOG_IN();
  result           = leoparse_init_object();
  result->type     = leoparse_o_id;
  result->id       = id;
  result->modifier = modifier;
  LOG_OUT();

  return result;
}

p_leoparse_object_t
leoparse_init_string_object(char *string)
{
  p_leoparse_object_t result = NULL;

  LOG_IN();
  result         = leoparse_init_object();
  result->type   = leoparse_o_string;
  result->string = string;
  LOG_OUT();

  return result;
}

p_leoparse_object_t
leoparse_init_val_object(int value)
{
  p_leoparse_object_t result = NULL;

  LOG_IN();
  result       = leoparse_init_object();
  result->type = leoparse_o_integer;
  result->val  = value;
  LOG_OUT();

  return result;
}

p_leoparse_object_t
leoparse_init_range_object(p_leoparse_range_t range)
{
  p_leoparse_object_t result = NULL;

  LOG_IN();
  result        = leoparse_init_object();
  result->type  = leoparse_o_range;
  result->range = range;
  LOG_OUT();

  return result;
}

/*
 * Release
 * -------
 */
void
leoparse_release_object(p_leoparse_object_t object)
{
  LOG_IN();
  memset(object, 0, sizeof(leoparse_object_t));
  TBX_FREE(object);
  LOG_OUT();
}

/*
 * Read
 * ----
 */
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

/* - */

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

/* - */

p_tbx_slist_t
leoparse_extract_slist(p_tbx_htable_t  htable,
		    const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_slist_t       result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_extract(htable, key);

  if (object)
    {
      result = leoparse_get_slist(object);
      leoparse_release_object(object);
    }
  LOG_OUT();

  return result;
}

p_tbx_slist_t
leoparse_extract_as_slist(p_tbx_htable_t  htable,
		       const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_slist_t       result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_extract(htable, key);

  if (object)
    {
      result = leoparse_get_as_slist(object);
      leoparse_release_object(object);
    }
  LOG_OUT();

  return result;
}

char *
leoparse_extract_id(p_tbx_htable_t  htable,
		 const char     *key)
{
  p_leoparse_object_t  object = NULL;
  char                *result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_extract(htable, key);

  if (object)
    {
      result = leoparse_get_id(object);
      leoparse_release_object(object);
    }
  LOG_OUT();

  return result;
}

char *
leoparse_extract_string(p_tbx_htable_t  htable,
		     const char     *key)
{
  p_leoparse_object_t  object = NULL;
  char                *result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_extract(htable, key);

  if (object)
    {
      result = leoparse_get_string(object);
      leoparse_release_object(object);
    }
  LOG_OUT();

  return result;
}

p_tbx_htable_t
leoparse_extract_htable(p_tbx_htable_t  htable,
		     const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_htable_t      result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_extract(htable, key);

  if (object)
    {
      result = leoparse_get_htable(object);
      leoparse_release_object(object);
    }
  LOG_OUT();

  return result;
}

int
leoparse_extract_val(p_tbx_htable_t  htable,
		  const char     *key)
{
  p_leoparse_object_t object = NULL;
  int                 result =    0;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_extract(htable, key);

  if (object)
    {
      result = leoparse_get_val(object);
      leoparse_release_object(object);
    }
  LOG_OUT();

  return result;
}

p_leoparse_range_t
leoparse_extract_range(p_tbx_htable_t  htable,
		    const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_leoparse_range_t  result = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_extract(htable, key);

  if (object)
    {
      result = leoparse_get_range(object);
      leoparse_release_object(object);
    }
  LOG_OUT();

  return result;
}

/* - */
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


/*
 * Write
 * -----
 */
void
leoparse_convert_to_slist(p_tbx_htable_t  htable,
			  const char     *key)
{
  p_leoparse_object_t object = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_extract(htable, key);

  if (object)
    {
      if (object->type != leoparse_o_slist)
	{
	  p_tbx_slist_t slist = NULL;

	  slist = tbx_slist_nil();
	  tbx_slist_append(slist, object);

	  object = TBX_CALLOC(1, sizeof(leoparse_object_t));
	  object->type  = leoparse_o_slist;
	  object->slist = slist;
	}

      tbx_htable_add(htable, key, object);
    }
  else
    FAILURE("nothing to convert");

  LOG_OUT();
}

void
leoparse_write_slist(p_tbx_htable_t  htable,
		     const char     *key,
		     p_tbx_slist_t   slist)
{
  p_leoparse_object_t object = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (!object)
    {
      object        = TBX_CALLOC(1, sizeof(leoparse_object_t));
      object->type  = leoparse_o_slist;
      object->slist = slist;
    }
  else
    FAILURE("trying to overwrite an object");

  LOG_OUT();
}

void
leoparse_write_id(p_tbx_htable_t  htable,
		  const char     *key,
		  char           *id)
{
  p_leoparse_object_t object = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (!object)
    {
      object       = TBX_CALLOC(1, sizeof(leoparse_object_t));
      object->type = leoparse_o_id;
      object->id   = id;
    }
  else
    FAILURE("trying to overwrite an object");

  LOG_OUT();
}

void
leoparse_write_string(p_tbx_htable_t  htable,
		      const char     *key,
		      char           *string)
{
  p_leoparse_object_t object = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (!object)
    {
      object         = TBX_CALLOC(1, sizeof(leoparse_object_t));
      object->type   = leoparse_o_string;
      object->string = string;
    }
  else
    FAILURE("trying to overwrite an object");

  LOG_OUT();
}

void
leoparse_write_htable(p_tbx_htable_t  htable,
		      const char     *key,
		      p_tbx_htable_t  table)
{
  p_leoparse_object_t object = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (!object)
    {
      object         = TBX_CALLOC(1, sizeof(leoparse_object_t));
      object->type   = leoparse_o_htable;
      object->htable = table;
    }
  else
    FAILURE("trying to overwrite an object");

  LOG_OUT();
}

void
leoparse_write_val(p_tbx_htable_t  htable,
		   const char     *key,
		   int             val)
{
  p_leoparse_object_t object = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (!object)
    {
      object       = TBX_CALLOC(1, sizeof(leoparse_object_t));
      object->type = leoparse_o_integer;
      object->val  = val;
    }
  else
    FAILURE("trying to overwrite an object");

  LOG_OUT();
}

void
leoparse_write_range(p_tbx_htable_t      htable,
		     const char         *key,
		     p_leoparse_range_t  range)
{
  p_leoparse_object_t object = NULL;

  LOG_IN();
  LOG_STR("key", key);
  object = tbx_htable_get(htable, key);

  if (!object)
    {
      object        = TBX_CALLOC(1, sizeof(leoparse_object_t));
      object->type  = leoparse_o_range;
      object->range = range;
    }
  else
    FAILURE("trying to overwrite an object");

  LOG_OUT();
}


