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
$Log: tbx_list_management.c,v $
Revision 1.4  2000/05/22 13:45:55  oaumage
- correction de bugs divers

Revision 1.3  2000/05/22 12:20:10  oaumage
- Listes de recherche

Revision 1.2  2000/03/01 11:03:46  oaumage
- mise a jour des #includes ("")

Revision 1.1  2000/01/13 14:51:33  oaumage
Inclusion de la toolbox dans le repository

______________________________________________________________________________
*/

/*
 * tbx_list_management.c
 * ---------------------
 */

#include "tbx.h"

/* MACROS */
#define INITIAL_LIST_ELEMENT 256

/* data structures */
static p_tbx_memory_t tbx_list_manager_memory;

/* functions */

/*
 * Initialization
 * --------------
 */
void tbx_list_manager_init()
{
  tbx_malloc_init(&tbx_list_manager_memory,
		  sizeof(tbx_list_element_t),
		  INITIAL_LIST_ELEMENT);
}

void tbx_list_init(p_tbx_list_t list)
{
  list->length = 0;
  list->read_only = tbx_false;
  list->mark_next = tbx_true; /* mark is always set
				 on the first element */
}

/*
 * Destruction
 * --------------
 */
void tbx_list_manager_exit()
{
  tbx_malloc_clean(tbx_list_manager_memory);
}

void tbx_destroy_list(p_tbx_list_t list)
{
  if (list->length)
    {
      p_tbx_list_element_t list_element ;
      
      list_element = list->first ;

      while (list_element != list->last)
	{
	  p_tbx_list_element_t temp_list_element = list_element->next;
	  
	  tbx_free(tbx_list_manager_memory, list_element);
	  list_element = temp_list_element;
	}

      tbx_free(tbx_list_manager_memory, list->last);

      list->length = 0 ;
    }
}

void tbx_foreach_destroy_list(p_tbx_list_t                list,
			      p_tbx_list_foreach_func_t   func)
{
  if (list->length)
    {
      p_tbx_list_element_t list_element ;
      
      list_element = list->first ;

      while (list_element != list->last)
	{
	  p_tbx_list_element_t temp_list_element = list_element->next;

	  func(list_element->object);
	  tbx_free(tbx_list_manager_memory, list_element);
	  list_element = temp_list_element;
	}

      func(list_element->object);
      tbx_free(tbx_list_manager_memory, list->last);

      list->length = 0 ;
    }
}

/*
 * Put/Get
 * -------
 */
void tbx_append_list(p_tbx_list_t   list,
		     void          *object)
{
  p_tbx_list_element_t list_element ;

  if (list->read_only)
    {
      FAILURE("attempted to modify a read only list");
    }
  
  list_element = tbx_malloc(tbx_list_manager_memory);

  list_element->object = object;
  list_element->next = NULL ;
  
  if (list->length)
    {
      list_element->previous = list->last ;
      list_element->previous->next = list_element ;
    }
  else
    {
      /* This is the first list element */

      list->first = list_element ;
      list_element->previous = NULL ;
    }

  list->last = list_element ;
  list->length++;

  if (list->mark_next)
    {
      list->mark = list_element ;
      list->mark_position = list->length - 1;
      list->mark_next = tbx_false ;
    }
}

void *tbx_get_list_object(p_tbx_list_t list)
{
  if (list->length)
    {
      return list->last->object;
    }
  else
    {
      FAILURE("empty list");
    }
}


/*
 * list mark control function
 * --------------------------
 */

void tbx_mark_list(p_tbx_list_t list)
{
  /* start a new sub list -> the next element
     added to the list will be the first element
     of the new sub list */
  list->mark_next = tbx_true;
}

tbx_bool_t tbx_empty_list(p_tbx_list_t list)
{
  if (list->length)
    {
      return tbx_false ;
    }
  else
    {
      return tbx_true ;
    }
}

/*
 * sub-list extraction
 * -------------------
 */

void tbx_duplicate_list(p_tbx_list_t   source,
			p_tbx_list_t   destination)
{
  *destination = *source ;
  destination->mark = destination->first ;
  destination->mark_position = 0;  
  destination->read_only = tbx_true;
}

void tbx_extract_sub_list(p_tbx_list_t   source,
			  p_tbx_list_t   destination)
{
  if (source->length)
    {
      destination->first = source->mark ;
      destination->last = source->last ;

      destination->length = source->length - source->mark_position;

      destination->mark = destination->first ;
      destination->mark_position = 0;
      destination->read_only = tbx_true;
    }
  else
    {
      FAILURE("empty list");
    }
}

/*
 * list reference functions
 * ------------------------
 */

void tbx_list_reference_init(p_tbx_list_reference_t   ref,
			     p_tbx_list_t             list)
{
  ref->list = list;

  if (list->length)
    {
      ref->reference = ref->list->first;
      ref->after_end = tbx_false;
    }
  else
    {
      ref->reference = NULL;
      ref->after_end = tbx_true;
    }
}

void *tbx_get_list_reference_object(p_tbx_list_reference_t ref)
{
  if (ref->reference == NULL)
    {
      if (ref->list->length)
	{
	  ref->reference = ref->list->first;
	  ref->after_end = tbx_false;
	}
    }

  if (ref->list->length)
    {
      if (ref->reference)
	{
	  if (ref->after_end)
	    {
	      if (ref->reference != ref->list->last)
		{
		  ref->reference = ref->reference->next;
		  ref->after_end = tbx_false;
		  return ref->reference->object;
		}
	      else
		{
		  /* reference passed the end of the list */
		  return NULL;
		}
	    }
	  else
	    { 
	      return ref->reference->object;
	    }
	}
      else
	{
	  /* the referenced list was empty when
	     the reference was initialized */
	  
	  ref->reference = ref->list->first;
	  ref->after_end = tbx_false;

	  return ref->reference->object;
	}
    }
  else
    {
      FAILURE("empty list");      
    }
}

/*
 * Cursor control function
 * -----------------------
 */
tbx_bool_t tbx_forward_list_reference(p_tbx_list_reference_t ref)
{
  if (ref->reference == NULL)
    {
      if (ref->list->length)
	{
	  ref->reference = ref->list->first;
	  ref->after_end = tbx_false;
	}
    }

  if (ref->list->length)
    {
      if (ref->after_end)
	{
	  if (ref->reference != ref->list->last)
	    {
	      ref->reference = ref->reference->next;
	      ref->after_end = tbx_false;
	      return tbx_true ;
	    }
	  else
	    {
	      return tbx_false;
	    }
	}
      else
	{
	  if (ref->reference != ref->list->last)
	    {
	      ref->reference = ref->reference->next ;
	      return tbx_true ;
	    }
	  else
	    {
	      ref->after_end = tbx_true ;
	      
	      return tbx_false ;
	    }
	}
    }
  else
    {
      FAILURE("empty list");
    }
}

void tbx_reset_list_reference(p_tbx_list_reference_t ref)
{
  if (ref->list->length)
    {
      ref->reference = ref->list->first;
      ref->after_end = tbx_false ;
    }
  else
    {
      ref->reference = NULL;
      ref->after_end = tbx_true;      
    }
}

tbx_bool_t tbx_reference_after_end_of_list(p_tbx_list_reference_t ref)
{
  LOG_IN();
  if (ref->reference == NULL)
    {
      LOG("tbx_reference_after_end_of_list: list still empty ?");
      if (ref->list->length)
	{
	  LOG("tbx_reference_after_end_of_list: list still empty - yes");
	  ref->reference = ref->list->first;
	  ref->after_end = tbx_false;
	}
    }

  if (ref->list->length)
    {
      if (ref->after_end)
	{
	  if (ref->reference != ref->list->last)
	    {
	      LOG("tbx_reference_after_end_of_list: list extended");
	      ref->reference = ref->reference->next;
	      ref->after_end = tbx_false;
	    }
	  LOG_OUT();
	  return ref->after_end;
	}
      else
	{
	  LOG_OUT();
	  return tbx_false;
	}
    }
  else
    {
      LOG_OUT();
      return tbx_true;
    }
}

