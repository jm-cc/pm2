/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                           

  NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: tbx_htable_management.c,v $
Revision 1.4  2000/12/19 16:57:51  oaumage
- finalisation de leoparse
- exemples pour leoparse
- modification des macros de logging
- version typesafe de certaines macros
- finalisation des tables de hachage
- finalisation des listes de recherche

Revision 1.3  2000/09/05 13:59:39  oaumage
- reecriture des slists et corrections diverses au niveau des htables

Revision 1.2  2000/07/10 14:25:58  oaumage
- Corrections diverses

Revision 1.1  2000/07/07 14:49:49  oaumage
- Ajout d'un support pour les tables de hachage


______________________________________________________________________________

*/

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
  LOG_PTR("*** htable", htable);
  LOG_STR("*** key", key);
  LOG_VAL("*** len", key_len);

  while(key_len--)
    {
      bucket += key[key_len];
    }

  LOG_VAL("*** pre_bucket", bucket);
  bucket %= htable->nb_bucket;
  LOG_VAL("*** post_bucket", bucket);
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
  LOG_PTR("htable", htable);
  LOG_PTR("object", object);
  LOG_STR("key", key);
  
  bucket  = __tbx_htable_get_bucket(htable, key);

  LOG_VAL("bucket", bucket);

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
  LOG_PTR("htable", htable);
  LOG_STR("key", key);
  LOG_VAL("bucket", bucket);
  
  element = htable->bucket_array[bucket];
  
  while(element)
    {
      if (!strcmp(key, element->key))
	{
	  void *object = NULL;
	  
	  object = element->object;
	  LOG_PTR("object", object);
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
