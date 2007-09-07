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
 * tbx_list_inline.h
 * -----------------
 */

#ifndef TBX_LIST_INLINE_H
#define TBX_LIST_INLINE_H

/** \addtogroup list_interface
 *
 * @{
 */

extern p_tbx_memory_t tbx_list_manager_memory;

static
__inline__
void
tbx_list_init(p_tbx_list_t list)
{
  list->length     = 0;
  list->read_only = tbx_false;
  list->mark_next = tbx_true; /* mark is always set
				 on the first element */
}

static
__inline__
void
tbx_destroy_list(p_tbx_list_t list)
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

static
__inline__
void
tbx_foreach_destroy_list(p_tbx_list_t                list,
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
static
__inline__
void
tbx_append_list(p_tbx_list_t   list,
		void          *object)
{
  p_tbx_list_element_t list_element ;

  if (list->read_only)
    TBX_FAILURE("attempted to modify a read only list");

  list_element = (p_tbx_list_element_t)tbx_malloc(tbx_list_manager_memory);

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

static
__inline__
void *
tbx_get_list_object(p_tbx_list_t list)
{
  if (list->length)
    {
      return list->last->object;
    }
  else
    TBX_FAILURE("empty list");
}


/*
 * list mark control function
 * --------------------------
 */

static
__inline__
void
tbx_mark_list(p_tbx_list_t list)
{
  /* start a new sub list -> the next element
     added to the list will be the first element
     of the new sub list */
  list->mark_next = tbx_true;
}

static
__inline__
tbx_bool_t
tbx_empty_list(p_tbx_list_t list)
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

static
__inline__
void
tbx_duplicate_list(p_tbx_list_t   source,
		   p_tbx_list_t   destination)
{
  *destination = *source ;
  destination->mark          = destination->first ;
  destination->mark_position = 0;
  destination->read_only     = tbx_true;
}

static
__inline__
void
tbx_extract_sub_list(p_tbx_list_t   source,
		     p_tbx_list_t   destination)
{
  if (source->length)
    {
      destination->first         = source->mark ;
      destination->last          = source->last ;
      destination->length        = source->length - source->mark_position;
      destination->mark          = destination->first ;
      destination->mark_position = 0;
      destination->read_only     = tbx_true;
    }
  else
    TBX_FAILURE("empty list");
}

/*
 * list reference functions
 * ------------------------
 */

static
__inline__
void
tbx_list_reference_init(p_tbx_list_reference_t   ref,
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

static
__inline__
void *
tbx_get_list_reference_object(p_tbx_list_reference_t ref)
{
  if (!ref->reference)
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
    TBX_FAILURE("empty list");
}

/*
 * Cursor control function
 * -----------------------
 */
static
__inline__
tbx_bool_t
tbx_forward_list_reference(p_tbx_list_reference_t ref)
{
  if (!ref->reference)
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
    TBX_FAILURE("empty list");
}

static
__inline__
void
tbx_reset_list_reference(p_tbx_list_reference_t ref)
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

static
__inline__
tbx_bool_t
tbx_reference_after_end_of_list(p_tbx_list_reference_t ref)
{
  LOG_IN();
  if (!ref->reference)
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

/* @} */

#endif /* TBX_LIST_INLINE_H */
