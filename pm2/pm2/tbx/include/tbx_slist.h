
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
