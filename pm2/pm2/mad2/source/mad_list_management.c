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
$Log: mad_list_management.c,v $
Revision 1.4  2000/01/05 15:51:26  oaumage
- mad_list_management.c: changement de `len' en `length'
- mad_channel.c: correction au niveau de l'appel a mad_link_exit

Revision 1.3  2000/01/05 09:42:58  oaumage
- mad_communication.c: support du nouveau `group_mode' (a surveiller !)
- madeleine.c: external_spawn_init n'est plus indispensable
- mad_list_management: rien de particulier, mais l'ajout d'une fonction
  d'acces a la longueur de la liste est a l'etude, pour permettre aux drivers
  de faire une optimisation simple au niveau des groupes de buffers de
  taille 1

Revision 1.2  1999/12/15 17:31:27  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * mad_list_management.c
 * ---------------------
 */

#include <madeleine.h>

/* MACROS */
#define INITIAL_LIST_ELEMENT 256

/* data structures */
static p_mad_memory_t mad_list_manager_memory;

/* functions */

/*
 * Initialization
 * --------------
 */
void mad_list_manager_init()
{
  mad_malloc_init(&mad_list_manager_memory,
		  sizeof(mad_list_t),
		  INITIAL_LIST_ELEMENT
		  );
}

void mad_list_init(p_mad_list_t list)
{
  list->length = 0;
  list->read_only = mad_false;
  list->mark_next = mad_true; /* mark is always set
				 on the first element */
}

/*
 * Destruction
 * --------------
 */
void mad_list_manager_exit()
{
  mad_malloc_clean(mad_list_manager_memory);
}

void mad_destroy_list(p_mad_list_t list)
{
  if (list->length)
    {
      p_mad_list_element_t list_element ;
      
      list_element = list->first ;

      while (list_element != list->last)
	{
	  p_mad_list_element_t temp_list_element = list_element->next;
	  
	  mad_free(mad_list_manager_memory, list_element);
	  list_element = temp_list_element;
	}

      mad_free(mad_list_manager_memory, list->last);

      list->length = 0 ;
    }
}

void mad_foreach_destroy_list(p_mad_list_t                list,
			      p_mad_list_foreach_func_t   func)
{
  if (list->length)
    {
      p_mad_list_element_t list_element ;
      
      list_element = list->first ;

      while (list_element != list->last)
	{
	  p_mad_list_element_t temp_list_element = list_element->next;

	  func(list_element->object);
	  mad_free(mad_list_manager_memory, list_element);
	  list_element = temp_list_element;
	}

      func(list_element->object);
      mad_free(mad_list_manager_memory, list->last);

      list->length = 0 ;
    }
}

/*
 * Put/Get
 * -------
 */
void mad_append_list(p_mad_list_t   list,
		     void          *object)
{
  p_mad_list_element_t list_element ;

  if (list->read_only)
    {
      FAILURE("attempted to modify a read only list");
    }
  
  list_element = mad_malloc(mad_list_manager_memory);

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
      list->mark_next = mad_false ;
    }
}

void *mad_get_list_object(p_mad_list_t list)
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

void mad_mark_list(p_mad_list_t list)
{
  /* start a new sub list -> the next element
     added to the list will be the first element
     of the new sub list */
  list->mark_next = mad_true;
}

mad_bool_t mad_empty_list(p_mad_list_t list)
{
  if (list->length)
    {
      return mad_false ;
    }
  else
    {
      return mad_true ;
    }
}

/*
 * sub-list extraction
 * -------------------
 */

void mad_duplicate_list(p_mad_list_t   source,
			p_mad_list_t   destination)
{
  *destination = *source ;
  destination->mark = destination->first ;
  destination->mark_position = 0;  
  destination->read_only = mad_true;
}

void mad_extract_sub_list(p_mad_list_t   source,
			  p_mad_list_t   destination)
{
  if (source->length)
    {
      destination->first = source->mark ;
      destination->last = source->last ;

      destination->length = source->length - source->mark_position;

      destination->mark = destination->first ;
      destination->mark_position = 0;
      destination->read_only = mad_true;
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

void mad_list_reference_init(p_mad_list_reference_t   ref,
			     p_mad_list_t             list)
{
  ref->list = list;

  if (list->length)
    {
      ref->reference = ref->list->first;
      ref->after_end = mad_false;
    }
  else
    {
      ref->reference = NULL;
      ref->after_end = mad_true;
    }
}

void *mad_get_list_reference_object(p_mad_list_reference_t ref)
{
  if (ref->reference == NULL)
    {
      if (ref->list->length)
	{
	  ref->reference = ref->list->first;
	  ref->after_end = mad_false;
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
		  ref->after_end = mad_false;
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
	  ref->after_end = mad_false;

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
mad_bool_t mad_forward_list_reference(p_mad_list_reference_t ref)
{
  if (ref->reference == NULL)
    {
      if (ref->list->length)
	{
	  ref->reference = ref->list->first;
	  ref->after_end = mad_false;
	}
    }

  if (ref->list->length)
    {
      if (ref->after_end)
	{
	  if (ref->reference != ref->list->last)
	    {
	      ref->reference = ref->reference->next;
	      ref->after_end = mad_false;
	      return mad_true ;
	    }
	  else
	    {
	      return mad_false;
	    }
	}
      else
	{
	  if (ref->reference != ref->list->last)
	    {
	      ref->reference = ref->reference->next ;
	      return mad_true ;
	    }
	  else
	    {
	      ref->after_end = mad_true ;
	      
	      return mad_false ;
	    }
	}
    }
  else
    {
      FAILURE("empty list");
    }
}

void mad_reset_list_reference(p_mad_list_reference_t ref)
{
  if (ref->list->length)
    {
      ref->reference = ref->list->first;
      ref->after_end = mad_false ;
    }
  else
    {
      ref->reference = NULL;
      ref->after_end = mad_true;      
    }
}

mad_bool_t mad_reference_after_end_of_list(p_mad_list_reference_t ref)
{
  LOG_IN();
  if (ref->reference == NULL)
    {
      LOG("mad_reference_after_end_of_list: list still empty ?");
      if (ref->list->length)
	{
	  LOG("mad_reference_after_end_of_list: list still empty - yes");
	  ref->reference = ref->list->first;
	  ref->after_end = mad_false;
	}
    }

  if (ref->list->length)
    {
      if (ref->after_end)
	{
	  if (ref->reference != ref->list->last)
	    {
	      LOG("mad_reference_after_end_of_list: list extended");
	      ref->reference = ref->reference->next;
	      ref->after_end = mad_false;
	    }
	  LOG_OUT();
	  return ref->after_end;
	}
      else
	{
	  LOG_OUT();
	  return mad_false;
	}
    }
  else
    {
      LOG_OUT();
      return mad_true;
    }
}

