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
#define DEFAULT_BUCKET_COUNT 17

/*
 * Data structures
 * ---------------
 */
static p_tbx_memory_t tbx_htable_manager_memory;

/*
 * Functions
 * =========
 */

/*
 * Initialization
 * --------------
 */
void
tbx_htable_manager_init()
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

  while(buckets--)
    {
      htable->bucket_array[buckets] = NULL;
    }
  LOG_OUT();
}

tbx_bool_t
tbx_htable_empty(p_tbx_htable_t htable)
{
  LOG_IN();
  LOG_OUT();  
  return (htable->nb_element == 0);
}

tbx_htable_element_count_t
tbx_htable_get_size(p_tbx_htable_t htable)
{
  LOG_IN();
  LOG_OUT();  
  return htable->nb_element;
}

static tbx_htable_bucket_count_t
tbx_htable_get_bucket(p_tbx_htable_t   htable,
		      tbx_htable_key_t key)
{
  tbx_htable_bucket_count_t bucket  = 0;
  size_t                    key_len = strlen(key);

  LOG_IN();
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
  bucket  = tbx_htable_get_bucket(htable, key);
  element = tbx_malloc(tbx_htable_manager_memory);
  CTRL_ALLOC(element);
  
  element->key = TBX_MALLOC(strlen(key) + 1);
  strcpy(element->key, key);
  element->object = object;
  element->next = htable->bucket_array[bucket];
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
  bucket  = tbx_htable_get_bucket(htable, key);
  element = htable->bucket_array[bucket];
  
  while(element)
    {
      if (strcmp(key, element->key))
	{
	  element = element->next;
	}
      else
	{
	  LOG_OUT();
	  return element->object;
	}
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
  bucket  = tbx_htable_get_bucket(htable, key);
  element = &(htable->bucket_array[bucket]);
  
  while(*element)
    {
      if (strcmp(key, element->key))
	{
	  element = &(*element->next);
	}
      else
	{
	  void *object = (*element)->object;
	  
	  tbx_free(tbx_htable_manager_memory, *element);
	  *element = NULL;
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
  tbx_htable_bucket_count_t bucket  = -1;
 
  LOG_IN();
  if (htable->nb_element)
    {
      for (bucket = 0; bucket < htable->nb_bucket; bucket++)
	{
	  p_tbx_htable_element_t element = htable->bucket_array[bucket];

	  while(element)
	    {
	      p_tbx_htable_element_t temp = element->next;
	      
	      tbx_free(tbx_htable_manager_memory, *element);
	      element = temp;
	    }

	  htable->bucket_array[bucket] = NULL;
	}  
    }

  htable->nb_element   = 0;
  TBX_FREE(htable->bucket_array);
  htable->bucket_array = NULL;
  htable->nb_bucket    = 0;
  LOG_OUT();
}

void
tbx_htable_manager_exit()
{
  LOG_IN();
  tbx_malloc_clean(tbx_htable_manager_memory);
  LOG_OUT();
}
