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
 * tbx_slist_management.c
 * :::::::::::::::::::::://///////////////////////////////////////////////////
 */

#include "tbx.h"

/* 
 *  Macros
 * ___________________________________________________________________________
 */
#define INITIAL_SLIST_ELEMENT 256
#define INITIAL_SLIST_NREF    64
#define INITIAL_SLIST_NUMBER 64

/*
 * data structures
 * ___________________________________________________________________________
 */
static p_tbx_memory_t tbx_slist_element_manager_memory = NULL;
static p_tbx_memory_t tbx_slist_nref_manager_memory    = NULL;
static p_tbx_memory_t tbx_slist_struct_manager_memory  = NULL;

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
  tbx_malloc_init(&tbx_slist_nref_manager_memory,
		  sizeof(tbx_slist_nref_t),
		  INITIAL_SLIST_NREF);
  tbx_malloc_init(&tbx_slist_struct_manager_memory,
		  sizeof(tbx_slist_t),
		  INITIAL_SLIST_NUMBER);
  LOG_OUT();
}

void
tbx_slist_manager_exit(void)
{
  LOG_IN();
  tbx_malloc_clean(tbx_slist_element_manager_memory);
  tbx_malloc_clean(tbx_slist_nref_manager_memory);
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
  TBX_INIT_SHARED(slist);
  slist->length    = 0;
  slist->head      = NULL;
  slist->tail      = NULL;
  slist->ref       = NULL;
  slist->nref_head = NULL;
  LOG_OUT();

  return slist;
}

static
void
__tbx_slist_free_struct(p_tbx_slist_t slist)
{
  LOG_IN();
  slist->length    = 0;
  slist->head      = NULL;
  slist->tail      = NULL;
  slist->ref       = NULL;
  slist->nref_head = NULL;
  tbx_free(tbx_slist_struct_manager_memory, slist);
  LOG_OUT();
}

void
tbx_slist_free(p_tbx_slist_t slist)
{
  LOG_IN();
  if (slist->length)
    FAILURE("slist not empty");
  if (slist->nref_head)
    FAILURE("nrefs still exist");
  __tbx_slist_free_struct(slist);
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

  if (slist->nref_head)
    {
      p_tbx_slist_nref_t nref = slist->nref_head;
      
      do
	{
	  if (nref->element == element)
	    {
	      nref->element = NULL;
	    }
	  
	  nref = nref->next;
	}
      while (nref);
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
		    tbx_slist_index_t idx)
{
  LOG_IN();

  if (!tbx_slist_is_nil(slist))
    {
      p_tbx_slist_element_t  element = NULL;
      
      element = slist->head;

      while (element)
	{
	  if (idx)
	    {
	      idx--;
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
			      tbx_slist_index_t idx)
{
  p_tbx_slist_element_t  element = NULL;

  LOG_IN();

  element = slist->head;
  while (element)
    {
      if (idx)
	{
	  idx--;
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
			tbx_slist_index_t idx)
{
  p_tbx_slist_element_t  element = NULL;
  void                  *object  = NULL;

  LOG_IN();
  element = __tbx_slist_index_get_element(slist, idx);
  
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
			tbx_slist_index_t idx)
{
  LOG_IN();

  if (!tbx_slist_is_nil(slist))
    {
      p_tbx_slist_element_t element = NULL;
      
      element = slist->head;

      while (element)
	{
	  if (idx)
	    {
	      idx--;
	      element = element->next;
	    }
	  else
	    {
	      slist->ref = element;
	      LOG_OUT();

	      return;
	    }
	}

      FAILURE("out-of-bound index");
    }
  else
    FAILURE("empty list");
}

void
tbx_slist_index_set_nref(p_tbx_slist_nref_t nref,
			 tbx_slist_index_t  idx)
{
  p_tbx_slist_t slist = NULL;

  LOG_IN();
  slist = nref->slist;

  if (!tbx_slist_is_nil(slist))
    {
      p_tbx_slist_element_t element = NULL;
      
      element = slist->head;

      while (element)
	{
	  if (idx)
	    {
	      idx--;
	      element = element->next;
	    }
	  else
	    {
	      nref->element = element;
	      LOG_OUT();

	      return;
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
  tbx_slist_index_t idx = 0;
  
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

		  return idx;
		}

	      idx++;
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

		  return idx;		  
		}

	      idx++;
	      ptr = ptr->next;
	    } 
	}
    }
  else
    FAILURE("empty list");
  LOG_OUT();

  return -1;
}

void *
tbx_slist_search_and_extract(p_tbx_slist_t              slist,
			     p_tbx_slist_search_func_t  sfunc,
			     void                      *object)
{
  void *result = NULL;

  LOG_IN();
  if (!tbx_slist_is_nil(slist))
    {
      p_tbx_slist_element_t element = NULL;
      
      element = slist->head;
      
      if (sfunc)
	{
	  while (element)
	    {
	      if (sfunc(object, element->object))
		{
		  result = __tbx_slist_free_element(slist, element);
		  LOG_OUT();

		  return result;
		}

	      element = element->next;
	    } 
	}
      else
	{
	  while (element)
	    {
	      if (element->object == object)
		{
		  result = __tbx_slist_free_element(slist, element);
		  LOG_OUT();

		  return result;
		}

	      element = element->next;
	    } 
	}
    }
  else
    FAILURE("empty list");
  LOG_OUT();

  return NULL;
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

tbx_bool_t
tbx_slist_search_forward_set_nref(p_tbx_slist_nref_t         nref,
				  p_tbx_slist_search_func_t  sfunc,
				  void                      *object)
{
  p_tbx_slist_t         slist        = NULL;
  p_tbx_slist_element_t nref_element = NULL;

  LOG_IN();
  slist = nref->slist;
  
  if (!tbx_slist_is_nil(slist))
    {
      nref_element = slist->head;
      
      if (sfunc)
	{
	  while (nref_element)
	    {
	      if (sfunc(object, nref_element->object))
		{
		  nref->element = nref_element;
		  LOG_OUT();

		  return tbx_true;
		}

	      nref_element = nref_element->next;
	    } 
	}
      else
	{
	  while (slist->ref)
	    {
	      if (slist->ref->object == object)
		{
		  nref->element = nref_element;
		  LOG_OUT();

		  return tbx_true;  
		}

	      nref_element = nref_element->next;
	    } 
	}
    }
  else
    FAILURE("empty list");
  LOG_OUT();

  return tbx_false;
}


tbx_bool_t
tbx_slist_search_backward_set_nref(p_tbx_slist_nref_t         nref,
				   p_tbx_slist_search_func_t  sfunc,
				   void                      *object)
{
  p_tbx_slist_t         slist        = NULL;
  p_tbx_slist_element_t nref_element = NULL;

  LOG_IN();
  slist = nref->slist;
  
  if (!tbx_slist_is_nil(slist))
    {
      nref_element = slist->tail;
      
      if (sfunc)
	{
	  while (nref_element)
	    {
	      if (sfunc(object, nref_element->object))
		{
		  nref->element = nref_element;
		  LOG_OUT();

		  return tbx_true;
		}

	      nref_element = nref_element->previous;
	    } 
	}
      else
	{
	  while (nref_element)
	    {
	      if (nref_element->object == object)
		{
		  nref->element = nref_element;
		  LOG_OUT();

		  return tbx_true;  
		}

	      nref_element = nref_element->previous;
	    } 
	}
    }
  else
    FAILURE("empty list");
  LOG_OUT();

  return tbx_false;
}

tbx_bool_t
tbx_slist_search_next_set_nref(p_tbx_slist_nref_t         nref,
			       p_tbx_slist_search_func_t  sfunc,
			       void                      *object)
{
  p_tbx_slist_t         slist        = NULL;
  p_tbx_slist_element_t nref_element = NULL;

  LOG_IN();
  slist        = nref->slist;
  nref_element = nref->element;
  
  if (nref_element)
    {
      if (!tbx_slist_is_nil(slist))
	{
	  if (sfunc)
	    {
	      while (nref_element)
		{
		  if (sfunc(object, nref_element->object))
		    {
		      nref->element = nref_element;
		      LOG_OUT();

		      return tbx_true;
		    }

		  nref_element = nref_element->next;
		} 
	    }
	  else
	    {
	      while (nref_element)
		{
		  if (nref_element->object == object)
		    {
		      nref->element = nref_element;
		      LOG_OUT();

		      return tbx_true;  
		    }

		  nref_element = nref_element->next;
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
tbx_slist_search_previous_set_nref(p_tbx_slist_nref_t         nref,
				   p_tbx_slist_search_func_t  sfunc,
				   void                      *object)
{
  p_tbx_slist_t         slist        = NULL;
  p_tbx_slist_element_t nref_element = NULL;

  LOG_IN();
  slist        = nref->slist;
  nref_element = nref->element;
  
  if (nref_element)
    {
      if (!tbx_slist_is_nil(slist))
	{
	  if (sfunc)
	    {
	      while (nref_element)
		{
		  if (sfunc(object, nref_element->object))
		    {
		      nref->element = nref_element;
		      LOG_OUT();

		      return tbx_true;
		    }

		  nref_element = nref_element->previous;
		} 
	    }
	  else
	    {
	      while (nref_element)
		{
		  if (nref_element->object == object)
		    {
		      nref->element = nref_element;
		      LOG_OUT();

		      return tbx_true;  
		    }

		  nref_element = nref_element->previous;
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
  tbx_slist_index_t idx = -1;

  LOG_IN();
  if (slist->ref)
    {
      p_tbx_slist_element_t element = NULL;

      element = slist->ref;

      do
	{
	  idx++;
	  element = element->previous;
	}
      while (element);
    }
  LOG_OUT();

  return idx;
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

void *
tbx_slist_ref_get(p_tbx_slist_t slist)
{
  LOG_IN();

  if (slist->ref)
    {
      if (!tbx_slist_is_nil(slist))
	{
	  void *object = NULL;

	  object = slist->ref->object;
	  LOG_OUT();

	  return object;
	}
      else
	FAILURE("empty list");
    }
  else
    FAILURE("uninitialized reference");
}

/*
 * NReference access functions
 * --------------------------
 */
p_tbx_slist_nref_t
tbx_slist_nref_alloc(p_tbx_slist_t slist)
{
  p_tbx_slist_nref_t nref = NULL;

  LOG_IN();
  nref = tbx_malloc(tbx_slist_nref_manager_memory);
  nref->element  = NULL;
  nref->previous = NULL;
  nref->next     = NULL;
  nref->slist    = slist;

  if (slist->nref_head)
    {
      nref->next           = slist->nref_head;
      nref->next->previous = nref;
      slist->nref_head     = nref;
    }

  slist->nref_head = nref;
  LOG_OUT();

  return nref;
}

void
tbx_slist_nref_free(p_tbx_slist_nref_t nref)
{
  p_tbx_slist_t slist = NULL;

  LOG_IN();
  slist = nref->slist;
  
  if (slist->nref_head == nref)
    {
      slist->nref_head = nref->next;
    }
  else
    {
      nref->previous->next = nref->next;
    }

  if (nref->next)
    {
      nref->next->previous = nref->previous;
    }
  
  nref->element  = NULL;
  nref->previous = NULL;
  nref->next     = NULL;
  nref->slist    = NULL;
  tbx_free(tbx_slist_nref_manager_memory, nref);
  LOG_OUT();
}

tbx_slist_index_t
tbx_slist_nref_get_index(p_tbx_slist_nref_t nref)
{
  tbx_slist_index_t idx = -1;

  LOG_IN();
  if (nref->element)
    {
      p_tbx_slist_element_t element = NULL;

      element = nref->element;

      do
	{
	  idx++;
	  element = element->previous;
	}
      while (element);
    }
  LOG_OUT();

  return idx;
}

void
tbx_slist_nref_to_head(p_tbx_slist_nref_t nref)
{
  p_tbx_slist_t slist = NULL;

  LOG_IN();
  slist = nref->slist;
  if (!tbx_slist_is_nil(slist))
    {
      nref->element = slist->head;
    }
  else
    FAILURE("empty list");
  LOG_OUT();
}

void
tbx_slist_nref_to_tail(p_tbx_slist_nref_t nref)
{
  p_tbx_slist_t slist = NULL;

  LOG_IN();
  slist = nref->slist;

  if (!tbx_slist_is_nil(slist))
    {
      nref->element = slist->tail;
    }
  else
    FAILURE("empty list");
  LOG_OUT();
}

tbx_bool_t
tbx_slist_nref_forward(p_tbx_slist_nref_t nref)
{
  tbx_bool_t result = tbx_false;

  LOG_IN();
  if (nref->element)
    {
      if (!tbx_slist_is_nil( nref->slist))
	{
	  nref->element = nref->element->next;
	  result = (nref->element != NULL);
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
tbx_slist_nref_backward(p_tbx_slist_nref_t nref)
{
  tbx_bool_t result = tbx_false;

  LOG_IN();
  if (nref->element)
    {
      if (!tbx_slist_is_nil(nref->slist))
	{
	  nref->element = nref->element->previous;
	  result = (nref->element != NULL);
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
tbx_slist_nref_step_forward(p_tbx_slist_nref_t   nref,
			    p_tbx_slist_offset_t offset)
{
  tbx_bool_t result = tbx_false;

  LOG_IN();
  if (nref->element)
    {
      if (!tbx_slist_is_nil(nref->slist))
	{
	  while (offset && nref->element)
	    {
	      offset--;
	      nref->element = nref->element->next;
	    }
	  
	  result = (nref->element != NULL);
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
tbx_slist_nref_step_backward(p_tbx_slist_nref_t   nref,
			     p_tbx_slist_offset_t offset)
{
  tbx_bool_t result = tbx_false;

  LOG_IN();
  if (nref->element)
    {
      if (!tbx_slist_is_nil(nref->slist))
	{
	  while (offset && nref->element)
	    {
	      offset--;
	      nref->element = nref->element->previous;
	    }
	  
	  result = (nref->element != NULL);
	}
      else
	FAILURE("empty list");
    }
  else
    FAILURE("uninitialized reference");
  LOG_OUT();

  return result;
}

void *
tbx_slist_nref_get(p_tbx_slist_nref_t nref)
{
  LOG_IN();
  if (nref->element)
    {
      if (!tbx_slist_is_nil(nref->slist))
	{
	  void *object = NULL;

	  object = nref->element->object;
	  LOG_OUT();

	  return object;
	}
      else
	FAILURE("empty list");
    }
  else
    FAILURE("uninitialized reference");
}

