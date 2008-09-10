/*! \file tbx_types.h
 *  \brief TBX simple types definition.
 *  This file defines TBX simple, general use types.
 */

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


/*! \typedef tbx_bool_t
 *  The boolean type.
 *  \sa p_tbx_bool_t
 */
/*! \typedef p_tbx_bool_t
 *  A pointer to a boolean object.
 *  \sa tbx_bool_t
 */
/*! \enum e_tbx_bool
 *  A boolean 
 *  \sa tbx_bool_t, p_tbx_bool_t
 */
#if (defined __cplusplus) || (__STDC_VERSION__ >= 199901L)

/* C++ and C99 have their own boolean type, which we must use to allow
   `tbx_bool_t' to be used as the native `bool' type.  Otherwise, problems
   about invalid casts may arise in situations like:

     tbx_bool_t foo (int x) { return (x >= 2); }

   where the return expression has type `bool', which is incompatible with
   `tbx_bool_t'.  */

typedef bool tbx_bool_t;
typedef tbx_bool_t *p_tbx_bool_t;

#define tbx_false false
#define tbx_true  true

#else

typedef enum e_tbx_bool
{
  tbx_false = 0, /* !< condition is false */
  tbx_true       /* !< condition is true */
} tbx_bool_t, *p_tbx_bool_t;

#endif

/*! \typedef tbx_flag_t
 *  The general purpose flag type.
 * \sa p_tbx_flag_t
 */
/*! \typedef p_tbx_flag_t
 *  A pointer to a flag object.
 * \sa tbx_flag_t
 */
/*! \enum e_tbx_flag
 *  A flag value.
 * \sa tbx_flag_t, p_tbx_flag_t
 */
typedef enum e_tbx_flag
{
  tbx_flag_clear = 0, /* !< flag is cleared */
  tbx_flag_set   = 1  /* !< flag is set */
} tbx_flag_t, *p_tbx_flag_t;


/*! \typedef tbx_cmp_t
 *  The comparison result type.
 * \sa p_tbx_cmp_t
 */
/*! \typedef p_tbx_cmp_t
 *  A pointer to a comparison result object.
 * \sa tbx_cmp_t
 */
/*! \enum e_tbx_cmp
 *  A comparison function result value.
 * \sa tbx_cmp_t, p_tbx_cmp_t
 */
typedef enum e_tbx_cmp
{
  tbx_cmp_eq =  0, /* !< objects are equal */
  tbx_cmp_gt =  1, /* !< object 1 is greater than object 2 */
  tbx_cmp_lt = -1  /* !< object 1 is lower than object 2 */
} tbx_cmp_t, *p_tbx_cmp_t;


/*! \typedef tbx_align_t
 *  A safe-malloc alignment value.
 *  \sa p_tbx_align_t
 */
/*! \typedef p_tbx_align_t
 *  A pointer to a safe-malloc alignment object.
 *  \sa tbx_align_t
 */
typedef unsigned long tbx_align_t, *p_tbx_align_t;


/*! \typedef tbx_list_length_t
 *  A linked list length.
 *  \sa p_tbx_list_length_t
 */
/*! \typedef tbx_list_length_t
 *  A pointer to a linked list length object.
 *  \sa tbx_list_length_t
 */
typedef int tbx_list_length_t, *p_tbx_list_length_t;


/*! \typedef tbx_list_mark_position_t
 *  A linked list mark position.
 *  \sa p_tbx_list_mark_position_t
 */
/*! \typedef tbx_list_mark_position_t
 *  A pointer to a linked list mark position object.
 *  \sa tbx_list_mark_position_t
 */
typedef int tbx_list_mark_position_t, *p_tbx_list_mark_position_t;


/*! \typedef tbx_slist_index_t
 *  A slist index value.
 *  \sa p_tbx_slist_index_t
 */
/*! \typedef p_tbx_slist_index_t
 *  A pointer to a slist index object.
 *  \sa tbx_slist_index_t
 */
typedef int tbx_slist_index_t, *p_tbx_slist_index_t;


/*! \typedef tbx_slist_length_t
 *  A slist index length.
 *  \sa p_tbx_slist_length_t
 */
/*! \typedef p_tbx_slist_length_t
 *  A pointer to a slist index length object.
 *  \sa tbx_slist_length_t
 */
typedef int tbx_slist_length_t, *p_tbx_slist_length_t;


/*! \typedef tbx_slist_offset_t
 *  A slist offset.
 *  \sa p_tbx_slist_offset_t
 */
/*! \typedef p_tbx_slist_offset_t
 *  A pointer to a slist offset object.
 *  \sa tbx_slist_offset_t
 */
typedef int tbx_slist_offset_t, *p_tbx_slist_offset_t;


/*! \typedef p_tbx_slist_search_func_t
 *  A slist search function.
 */
typedef tbx_bool_t (*p_tbx_slist_search_func_t)(void *ref_obj, void *obj);


/*! \typedef p_tbx_slist_cmp_func_t
 *  A slist comparison function.
 */
typedef tbx_cmp_t (*p_tbx_slist_cmp_func_t)(void *ref_obj, void *obj);


/*! \typedef p_tbx_slist_dup_func_t
 *  A slist duplication function.
 */
typedef void * (*p_tbx_slist_dup_func_t)(void *object);


/*! \typedef tbx_darray_index_t
 *  A darray index value.
 *  \sa p_tbx_darray_index_t
 */
/*! \typedef p_tbx_darray_index_t
 *  A pointer to a darray index object.
 *  \sa tbx_darray_index_t
 */
typedef int tbx_darray_index_t, *p_tbx_darray_index_t;


/*! \typedef p_tbx_specific_dest_func_t
 *  A \em specific field destructor function.
 */
typedef void (*p_tbx_specific_dest_func_t)(void *specific);

#endif /* TBX_TYPES_H */
