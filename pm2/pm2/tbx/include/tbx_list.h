
/*
 * Tbx_list.h
 * ==========
 */

#ifndef TBX_LIST_H
#define TBX_LIST_H

typedef void (*p_tbx_list_foreach_func_t)(void * ptr);

typedef struct s_tbx_list_element
{
  void                  *object;
  p_tbx_list_element_t   next ;
  p_tbx_list_element_t   previous ;
} tbx_list_element_t;

typedef struct s_tbx_list
{
  TBX_SHARED;
  tbx_list_length_t        length ;
  p_tbx_list_element_t     first ;
  p_tbx_list_element_t     last ;
  p_tbx_list_element_t     mark;
  tbx_list_mark_position_t mark_position ;
  tbx_bool_t               mark_next;
  tbx_bool_t               read_only;
} tbx_list_t;

/* list reference: 
 * allow additional cursor/mark over a given list
 * NOTE: the validity of cursor & mark element cannot
 *       be enforced by the list manager
 */
typedef struct s_tbx_list_reference
{
  p_tbx_list_t           list ;
  p_tbx_list_element_t   reference;
  tbx_bool_t             after_end;
} tbx_list_reference_t;

#endif /* TBX_LIST_H */
