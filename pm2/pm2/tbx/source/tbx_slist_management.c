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
Revision 1.7  2000/09/05 13:59:40  oaumage
- reecriture des slists et corrections diverses au niveau des htables

Revision 1.6  2000/06/08 13:57:24  oaumage
- Corrections diverses

Revision 1.5  2000/06/07 08:58:38  oaumage
- Suppression de Warnings

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
 * :::::::::::::::::::::://///////////////////////////////////////////////////
 */

#include "tbx.h"

/* 
 *  Macros
 * ___________________________________________________________________________
 */
#define INITIAL_SLIST_ELEMENT 256

/*
 * data structures
 * ___________________________________________________________________________
 */
static p_tbx_memory_t tbx_slist_element_manager_memory;
static p_tbx_memory_t tbx_slist_struct_manager_memory;

/*
 *  Functions
 * ___________________________________________________________________________
 */


/*
 * Initialization
 * --------------
 */
void
tbx_slist_manager_init(void)
{
  LOG_IN();
  tbx_malloc_init(&tbx_slist_element_manager_memory,
		  sizeof(tbx_slist_element_t),
		  INITIAL_SLIST_ELEMENT);
  tbx_malloc_init(&tbx_slist_struct_manager_memory,
		  sizeof(tbx_slist_t),
		  INITIAL_SLIST_ELEMENT);
  LOG_OUT();
}

void
tbx_slist_manager_exit(void)
{
  LOG_IN();
  tbx_malloc_clean(tbx_slist_element_manager_memory);
  tbx_malloc_clean(tbx_slist_struct_manager_memory);
  LOG_OUT();
}

/*
 * Return a nil list
 * -----------------
 */
p_tbx_slist_t
tbx_slist_nil(void)
{
  p_tbx_slist_t slist = NULL;

  LOG_IN();
  slist = tbx_malloc(tbx_slist_struct_manager_memory);
  slist->length = 0;
  slist->head   = NULL;
  slist->tail   = NULL;
  slist->ref    = NULL;
  LOG_OUT();

  return slist;
}

static
void
__tbx_slist_free_struct(p_tbx_slist_t slist)
{
  LOG_IN();
  slist->length = 0;
  slist->head   = NULL;
  slist->tail   = NULL;
  slist->ref    = NULL;
  tbx_free(tbx_slist_struct_manager_memory, slist);
  LOG_OUT();
}

/*
 * Return a new list element
 * -------------------------
 */
p_tbx_slist_element_t
tbx_slist_alloc_element(void *object)
{
  p_tbx_slist_element_t slist_element = NULL;

  LOG_IN();
  slist_element = tbx_malloc(tbx_slist_element_manager_memory);
  slist_element->previous = NULL;
  slist_element->next     = NULL;
  slist_element->object   = object;
  LOG_OUT();

  return slist_element;
}

/*
 * Free a list element
 * -------------------------
 */
static
void *
__tbx_slist_free_element(p_tbx_slist_t         slist,
			 p_tbx_slist_element_t element)
{
  void *object = NULL;

  LOG_IN();
  object = element->object;
  
  if (!element->previous)
    {
      slist->head = element->next;
    }

  if (!element->next)
    {
      slist->tail = element->previous;
    }
      
  if (element->previous)
    {
      element->previous->next = element->next;
    }
  
  if (element->next)
    {
      element->next->previous = element->previous;
    }

  if (slist->ref == element)
    {
      slist->ref = NULL;
    }

  element->previous = NULL;
  element->next     = NULL;
  element->object   = NULL;

  tbx_free(tbx_slist_element_manager_memory, element);

  slist->length--;
  LOG_OUT();
  return object;
}

/*
 * Length
 * ------
 */
tbx_slist_length_t
tbx_slist_get_length(p_tbx_slist_t slist)
{
  tbx_slist_length_t length = -1;
  
  LOG_IN();
  length = slist->length;
  LOG_OUT();

  return length;
}


/*
 * Test for nil
 * ------------
 */
tbx_bool_t
tbx_slist_is_nil(p_tbx_slist_t slist)
{
  tbx_bool_t test = tbx_false;
  
  LOG_IN();
  test = (slist->length == 0);
  LOG_OUT();

  return test;
}

/*
 * Duplicate a list
 * ----------------
 */
p_tbx_slist_t
tbx_slist_dup(p_tbx_slist_t source)
{
  p_tbx_slist_t destination = NULL;

  LOG_IN();
  destination = tbx_slist_nil();

  if (!tbx_slist_is_nil(source))
    {
      p_tbx_slist_element_t ptr;

      ptr = source->head;
      
      destination->head = tbx_slist_alloc_element(ptr->object);
      destination->tail = destination->head;

      ptr = ptr->next;

      while (ptr)
	{
	  destination->tail->next = tbx_slist_alloc_element(ptr->object);
	  destination->tail->next->previous = destination->tail;	  
	  destination->tail = destination->tail->next;
	  ptr = ptr->next;
	}

      destination->length = source->length;
    }  

  LOG_OUT();
  return destination;
}

void
tbx_slist_add_at_head(p_tbx_slist_t  slist,
		      void          *object)
{
  LOG_IN();
  if (tbx_slist_is_nil(slist))
    {
      slist->head = tbx_slist_alloc_element(object);
      slist->tail = slist->head;
    }
  else
    {
      slist->head->previous = tbx_slist_alloc_element(object);
      slist->head->previous->next = slist->head;
      slist->head = slist->head->previous;
    }

  slist->length++;  
  LOG_OUT();
}

void
tbx_slist_add_at_tail(p_tbx_slist_t  slist,
		      void          *object)
{
  LOG_IN();
  if (tbx_slist_is_nil(slist))
    {
      slist->tail = tbx_slist_alloc_element(object);
      slist->head = slist->tail;
    }
  else
    {
      slist->tail->next = tbx_slist_alloc_element(object);
      slist->tail->next->previous = slist->tail;
      slist->tail = slist->tail->next;
    }  

  slist->length++;
  LOG_OUT();
}

void *
tbx_slist_remove_from_head(p_tbx_slist_t slist)
{
  LOG_IN();
  if (!tbx_slist_is_nil(slist))
    {
      p_tbx_slist_element_t  element = slist->head;
      void                  *object  = NULL;
      
      object = __tbx_slist_free_element(slist, element);
      
      LOG_OUT();
      return object;
    }
  else
    FAILURE("empty slist");
}

void *
tbx_slist_remove_from_tail(p_tbx_slist_t slist)
{
  LOG_IN();
  if (!tbx_slist_is_nil(slist))
    {
      p_tbx_slist_element_t  element = slist->tail;
      void                  *object  = NULL;
      
      object = __tbx_slist_free_element(slist, element);
      
      LOG_OUT();
      return object;
    }
  else
    FAILURE("empty slist");
}


/*
 * Queue access functions
 * ----------------------
 */

/* Add an element at the head of the list */
void
tbx_slist_enqueue(p_tbx_slist_t  slist,
		  void          *object)
{
  LOG_IN();
  tbx_slist_add_at_head(slist, object);
  LOG_OUT();
}

/* Remove the element at the tail of the list */
void *
tbx_slist_dequeue(p_tbx_slist_t slist)
{
  void *object;
  
  LOG_IN();
  object = tbx_slist_remove_from_tail(slist);
  LOG_OUT();

  return object;
}


/*
 * Stack access functions
 * ----------------------
 */

/* Add an element at the tail of the list */
void
tbx_slist_push(p_tbx_slist_t  slist,
	       void          *object)
{
  LOG_IN();
  tbx_slist_add_at_tail(slist, object);
  LOG_OUT();  
}

/* Remove the element at the tail of the list */
void *
tbx_slist_pop(p_tbx_slist_t slist)
{
  void *object;
  
  LOG_IN();
  object = tbx_slist_remove_from_tail(slist);
  LOG_OUT();

  return object;
}


/* 
 * File access functions
 * ---------------------
 */

/* Add an element at the tail of the list */
void
tbx_slist_append(p_tbx_slist_t slist, void *object)
{
  LOG_IN();
  tbx_slist_add_at_tail(slist, object);
  LOG_OUT();  
}

/* Remove an element at the head of the list */
void *
tbx_slist_extract(p_tbx_slist_t slist)
{
  void *object;
  
  LOG_IN();
  object = tbx_slist_remove_from_head(slist);
  LOG_OUT();

  return object;  
}

/*
 * Concatenation functions 
 * -----------------------
 */
p_tbx_slist_t
tbx_slist_cons(void          *object,
	       p_tbx_slist_t  source)
{
  p_tbx_slist_t destination = NULL;

  LOG_IN();
  destination = tbx_slist_dup(source);
  tbx_slist_add_at_head(destination, object);
  LOG_OUT();

  return destination;
}

static
void
__tbx_slist_merge_before_and_free(p_tbx_slist_t destination,
				p_tbx_slist_t source)
{
  LOG_IN();
  if (destination->head)
    {
      destination->head->previous = source->tail;
    }

  if (source->tail)
    {
      source->tail->next = destination->head;
    }

  destination->head = source->head;
  destination->length += source->length;

  __tbx_slist_free_struct(source);
  LOG_OUT();
}

static
void
__tbx_slist_merge_after_and_free(p_tbx_slist_t destination,
			       p_tbx_slist_t source)
{
  LOG_IN();
  if (source->head)
    {
      source->head->previous = destination->tail;
    }

  if (destination->tail)
    {
      destination->tail->next = source->head;
    }

  destination->tail = source->tail;
  destination->length += source->length;

  __tbx_slist_free_struct(source);
  LOG_OUT();
}

void
tbx_slist_merge_before(p_tbx_slist_t destination,
		       p_tbx_slist_t source)
{
  LOG_IN();
  source = tbx_slist_dup(source);
  __tbx_slist_merge_before_and_free(destination, source);
  LOG_OUT();
}

void
tbx_slist_merge_after(p_tbx_slist_t destination,
		      p_tbx_slist_t source)
{
  LOG_IN();
  source = tbx_slist_dup(source);
  __tbx_slist_merge_after_and_free(destination, source);
  LOG_OUT();
}

p_tbx_slist_t
tbx_slist_merge(p_tbx_slist_t destination,
		p_tbx_slist_t source)
{
  LOG_IN();
  destination = tbx_slist_dup(destination);
  tbx_slist_merge_after(destination, source);
  LOG_OUT();

  return destination;
}

/*
 * List reversion functions
 * ------------------------
 */
void
tbx_slist_reverse_list(p_tbx_slist_t slist)
{
  p_tbx_slist_element_t  ptr = NULL;

  LOG_IN();
  ptr         = slist->head;
  slist->head = slist->tail;
  slist->tail = ptr;
  
  while (ptr)
    {
      void *temp = NULL;
      
      temp          = ptr->next;
      ptr->next     = ptr->previous;
      ptr->previous = temp;

      ptr = ptr->previous;
    }
  LOG_OUT();
}

p_tbx_slist_t
tbx_slist_reverse(p_tbx_slist_t source)
{
  p_tbx_slist_t destination = NULL;
  
  LOG_IN();
  destination = tbx_slist_dup(source);
  tbx_slist_reverse_list(destination);
  LOG_OUT();
  
  return destination;
}

/*
 * Index access functions
 * ----------------------
 */
void *
tbx_slist_index_get(p_tbx_slist_t     slist,
		    tbx_slist_index_t index)
{
  LOG_IN();

  if (!tbx_slist_is_nil(slist))
    {
      p_tbx_slist_element_t  element = NULL;
      
      element = slist->head;

      while (element)
	{
	  if (index)
	    {
	      index--;
	      element = element->next;
	    }
	  else
	    {
	      void *object  = NULL;

	      object = element->object;
	      LOG_OUT();

	      return object;
	    }
	}

      FAILURE("out-of-bound index");
    }
  else
    FAILURE("empty list");
}

static 
p_tbx_slist_element_t
__tbx_slist_index_get_element(p_tbx_slist_t     slist,
			      tbx_slist_index_t index)
{
  p_tbx_slist_element_t  element = NULL;

  LOG_IN();

  element = slist->head;
  while (element)
    {
      if (index)
	{
	  index--;
	  element = element->next;
	}
      else
	{
	  break;
	}
    }
  
  LOG_OUT();

  return element;
}

void *
tbx_slist_index_extract(p_tbx_slist_t     slist,
			tbx_slist_index_t index)
{
  p_tbx_slist_element_t  element = NULL;
  void                  *object  = NULL;

  LOG_IN();
  element = __tbx_slist_index_get_element(slist, index);
  
  if (element)
    {
      object = __tbx_slist_free_element(slist, element);
    }
  else
    FAILURE("out-of-bound index");
      
  LOG_OUT();

  return object;
}

void
tbx_slist_index_set_ref(p_tbx_slist_t     slist,
			tbx_slist_index_t index)
{
  LOG_IN();

  if (!tbx_slist_is_nil(slist))
    {
      p_tbx_slist_element_t element = NULL;
      
      element = slist->head;

      while (element)
	{
	  if (index)
	    {
	      index--;
	      element = element->next;
	    }
	  else
	    {
	      slist->ref = element;
	      LOG_OUT();
	    }
	}

      FAILURE("out-of-bound index");
    }
  else
    FAILURE("empty list");
}

/*
 * Search access functions
 * -----------------------
 */
tbx_slist_index_t
tbx_slist_search_get_index(p_tbx_slist_t              slist,
			   p_tbx_slist_search_func_t  sfunc,
			   void                      *object)
{
  tbx_slist_index_t index = 0;
  
  LOG_IN();
  if (!tbx_slist_is_nil(slist))
    {
      p_tbx_slist_element_t ptr = NULL;
      
      ptr = slist->head;
      
      if (sfunc)
	{
	  while (ptr)
	    {
	      if (sfunc(object, ptr->object))
		{
		  LOG_OUT();

		  return index;
		}

	      index++;
	      ptr = ptr->next;
	    } 
	}
      else
	{
	  while (ptr)
	    {
	      if (ptr->object == object)
		{
		  LOG_OUT();

		  return index;		  
		}

	      index++;
	      ptr = ptr->next;
	    } 
	}
    }
  else
    FAILURE("empty list");
  LOG_OUT();

  return -1;
}

tbx_bool_t
tbx_slist_search_forward_set_ref(p_tbx_slist_t              slist,
				 p_tbx_slist_search_func_t  sfunc,
				 void                      *object)
{
  LOG_IN();
  if (!tbx_slist_is_nil(slist))
    {
      slist->ref = slist->head;
      
      if (sfunc)
	{
	  while (slist->ref)
	    {
	      if (sfunc(object, slist->ref->object))
		{
		  LOG_OUT();

		  return tbx_true;
		}

	      slist->ref = slist->ref->next;
	    } 
	}
      else
	{
	  while (slist->ref)
	    {
	      if (slist->ref->object == object)
		{
		  LOG_OUT();

		  return tbx_true;  
		}

	      slist->ref = slist->ref->next;
	    } 
	}
    }
  else
    FAILURE("empty list");
  LOG_OUT();

  return tbx_false;
}


tbx_bool_t
tbx_slist_search_backward_set_ref(p_tbx_slist_t              slist,
				  p_tbx_slist_search_func_t  sfunc,
				  void                      *object)
{
  LOG_IN();
  if (!tbx_slist_is_nil(slist))
    {
      slist->ref = slist->tail;
      
      if (sfunc)
	{
	  while (slist->ref)
	    {
	      if (sfunc(object, slist->ref->object))
		{
		  LOG_OUT();

		  return tbx_true;
		}

	      slist->ref = slist->ref->previous;
	    } 
	}
      else
	{
	  while (slist->ref)
	    {
	      if (slist->ref->object == object)
		{
		  LOG_OUT();

		  return tbx_true;  
		}

	      slist->ref = slist->ref->previous;
	    } 
	}
    }
  else
    FAILURE("empty list");
  LOG_OUT();

  return tbx_false;
}

tbx_bool_t
tbx_slist_search_next_set_ref(p_tbx_slist_t              slist,
			      p_tbx_slist_search_func_t  sfunc,
			      void                      *object)
{
  LOG_IN();
  if (slist->ref)
    {
      if (!tbx_slist_is_nil(slist))
	{
	  if (sfunc)
	    {
	      while (slist->ref)
		{
		  if (sfunc(object, slist->ref->object))
		    {
		      LOG_OUT();

		      return tbx_true;
		    }

		  slist->ref = slist->ref->next;
		} 
	    }
	  else
	    {
	      while (slist->ref)
		{
		  if (slist->ref->object == object)
		    {
		      LOG_OUT();

		      return tbx_true;  
		    }

		  slist->ref = slist->ref->next;
		} 
	    }
	}
      else
	FAILURE("empty list");
    }
  else
    FAILURE("uninitialized reference");
  LOG_OUT();

  return tbx_false;
}


tbx_bool_t
tbx_slist_search_previous_set_ref(p_tbx_slist_t              slist,
				  p_tbx_slist_search_func_t  sfunc,
				  void                      *object)
{
  LOG_IN();
  if (slist->ref)
    {
      if (!tbx_slist_is_nil(slist))
	{
	  if (sfunc)
	    {
	      while (slist->ref)
		{
		  if (sfunc(object, slist->ref->object))
		    {
		      LOG_OUT();

		      return tbx_true;
		    }

		  slist->ref = slist->ref->previous;
		} 
	    }
	  else
	    {
	      while (slist->ref)
		{
		  if (slist->ref->object == object)
		    {
		      LOG_OUT();

		      return tbx_true;  
		    }

		  slist->ref = slist->ref->previous;
		} 
	    }
	}
      else
	FAILURE("empty list");
    }
  else
    FAILURE("uninitialized reference");
  LOG_OUT();

  return tbx_false;
}

/*
 * Reference access functions
 * --------------------------
 */

tbx_slist_index_t
tbx_slist_ref_get_index(p_tbx_slist_t slist)
{
  tbx_slist_index_t index = -1;

  LOG_IN();
  if (slist->ref)
    {
      p_tbx_slist_element_t element = NULL;

      element = slist->ref;

      do
	{
	  index++;
	  element = element->previous;
	}
      while (element);
    }
  LOG_OUT();

  return index;
}

void
tbx_slist_ref_to_head(p_tbx_slist_t slist)
{
  LOG_IN();
  if (!tbx_slist_is_nil(slist))
    {
      slist->ref = slist->head;
    }
  else
    FAILURE("empty list");
  LOG_OUT();
}

void
tbx_slist_ref_to_tail(p_tbx_slist_t slist)
{
  LOG_IN();
  if (!tbx_slist_is_nil(slist))
    {
      slist->ref = slist->tail;
    }
  else
    FAILURE("empty list");
  LOG_OUT();
}

tbx_bool_t
tbx_slist_ref_forward(p_tbx_slist_t slist)
{
  tbx_bool_t result = tbx_false;

  LOG_IN();
  if (slist->ref)
    {
      if (!tbx_slist_is_nil(slist))
	{
	  slist->ref = slist->ref->next;
	  result = (slist->ref != NULL);
	}
      else
	FAILURE("empty list");
    }
  else
    FAILURE("uninitialized reference");
  LOG_OUT();

  return result;
}

tbx_bool_t
tbx_slist_ref_backward(p_tbx_slist_t slist)
{
  tbx_bool_t result = tbx_false;

  LOG_IN();
  if (slist->ref)
    {
      if (!tbx_slist_is_nil(slist))
	{
	  slist->ref = slist->ref->previous;
	  result = (slist->ref != NULL);
	}
      else
	FAILURE("empty list");
    }
  else
    FAILURE("uninitialized reference");
  LOG_OUT();

  return result;
}

tbx_bool_t
tbx_slist_ref_step_forward(p_tbx_slist_t        slist,
			   p_tbx_slist_offset_t offset)
{
  tbx_bool_t result = tbx_false;

  LOG_IN();
  if (slist->ref)
    {
      if (!tbx_slist_is_nil(slist))
	{
	  while (offset && slist->ref)
	    {
	      offset--;
	      slist->ref = slist->ref->next;
	    }
	  
	  result = (slist->ref != NULL);
	}
      else
	FAILURE("empty list");
    }
  else
    FAILURE("uninitialized reference");
  LOG_OUT();

  return result;
}

tbx_bool_t
tbx_slist_ref_step_backward(p_tbx_slist_t        slist,
			    p_tbx_slist_offset_t offset)
{
  tbx_bool_t result = tbx_false;

  LOG_IN();
  if (slist->ref)
    {
      if (!tbx_slist_is_nil(slist))
	{
	  while (offset && slist->ref)
	    {
	      offset--;
	      slist->ref = slist->ref->previous;
	    }
	  
	  result = (slist->ref != NULL);
	}
      else
	FAILURE("empty list");
    }
  else
    FAILURE("uninitialized reference");
  LOG_OUT();

  return result;
}
