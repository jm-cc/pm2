
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
 * Tbx_types.h
 * ===========
 */

#ifndef TBX_TYPES_H
#define TBX_TYPES_H

/*
 * General purpose types
 * :::::::::::::::::::::______________________________________________________
 */

/*
 * Classical boolean type
 * ----------------------
 */
typedef enum
{
  tbx_false = 0,
  tbx_true
} tbx_bool_t, *p_tbx_bool_t;

/*
 * Flag type
 * ----------------------
 */
typedef enum
{
  tbx_flag_clear = 0,
  tbx_flag_set   = 1
} tbx_flag_t, *p_tbx_flag_t;

/*
 * Comparison type
 * ----------------------
 */
typedef enum
{
  tbx_cmp_eq =  0,  /* == */
  tbx_cmp_gt =  1,  /*  > */
  tbx_cmp_lt = -1   /*  < */
} tbx_cmp_t, *p_tbx_cmp_t;

/*
 * Safe malloc related types
 * :::::::::::::::::::::::::__________________________________________________
 */
typedef unsigned long tbx_align_t, *p_tbx_align_t;

/*
 * List management related types 
 * :::::::::::::::::::::::::::::______________________________________________
 */
typedef int tbx_list_length_t,        *p_tbx_list_length_t;
typedef int tbx_list_mark_position_t, *p_tbx_list_mark_position_t;


/*
 * SList management related types 
 * ::::::::::::::::::::::::::::::_____________________________________________
 */
typedef int tbx_slist_index_t,  *p_tbx_slist_index_t;
typedef int tbx_slist_length_t, *p_tbx_slist_length_t;
typedef int tbx_slist_offset_t, *p_tbx_slist_offset_t;
typedef tbx_bool_t (*p_tbx_slist_search_func_t)(void *ref_obj, void *obj);
typedef tbx_cmp_t  (*p_tbx_slist_cmp_func_t)(void *ref_obj, void *obj);


#endif /* TBX_TYPES_H */
