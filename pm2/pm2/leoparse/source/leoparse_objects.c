#include "leoparse.h"

/*
 * leoparse_objects.c
 * ------------------
 */
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

p_tbx_slist_t
leoparse_read_slist(p_tbx_htable_t  htable,
		    char           *key)
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
		 char           *key)
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
		     char           *key)
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
		     char           *key)
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
