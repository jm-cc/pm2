
/*
 * tbx_htable_management.c
 * -----------------------
 */

#include "tbx.h"

/* 
 * Macros
 * ------
 */
#define INITIAL_HTABLE_ELEMENT 256
#define DEFAULT_BUCKET_COUNT    17

/*
 * Data structures
 * ---------------
 */
static p_tbx_memory_t tbx_htable_manager_memory = NULL;

/*
 * Functions
 * =========
 */

/*
 * Initialization
 * --------------
 */
void
tbx_htable_manager_init(void)
{
  LOG_IN();
  tbx_malloc_init(&tbx_htable_manager_memory,
		  sizeof(tbx_htable_element_t),
		  INITIAL_HTABLE_ELEMENT);
  LOG_OUT();
}

void
tbx_htable_init(p_tbx_htable_t            htable,
		tbx_htable_bucket_count_t buckets)
{
  LOG_IN();
  if (!buckets)
    {
      buckets = DEFAULT_BUCKET_COUNT;
    }
  
  htable->nb_bucket    = buckets;
  htable->bucket_array = TBX_MALLOC(buckets * sizeof(tbx_htable_element_t));
  htable->nb_element   = 0;

  while (buckets--)
    {
      htable->bucket_array[buckets] = NULL;
    }
  LOG_OUT();
}

tbx_bool_t
tbx_htable_empty(p_tbx_htable_t htable)
{
  tbx_bool_t test = tbx_false;
  
  LOG_IN();
  test = (htable->nb_element == 0);
  LOG_OUT();  

  return test;
}

tbx_htable_element_count_t
tbx_htable_get_size(p_tbx_htable_t htable)
{
  tbx_htable_element_count_t count = -1;
  
  LOG_IN();
  count = htable->nb_element;
  LOG_OUT();
  
  return count;
}

static
tbx_htable_bucket_count_t
__tbx_htable_get_bucket(p_tbx_htable_t   htable,
			tbx_htable_key_t key)
{
  tbx_htable_bucket_count_t bucket  = 0;
  size_t                    key_len = 0;

  LOG_IN();
  key_len = strlen(key);

  while(key_len--)
    {
      bucket += key[key_len];
    }

  bucket %= htable->nb_bucket;
  LOG_OUT();

  return bucket;
}

void
tbx_htable_add(p_tbx_htable_t    htable,
	       tbx_htable_key_t  key,
	       void             *object)
{
  tbx_htable_bucket_count_t bucket  = -1;
  p_tbx_htable_element_t    element = NULL;
  
  LOG_IN();
  bucket  = __tbx_htable_get_bucket(htable, key);
  element = tbx_malloc(tbx_htable_manager_memory);  
  element->key = TBX_MALLOC(strlen(key) + 1);
  CTRL_ALLOC(element->key);

  strcpy(element->key, key);
  element->object = object;
  element->next   = htable->bucket_array[bucket];
  
  htable->bucket_array[bucket] = element;
  htable->nb_element++;
  LOG_OUT();
}

void *
tbx_htable_get(p_tbx_htable_t   htable,
	       tbx_htable_key_t key)
{
  tbx_htable_bucket_count_t bucket  = -1;
  p_tbx_htable_element_t    element = NULL;
  
  LOG_IN();
  bucket  = __tbx_htable_get_bucket(htable, key);
  element = htable->bucket_array[bucket];
  
  while(element)
    {
      if (!strcmp(key, element->key))
	{
	  void *object = NULL;
	  
	  object = element->object;
	  LOG_OUT();

	  return object;
	}

      element = element->next;
    }  
  LOG_OUT();

  return NULL;
}

void *
tbx_htable_extract(p_tbx_htable_t   htable,
		   tbx_htable_key_t key)
{
  tbx_htable_bucket_count_t  bucket  = -1;
  p_tbx_htable_element_t    *element = NULL;
  
  LOG_IN();
  bucket  = __tbx_htable_get_bucket(htable, key);
  element = &(htable->bucket_array[bucket]);
  
  while (*element)
    {
      if (strcmp(key, (*element)->key))
	{
	  element = &((*element)->next);
	}
      else
	{
	  p_tbx_htable_element_t  tmp_element = NULL;
	  void                   *object      = NULL;

	  tmp_element = *element;

	  object   = tmp_element->object;
	  tmp_element->object = NULL;

	  *element = tmp_element->next;
	  tmp_element->next = NULL;

	  TBX_FREE(tmp_element->key);
	  tmp_element->key = NULL;

	  tbx_free(tbx_htable_manager_memory, tmp_element);

	  LOG_OUT();
	  return object;
	}
    }
  
  LOG_OUT();
  return NULL;
}

void
tbx_htable_free(p_tbx_htable_t htable)
{
  LOG_IN();
  if (htable->nb_element)
    {
      while (htable->nb_bucket--)
	{
	  p_tbx_htable_element_t element = NULL;

	  element = htable->bucket_array[htable->nb_bucket];

	  while (element)
	    {
	      p_tbx_htable_element_t temp = NULL;

	      temp = element->next;
	      
	      TBX_FREE(element->key);
	      element->key    = NULL;
	      element->object = NULL;
	      element->next   = NULL;

	      tbx_free(tbx_htable_manager_memory, element);
	      
	      element = temp;
	    }

	  htable->bucket_array[htable->nb_bucket] = NULL;
	}  
    }

  htable->nb_element = 0;
  TBX_FREE(htable->bucket_array);
  htable->bucket_array = NULL;
  LOG_OUT();
}

void
tbx_htable_manager_exit(void)
{
  LOG_IN();
  tbx_malloc_clean(tbx_htable_manager_memory);
  LOG_OUT();
}

p_tbx_slist_t
tbx_htable_get_key_slist(p_tbx_htable_t htable)
{
  p_tbx_slist_t slist = NULL;

  LOG_IN();
  slist = tbx_slist_nil();

  if (htable->nb_element)
    {
      while (htable->nb_bucket--)
	{
	  p_tbx_htable_element_t element = NULL;

	  element = htable->bucket_array[htable->nb_bucket];

	  while (element)
	    {
	      char *key_copy = NULL;
	      
	      key_copy = TBX_MALLOC(strlen(element->key) + 1);
	      strcpy(key_copy, element->key);

	      tbx_slist_append(slist, key_copy);

	      element = element->next;
	    }
	}  
    }
  LOG_OUT();

  return tbx_slist_nil();
}
