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
$Log: tbx_slist_management.c,v $
Revision 1.4  2000/06/05 11:38:02  oaumage
- Ajout d'une fonction de duplication
- Corrections diverses

Revision 1.3  2000/05/31 14:24:50  oaumage
- Ajout au niveau des slists

Revision 1.2  2000/05/22 13:45:55  oaumage
- correction de bugs divers

Revision 1.1  2000/05/22 12:20:10  oaumage
- Listes de recherche


______________________________________________________________________________
*/

/*
 * tbx_slist_management.c
 * ----------------------
 */

#include "tbx.h"

/* MACROS */
#define INITIAL_SLIST_ELEMENT 256

/* data structures */
static p_tbx_memory_t tbx_slist_manager_memory;

/* functions */

/*
 * Initialization
 * --------------
 */
void tbx_slist_manager_init()
{
  LOG_IN();
  tbx_malloc_init(&tbx_slist_manager_memory,
		  sizeof(tbx_slist_element_t),
		  INITIAL_SLIST_ELEMENT);
  LOG_OUT();
}

void tbx_slist_init(p_tbx_slist_t slist)
{
  LOG_IN();
  slist->length = 0;
  slist->head   = NULL;
  slist->tail   = NULL;
  LOG_OUT();
}

/*
 * Destruction
 * --------------
 */
void tbx_slist_manager_exit()
{
  LOG_IN();
  tbx_malloc_clean(tbx_slist_manager_memory);
  LOG_OUT();
}

tbx_bool_t
tbx_slist_empty(p_tbx_slist_t slist)
{
  LOG_IN();
  LOG_OUT();
  return (slist->length == 0);
}

void
tbx_slist_append_head(p_tbx_slist_t  slist,
		      void          *object)
{
  p_tbx_slist_element_t slist_element = NULL;

  LOG_IN();
  slist_element = tbx_malloc(tbx_slist_manager_memory);

  slist_element->object = object;
  if (slist->length)
    {
      slist_element->next     = slist->head;
      slist_element->previous = NULL;
      slist->head->previous   = slist_element;
    }
  else
    {
      slist_element->next     = NULL;
      slist_element->previous = NULL;
      slist->head             = slist_element;
      slist->tail             = slist_element;
    }
  slist->length++;
  LOG_OUT();
}


void
tbx_slist_append_tail(p_tbx_slist_t  slist,
		      void          *object)
{
  p_tbx_slist_element_t slist_element = NULL;

  LOG_IN();
  slist_element = tbx_malloc(tbx_slist_manager_memory);

  slist_element->object = object;
  if (slist->length)
    {
      slist_element->next     = NULL;
      slist_element->previous = slist->tail;
      slist->tail->next       = slist_element;
    }
  else
    {
      slist_element->next     = NULL;
      slist_element->previous = NULL;
      slist->head             = slist_element;
      slist->tail             = slist_element;
    }
  slist->length++;
  LOG_OUT();
}

void *
tbx_slist_extract_head(p_tbx_slist_t  slist)
{
  LOG_IN();
  if (slist->length)
    {
      p_tbx_slist_element_t  slist_element = slist->head;
      void                  *object        = slist_element->object;

      if (--(slist->length))
	{
	  slist->head = slist_element->next;
	}
      else
	{
	  slist->head = NULL;
	  slist->tail = NULL;
	}

      tbx_free(tbx_slist_manager_memory, slist_element);

      LOG_OUT();
      return object;
    }
  else
    FAILURE("empty slist");
}

void *
tbx_slist_extract_tail(p_tbx_slist_t slist)
{
  LOG_IN();
  if (slist->length)
    {
      p_tbx_slist_element_t  slist_element = slist->tail;
      void                  *object        = slist_element->object;

      if (--(slist->length))
	{
	  slist->tail = slist_element->previous;
	}
      else
	{
	  slist->head = NULL;
	  slist->tail = NULL;
	}

      tbx_free(tbx_slist_manager_memory, slist_element);
  
      LOG_OUT();
      return object;
    }
  else
    FAILURE("empty slist");
}

void
tbx_slist_add_after(p_tbx_slist_reference_t  ref,
		    void                    *object)
{
  LOG_IN();
  if (ref->reference)
    {
      p_tbx_slist_t         slist         = ref->slist;
      p_tbx_slist_element_t slist_element = NULL;
      
      slist_element = tbx_malloc(tbx_slist_manager_memory);
      slist_element->object          = object;
      slist_element->previous        = ref->reference;
      slist_element->next            = ref->reference->next;
      ref->reference->next->previous = slist_element;
      ref->reference->next           = slist_element;

      if (slist->tail == ref->reference)
	{
	  slist->tail = slist_element;
	}
      
      slist->length++;
    }
  else
    FAILURE("undefined slist reference");

  LOG_OUT();
}

void
tbx_slist_add_before(p_tbx_slist_reference_t  ref,
		     void                    *object)
{
  LOG_IN();
  if (ref->reference)
    {
      p_tbx_slist_t         slist         = ref->slist;
      p_tbx_slist_element_t slist_element = NULL;
  
      slist_element = tbx_malloc(tbx_slist_manager_memory);
      slist_element->object          = object;
      slist_element->next            = ref->reference;
      slist_element->previous        = ref->reference->previous;
      ref->reference->previous->next = slist_element;
      ref->reference->previous       = slist_element;

      if (slist->head == ref->reference)
	{
	  slist->head = slist_element;
	}

      slist->length++;
    }
  LOG_OUT();
}

void *
tbx_slist_remove(p_tbx_slist_reference_t ref)
{
  LOG_IN();
  if (ref->reference)
    {
      p_tbx_slist_t          slist         = ref->slist;
      p_tbx_slist_element_t  slist_element = ref->reference;
      void                  *object        = slist_element->object;

      if (--(slist->length))
	{
	  if (!slist_element->next)
	    {
	      slist_element->previous->next = NULL;
	      slist->tail                   = slist_element->previous;
	      ref->reference                = slist->tail;
	    }
	  else if (!slist_element->previous)
	    {
	      slist_element->next->previous = NULL;
	      slist->head                   = slist_element->next;
	      ref->reference                = slist->head;
	    }
	  else
	    {
	      slist_element->previous->next = slist_element->next;
	      slist_element->next->previous = slist_element->previous;

	      /* Warning: asymetric behaviour here */
	      ref->reference                = ref->reference->previous;
	    }
	}
      else
	{
	  slist->head    = NULL;
	  slist->tail    = NULL;
	  ref->reference = NULL;
	}

      tbx_free(tbx_slist_manager_memory, slist_element);

      return object;
    }
  else
    FAILURE("undefined slist reference");

  LOG_OUT();
}

void *
tbx_slist_get(p_tbx_slist_reference_t ref)
{
  LOG_IN();
  if (ref->reference)
    {
      LOG_OUT();
      return ref->reference->object;
    }
  else
    FAILURE("undefined slist reference");
}

/* ------------- */
static void
tbx_slist_elt_append_head(p_tbx_slist_t         slist,
			  p_tbx_slist_element_t slist_element)
{
  LOG_IN();
  if (slist->length)
    {
      slist_element->next     = slist->head;
      slist_element->previous = NULL;
      slist->head->previous   = slist_element;
    }
  else
    {
      slist_element->next     = NULL;
      slist_element->previous = NULL;
      slist->head             = slist_element;
      slist->tail             = slist_element;
    }
  slist->length++;
  LOG_OUT();
}


static void
tbx_slist_elt_append_tail(p_tbx_slist_t         slist,
			  p_tbx_slist_element_t slist_element)
{
  LOG_IN();
  if (slist->length)
    {
      slist_element->next     = NULL;
      slist_element->previous = slist->tail;
      slist->tail->next       = slist_element;
    }
  else
    {
      slist_element->next     = NULL;
      slist_element->previous = NULL;
      slist->head             = slist_element;
      slist->tail             = slist_element;
    }
  slist->length++;
  LOG_OUT();
}

static p_tbx_slist_element_t
tbx_slist_elt_extract_head(p_tbx_slist_t slist)
{
  LOG_IN();
  if (slist->length)
    {
      p_tbx_slist_element_t  slist_element = slist->head;

      if (--(slist->length))
	{
	  slist->head = slist_element->next;
	}
      else
	{
	  slist->head = NULL;
	  slist->tail = NULL;
	}

      slist_element->previous = NULL;
      slist_element->next     = NULL;

      LOG_OUT();
      return slist_element;
    }
  else
    FAILURE("empty slist");
}

static p_tbx_slist_element_t
tbx_slist_elt_extract_tail(p_tbx_slist_t slist)
{
  LOG_IN();
  if (slist->length)
    {
      p_tbx_slist_element_t slist_element = slist->tail;

      if (--(slist->length))
	{
	  slist->tail = slist_element->previous;
	}
      else
	{
	  slist->head = NULL;
	  slist->tail = NULL;
	}

      slist_element->previous = NULL;
      slist_element->next     = NULL;
  
      LOG_OUT();
      return slist_element;
    }
  else
    FAILURE("empty slist");
}

static void
tbx_slist_elt_add_after(p_tbx_slist_reference_t ref,
			p_tbx_slist_element_t   slist_element)
{
  LOG_IN();
  if (ref->reference)
    {
      p_tbx_slist_t slist = ref->slist;

      slist_element->previous        = ref->reference;
      slist_element->next            = ref->reference->next;
      ref->reference->next->previous = slist_element;
      ref->reference->next           = slist_element;

      if (slist->tail == ref->reference)
	{
	  slist->tail = slist_element;
	}
      
      slist->length++;
    }
  else
    FAILURE("undefined slist reference");

  LOG_OUT();
}

static void
tbx_slist_elt_add_before(p_tbx_slist_reference_t ref,
			 p_tbx_slist_element_t   slist_element)
{
  LOG_IN();
  if (ref->reference)
    {
      p_tbx_slist_t         slist         = ref->slist;
      p_tbx_slist_element_t slist_element = NULL;
  
      slist_element->next            = ref->reference;
      slist_element->previous        = ref->reference->previous;
      ref->reference->previous->next = slist_element;
      ref->reference->previous       = slist_element;

      if (slist->head == ref->reference)
	{
	  slist->head = slist_element;
	}

      slist->length++;
    }
  LOG_OUT();
}

static p_tbx_slist_element_t
tbx_slist_elt_remove(p_tbx_slist_reference_t ref)
{
  LOG_IN();
  if (ref->reference)
    {
      p_tbx_slist_t         slist         = ref->slist;
      p_tbx_slist_element_t slist_element = ref->reference;

      if (--(slist->length))
	{
	  if (!slist_element->next)
	    {
	      slist_element->previous->next = NULL;
	      slist->tail                   = slist_element->previous;
	      ref->reference                = slist->tail;
	    }
	  else if (!slist_element->previous)
	    {
	      slist_element->next->previous = NULL;
	      slist->head                   = slist_element->next;
	      ref->reference                = slist->head;
	    }
	  else
	    {
	      slist_element->previous->next = slist_element->next;
	      slist_element->next->previous = slist_element->previous;

	      /* Warning: asymetric behaviour here */
	      ref->reference                = ref->reference->previous;
	    }
	}
      else
	{
	  slist->head    = NULL;
	  slist->tail    = NULL;
	  ref->reference = NULL;
	}

      slist_element->previous = NULL;
      slist_element->next     = NULL;

      LOG_OUT();
      return slist_element;
    }
  else
    FAILURE("undefined slist reference");
}

/* ------------- */

tbx_bool_t
tbx_slist_search(p_tbx_slist_search_func_t  sfunc,
		 void                      *ref_obj,
		 p_tbx_slist_reference_t    ref)
{
  p_tbx_slist_t         slist = ref->slist;
  p_tbx_slist_element_t slist_element = slist->head;

  LOG_IN();
  if (sfunc)
    {
      while (slist_element)
	{
	  if (sfunc(ref_obj, slist_element->object))
	    {
	      ref->reference = slist_element;
	      return tbx_true;
	    }

	  slist_element = slist_element->next;
	}      
    }
  else
    {
      while (slist_element)
	{
	  if (ref_obj == slist_element->object)
	    {
	      ref->reference = slist_element;
	      return tbx_true;
	    }

	  slist_element = slist_element->next;
	}      
    }

  ref->reference = NULL;

  LOG_OUT();
  return tbx_false;
}

void
tbx_slist_ref_init_head(p_tbx_slist_t           slist,
			p_tbx_slist_reference_t ref)
{
  LOG_IN();
  ref->slist     = slist;
  ref->reference = slist->head;
  LOG_OUT();
}

void
tbx_slist_ref_init_tail(p_tbx_slist_t           slist,
			p_tbx_slist_reference_t ref)
{
  LOG_IN();
  ref->slist     = slist;
  ref->reference = slist->tail;
  LOG_OUT();
}

tbx_bool_t
tbx_slist_ref_fwd(p_tbx_slist_reference_t ref)
{
  LOG_IN();
  if (ref->reference)
    {
      LOG_OUT();
      return (ref->reference = ref->reference->next) != NULL;
    }
  else
    {
      LOG_OUT();
      return tbx_false;
    }
}

tbx_bool_t
tbx_slist_ref_rew(p_tbx_slist_reference_t ref)
{
  LOG_IN();
  if (ref->reference)
    {
      LOG_OUT();
      return (ref->reference = ref->reference->previous) != NULL;
    }
  else
    {
      LOG_OUT();
      return tbx_false;
    }
}

void
tbx_slist_ref_to_head(p_tbx_slist_reference_t ref)
{
  LOG_IN();
  ref->reference = ref->slist->head;
  LOG_OUT();
}

void
tbx_slist_ref_to_tail(p_tbx_slist_reference_t ref)
{
  LOG_IN();
  ref->reference = ref->slist->tail;
  LOG_OUT();
}

tbx_bool_t
tbx_slist_ref_defined(p_tbx_slist_reference_t ref)
{
  LOG_IN();
  LOG_OUT();
  return (ref->reference) != NULL;
}

void
tbx_slist_sort(p_tbx_slist_t          slist,
	       p_tbx_slist_cmp_func_t cmp)
{
  if (slist->length > 1)
    {
      tbx_slist_t            slist1;
      tbx_slist_t            slist2;
      void                  *object = NULL;
      tbx_slist_reference_t  ref;
      
      tbx_slist_ref_init_head(slist, &ref);

      tbx_slist_init(&slist1);
      tbx_slist_init(&slist2);
	  
      object = tbx_slist_get(&ref);

      while(tbx_slist_ref_fwd(&ref))
	{
	  p_tbx_slist_element_t elt = NULL;

	  elt = tbx_slist_elt_remove(&ref);
	  
	  if (cmp(object, elt->object) == -1)
	    {
	      tbx_slist_elt_append_head(&slist1, elt);
	    }
	  else
	    {
	      tbx_slist_elt_append_head(&slist2, elt);
	    }
	}

      tbx_slist_sort(&slist1, cmp);
      tbx_slist_sort(&slist2, cmp);

      slist->head->previous = slist1.tail;
      slist->head->next     = slist2.head;

      if (slist1.tail)
	{
	  slist1.tail->next = slist->head;
	}

      if (slist2.tail)
	{
	  slist2.tail->previous = slist->head;
	}

      slist->head = slist1.head;
      slist->tail = slist2.tail;
    }
}

void
tbx_slist_dup(p_tbx_slist_t dest,
	      p_tbx_slist_t source)
{
  tbx_slist_init(dest);

  if (!tbx_slist_empty(source))
    {
      tbx_slist_reference_t ref;      

      tbx_slist_ref_init_head(source, &ref);

      do
	{
	  tbx_slist_append_tail(dest, tbx_slist_get(&ref));
	}
      while(tbx_slist_ref_fwd(&ref));
    }
}

