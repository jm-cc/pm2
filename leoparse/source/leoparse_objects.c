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

  result = TBX_CALLOC(1, sizeof(leoparse_object_t));

  return result;
}

p_leoparse_object_t
leoparse_init_slist_object(p_tbx_slist_t slist)
{
  p_leoparse_object_t result = NULL;

  result        = leoparse_init_object();
  result->type  = leoparse_o_slist;
  result->slist = slist;

  return result;
}

p_leoparse_object_t
leoparse_init_htable_object(p_tbx_htable_t htable)
{
  p_leoparse_object_t result = NULL;

  result         = leoparse_init_object();
  result->type   = leoparse_o_htable;
  result->htable = htable;

  return result;
}

p_leoparse_object_t
leoparse_init_id_object(char                  *id,
			p_leoparse_modifier_t  modifier)
{
  p_leoparse_object_t result = NULL;

  result           = leoparse_init_object();
  result->type     = leoparse_o_id;
  result->id       = id;
  result->modifier = modifier;

  return result;
}

p_leoparse_object_t
leoparse_init_string_object(char *string)
{
  p_leoparse_object_t result = NULL;

  result         = leoparse_init_object();
  result->type   = leoparse_o_string;
  result->string = string;

  return result;
}

p_leoparse_object_t
leoparse_init_val_object(int value)
{
  p_leoparse_object_t result = NULL;

  result       = leoparse_init_object();
  result->type = leoparse_o_integer;
  result->val  = value;

  return result;
}

p_leoparse_object_t
leoparse_init_range_object(p_leoparse_range_t range)
{
  p_leoparse_object_t result = NULL;

  result        = leoparse_init_object();
  result->type  = leoparse_o_range;
  result->range = range;

  return result;
}

/*
 * Release
 * -------
 */
void
leoparse_release_object(p_leoparse_object_t object)
{
  memset(object, 0, sizeof(leoparse_object_t));
  TBX_FREE(object);
}

/*
 * Read
 * ----
 */
p_tbx_slist_t
leoparse_get_slist(p_leoparse_object_t object)
{
  p_tbx_slist_t result = NULL;

  if (object->type == leoparse_o_slist)
    {
      result = object->slist;
    }
  else
    TBX_FAILURE("parse error: list expected");


  return result;
}

p_tbx_slist_t
leoparse_get_as_slist(p_leoparse_object_t object)
{
  p_tbx_slist_t result = NULL;

  if (object->type == leoparse_o_slist)
    {
      result = object->slist;
    }
  else
    {
      result = tbx_slist_nil();
      tbx_slist_append(result, object);
    }

  return result;
}

p_tbx_htable_t
leoparse_get_htable(p_leoparse_object_t object)
{
  p_tbx_htable_t result = NULL;

  if (object->type == leoparse_o_htable)
    {
      result = object->htable;
    }
  else
    TBX_FAILURE("parse error: table expected");

  return result;
}

char *
leoparse_get_string(p_leoparse_object_t object)
{
  char *result = NULL;

  if (object->type == leoparse_o_string)
    {
      result = object->string;
    }
  else
    TBX_FAILURE("parse error: string expected");

  return result;
}

char *
leoparse_get_id(p_leoparse_object_t object)
{
  char *result = NULL;

  if (object->type == leoparse_o_id)
    {
      result = object->id;
    }
  else
    TBX_FAILURE("parse error: id expected");

  return result;
}

int
leoparse_get_val(p_leoparse_object_t object)
{
  int result = 0;

  if (object->type == leoparse_o_integer)
    {
      result = object->val;
    }
  else
    TBX_FAILURE("parse error: integer expected");

  return result;
}

p_leoparse_range_t
leoparse_get_range(p_leoparse_object_t object)
{
  p_leoparse_range_t result = NULL;

  if (object->type == leoparse_o_range)
    {
      result = object->range;
    }
  else
    TBX_FAILURE("parse error: range expected");

  return result;
}

/* - */

p_tbx_slist_t
leoparse_read_slist(p_tbx_htable_t  htable,
		    const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_slist_t       result = NULL;

  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_get_slist(object);
    }
  return result;
}

p_tbx_slist_t
leoparse_read_as_slist(p_tbx_htable_t  htable,
		       const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_slist_t       result = NULL;

  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_get_as_slist(object);
    }

  return result;
}

char *
leoparse_read_id(p_tbx_htable_t  htable,
		 const char     *key)
{
  p_leoparse_object_t  object = NULL;
  char                *result = NULL;

  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_get_id(object);
    }
  return result;
}

char *
leoparse_read_string(p_tbx_htable_t  htable,
		     const char     *key)
{
  p_leoparse_object_t  object = NULL;
  char                *result = NULL;

  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_get_string(object);
    }

  return result;
}

p_tbx_htable_t
leoparse_read_htable(p_tbx_htable_t  htable,
		     const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_htable_t      result = NULL;

  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_get_htable(object);
    }

  return result;
}

int
leoparse_read_val(p_tbx_htable_t  htable,
		  const char     *key)
{
  p_leoparse_object_t object = NULL;
  int                 result =    0;

  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_get_val(object);
    }

  return result;
}

p_leoparse_range_t
leoparse_read_range(p_tbx_htable_t  htable,
		    const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_leoparse_range_t  result = NULL;


  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_get_range(object);
    }

  return result;
}

/* - */

p_tbx_slist_t
leoparse_extract_slist(p_tbx_htable_t  htable,
		    const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_slist_t       result = NULL;

  object = tbx_htable_extract(htable, key);

  if (object)
    {
      result = leoparse_get_slist(object);
      leoparse_release_object(object);
    }

  return result;
}

p_tbx_slist_t
leoparse_extract_as_slist(p_tbx_htable_t  htable,
		       const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_slist_t       result = NULL;

  object = tbx_htable_extract(htable, key);

  if (object)
    {
      result = leoparse_get_as_slist(object);
      leoparse_release_object(object);
    }

  return result;
}

char *
leoparse_extract_id(p_tbx_htable_t  htable,
		 const char     *key)
{
  p_leoparse_object_t  object = NULL;
  char                *result = NULL;

  object = tbx_htable_extract(htable, key);

  if (object)
    {
      result = leoparse_get_id(object);
      leoparse_release_object(object);
    }

  return result;
}

char *
leoparse_extract_string(p_tbx_htable_t  htable,
		     const char     *key)
{
  p_leoparse_object_t  object = NULL;
  char                *result = NULL;

  object = tbx_htable_extract(htable, key);

  if (object)
    {
      result = leoparse_get_string(object);
      leoparse_release_object(object);
    }

  return result;
}

p_tbx_htable_t
leoparse_extract_htable(p_tbx_htable_t  htable,
		     const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_htable_t      result = NULL;

  object = tbx_htable_extract(htable, key);

  if (object)
    {
      result = leoparse_get_htable(object);
      leoparse_release_object(object);
    }

  return result;
}

int
leoparse_extract_val(p_tbx_htable_t  htable,
		  const char     *key)
{
  p_leoparse_object_t object = NULL;
  int                 result =    0;

  object = tbx_htable_extract(htable, key);

  if (object)
    {
      result = leoparse_get_val(object);
      leoparse_release_object(object);
    }

  return result;
}

p_leoparse_range_t
leoparse_extract_range(p_tbx_htable_t  htable,
		    const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_leoparse_range_t  result = NULL;

  object = tbx_htable_extract(htable, key);

  if (object)
    {
      result = leoparse_get_range(object);
      leoparse_release_object(object);
    }

  return result;
}

/* - */
p_tbx_slist_t
leoparse_try_get_slist(p_leoparse_object_t object)
{
  p_tbx_slist_t result = NULL;

  if (object->type == leoparse_o_slist)
    {
      result = object->slist;
    }

  return result;
}

p_tbx_slist_t
leoparse_try_get_as_slist(p_leoparse_object_t object)
{
  p_tbx_slist_t result = NULL;

  result = leoparse_get_as_slist(object);

  return result;
}

p_tbx_htable_t
leoparse_try_get_htable(p_leoparse_object_t object)
{
  p_tbx_htable_t result = NULL;

  if (object->type == leoparse_o_htable)
    {
      result = object->htable;
    }

  return result;
}

char *
leoparse_try_get_string(p_leoparse_object_t object)
{
  char *result = NULL;

  if (object->type == leoparse_o_string)
    {
      result = object->string;
    }

  return result;
}

char *
leoparse_try_get_id(p_leoparse_object_t object)
{
  char *result = NULL;

  if (object->type == leoparse_o_id)
    {
      result = object->id;
    }

  return result;
}

int
leoparse_try_get_val(p_leoparse_object_t object,
		     int                 default_value)
{
  int result = 0;

  if (object->type == leoparse_o_integer)
    {
      result = object->val;
    }

  return result;
}

p_leoparse_range_t
leoparse_try_get_range(p_leoparse_object_t object)
{
  p_leoparse_range_t result = NULL;

  if (object->type == leoparse_o_range)
    {
      result = object->range;
    }

  return result;
}

p_tbx_slist_t
leoparse_try_read_slist(p_tbx_htable_t  htable,
			const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_slist_t       result = NULL;

  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_try_get_slist(object);
    }

  return result;
}

p_tbx_slist_t
leoparse_try_read_as_slist(p_tbx_htable_t  htable,
			const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_slist_t       result = NULL;

  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_try_get_as_slist(object);
    }

  return result;
}

char *
leoparse_try_read_id(p_tbx_htable_t  htable,
		     const char     *key)
{
  p_leoparse_object_t  object = NULL;
  char                *result = NULL;

  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_try_get_id(object);
    }

  return result;
}

char *
leoparse_try_read_string(p_tbx_htable_t  htable,
			 const char     *key)
{
  p_leoparse_object_t  object = NULL;
  char                *result = NULL;

  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_try_get_string(object);
    }

  return result;
}

p_tbx_htable_t
leoparse_try_read_htable(p_tbx_htable_t  htable,
			 const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_tbx_htable_t      result = NULL;

  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_try_get_htable(object);
    }

  return result;
}

int
leoparse_try_read_val(p_tbx_htable_t  htable,
		      const char     *key,
		      int             default_value)
{
  p_leoparse_object_t object = NULL;
  int                 result = default_value;

  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_try_get_val(object, result);
    }

  return result;
}

p_leoparse_range_t
leoparse_try_read_range(p_tbx_htable_t  htable,
			const char     *key)
{
  p_leoparse_object_t object = NULL;
  p_leoparse_range_t  result = NULL;

  object = tbx_htable_get(htable, key);

  if (object)
    {
      result = leoparse_try_get_range(object);
    }

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
    TBX_FAILURE("nothing to convert");

}

void
leoparse_write_slist(p_tbx_htable_t  htable,
		     const char     *key,
		     p_tbx_slist_t   slist)
{
  p_leoparse_object_t object = NULL;

  object = tbx_htable_get(htable, key);

  if (!object)
    {
      object        = TBX_CALLOC(1, sizeof(leoparse_object_t));
      object->type  = leoparse_o_slist;
      object->slist = slist;
    }
  else
    TBX_FAILURE("trying to overwrite an object");

}

void
leoparse_write_id(p_tbx_htable_t  htable,
		  const char     *key,
		  char           *id)
{
  p_leoparse_object_t object = NULL;

  object = tbx_htable_get(htable, key);

  if (!object)
    {
      object       = TBX_CALLOC(1, sizeof(leoparse_object_t));
      object->type = leoparse_o_id;
      object->id   = id;
    }
  else
    TBX_FAILURE("trying to overwrite an object");

}

void
leoparse_write_string(p_tbx_htable_t  htable,
		      const char     *key,
		      char           *string)
{
  p_leoparse_object_t object = NULL;

  object = tbx_htable_get(htable, key);

  if (!object)
    {
      object         = TBX_CALLOC(1, sizeof(leoparse_object_t));
      object->type   = leoparse_o_string;
      object->string = string;
    }
  else
    TBX_FAILURE("trying to overwrite an object");

}

void
leoparse_write_htable(p_tbx_htable_t  htable,
		      const char     *key,
		      p_tbx_htable_t  table)
{
  p_leoparse_object_t object = NULL;

  object = tbx_htable_get(htable, key);

  if (!object)
    {
      object         = TBX_CALLOC(1, sizeof(leoparse_object_t));
      object->type   = leoparse_o_htable;
      object->htable = table;
    }
  else
    TBX_FAILURE("trying to overwrite an object");

}

void
leoparse_write_val(p_tbx_htable_t  htable,
		   const char     *key,
		   int             val)
{
  p_leoparse_object_t object = NULL;

  object = tbx_htable_get(htable, key);

  if (!object)
    {
      object       = TBX_CALLOC(1, sizeof(leoparse_object_t));
      object->type = leoparse_o_integer;
      object->val  = val;
    }
  else
    TBX_FAILURE("trying to overwrite an object");

}

void
leoparse_write_range(p_tbx_htable_t      htable,
		     const char         *key,
		     p_leoparse_range_t  range)
{
  p_leoparse_object_t object = NULL;

  object = tbx_htable_get(htable, key);

  if (!object)
    {
      object        = TBX_CALLOC(1, sizeof(leoparse_object_t));
      object->type  = leoparse_o_range;
      object->range = range;
    }
  else
    TBX_FAILURE("trying to overwrite an object");

}

void
leoparse_dump_object_slist(p_tbx_slist_t l) {
        p_leoparse_object_t _o;

        if (tbx_slist_is_nil(l))
                return;

        tbx_slist_ref_to_head(l);
        _o = tbx_slist_ref_get(l);
        leoparse_dump_object(_o);

        while (tbx_slist_ref_forward(l)) {
                fprintf(stderr, ", ");
                _o = tbx_slist_ref_get(l);
                leoparse_dump_object(_o);
        }
}

void
leoparse_dump_object_htable(p_tbx_htable_t h) {
        p_tbx_slist_t  const l = tbx_htable_get_key_slist(h);
        char                *_k;
        p_leoparse_object_t  _o;

        if (tbx_slist_is_nil(l))
                goto out;

        _k = tbx_slist_extract(l);
        fprintf(stderr, "%s: ", _k);

        _o = tbx_htable_get(h, _k);
        TBX_FREE(_k);
        leoparse_dump_object(_o);
        fprintf(stderr, ";");

        while (!tbx_slist_is_nil(l)) {
                _k = tbx_slist_extract(l);
                fprintf(stderr, " %s: ", _k);

                _o = tbx_htable_get(h, _k);
                TBX_FREE(_k);
                leoparse_dump_object(_o);
                fprintf(stderr, ";");
        }

 out:
        tbx_slist_free(l);
}

void
leoparse_dump_object(p_leoparse_object_t object) {
        switch (object->type) {
        case  leoparse_o_id: {
                fprintf(stderr, "'%s'", object->id);
        }
                break;
        case leoparse_o_string: {
                fprintf(stderr, "\"%s\"", object->string);
        }
                break;
        case leoparse_o_htable: {
                p_tbx_htable_t const h = object->htable;

                fprintf(stderr, "{");
                leoparse_dump_object_htable(h);
                fprintf(stderr, "}");
        }
                break;
        case leoparse_o_integer: {
                fprintf(stderr, "%d", object->val);
        }
                break;
        case leoparse_o_range: {
                fprintf(stderr, "%d..%d", object->range->begin, object->range->end);
        }
                break;
        case leoparse_o_slist: {
                p_tbx_slist_t const l = object->slist;
                fprintf(stderr, "(");
                leoparse_dump_object_slist(l);
                fprintf(stderr, ")");
        }
                break;

        default:
                TBX_FAILURE("unknown object type");
        }

        if (object->modifier) {
                switch (object->modifier->type) {
                case leoparse_m_sbracket: {
                        fprintf(stderr, "[");
                        leoparse_dump_object_slist(object->modifier->sbracket);
                        fprintf(stderr, "]");
                }
                        break;
                case leoparse_m_parenthesis: {
                        fprintf(stderr, "(");
                        leoparse_dump_object_slist(object->modifier->parenthesis);
                        fprintf(stderr, ")");
                }
                        break;
                default:
                        TBX_FAILURE("unknown object modifier type");
                }
        }
}
