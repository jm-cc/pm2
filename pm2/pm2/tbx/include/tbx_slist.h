
/*
 * Tbx_slist.h : double-linked search lists
 * ===========--------------------------------
 */

#ifndef TBX_SLIST_H
#define TBX_SLIST_H

/*
 * Data structures 
 * ---------------
 */
typedef struct s_tbx_slist_element
{
  p_tbx_slist_element_t  previous;
  p_tbx_slist_element_t  next;
  void                  *object;
} tbx_slist_element_t;

typedef struct s_tbx_slist
{
  TBX_SHARED;
  tbx_slist_length_t     length;
  p_tbx_slist_element_t  head;
  p_tbx_slist_element_t  tail;
  p_tbx_slist_element_t  ref;
} tbx_slist_t;

#endif /* TBX_SLIST_H */
