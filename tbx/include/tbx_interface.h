/*! \file tbx_interface.h
 *  \brief TBX functional interface file
 *
 *  This file declares the prototype of any TBX exported function
 *
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
 * tbx_interface.h
 * --------------------
 */

#ifndef TBX_INTERFACE_H
#define TBX_INTERFACE_H

#include "tbx_compiler.h"

/*
 * Common
 * ------
 */
int tbx_initialized(void);

void tbx_init(int *argc, char ***argv);

void tbx_exit(void);

void tbx_purge_cmd_line(void);

/** \addtogroup timing_interface
 *  @{
 */
/*
 * Timing
 * ------
 */

extern double tbx_ticks2delay(const tbx_tick_t *t1, const tbx_tick_t *t2);

extern double tbx_tick2usec(tbx_tick_t t);

/* @} */

/** \addtogroup malloc_interface
 *  @{
 */
/*
 * Aligned malloc
 * --------------
 */
TBX_FMALLOC
static __inline__
void *tbx_aligned_malloc(const size_t size, const tbx_align_t align);

static __inline__
void tbx_aligned_free(void *ptr, const tbx_align_t align);


/*
 * Fast malloc
 * -----------
 */
static
__inline__
void
tbx_malloc_init(p_tbx_memory_t * mem, const size_t block_len,
		unsigned long initial_block_number, const char *name);

TBX_FMALLOC static
__inline__ void *tbx_malloc(p_tbx_memory_t mem);

static __inline__ void tbx_free(p_tbx_memory_t mem, void *ptr);

static __inline__ void tbx_malloc_clean(p_tbx_memory_t memory);


/* @} */


/** \addtogroup list_interface
 *  @{
 */
/*
 * List management
 * ---------------
 */
void tbx_list_manager_init(void);


static __inline__ void tbx_list_init(p_tbx_list_t list);

static
__inline__
void
tbx_foreach_destroy_list(p_tbx_list_t list,
			 const p_tbx_list_foreach_func_t func);

void tbx_list_manager_exit(void);


static __inline__ void tbx_destroy_list(p_tbx_list_t list);

static __inline__ void tbx_append_list(p_tbx_list_t list, void *object);
static __inline__ void *tbx_get_list_object(const p_tbx_list_t list);

static __inline__ void tbx_mark_list(p_tbx_list_t list);

static
__inline__
void
tbx_duplicate_list(const p_tbx_list_t source, p_tbx_list_t destination);

static
__inline__
void
tbx_extract_sub_list(const p_tbx_list_t source, p_tbx_list_t destination);

static
__inline__ tbx_bool_t tbx_empty_list(const p_tbx_list_t list);

static
__inline__
void
tbx_list_reference_init(p_tbx_list_reference_t ref,
			const p_tbx_list_t list);

static
__inline__
void *tbx_get_list_reference_object(const p_tbx_list_reference_t ref);

static
__inline__
tbx_bool_t tbx_forward_list_reference(p_tbx_list_reference_t ref);

static
__inline__ void tbx_reset_list_reference(p_tbx_list_reference_t ref);

static
__inline__
tbx_bool_t
tbx_reference_after_end_of_list(const p_tbx_list_reference_t ref);

/* @} */

/** \addtogroup slist_interface
 *  @{
 */
/*
 * Search list management
 * ----------------------
 */

void tbx_slist_manager_init(void);

void tbx_slist_manager_exit(void);

static __inline__ p_tbx_slist_t tbx_slist_nil(void);

static __inline__ void tbx_slist_free(p_tbx_slist_t slist);

static __inline__ void tbx_slist_clear(p_tbx_slist_t slist);

static __inline__ void tbx_slist_clear_and_free(p_tbx_slist_t slist);

static
__inline__
TBX_FMALLOC
p_tbx_slist_element_t tbx_slist_alloc_element(void *object);

static
__inline__
tbx_slist_length_t tbx_slist_get_length(const p_tbx_slist_t slist);

static __inline__ tbx_bool_t tbx_slist_is_nil(const p_tbx_slist_t slist);

static
__inline__
p_tbx_slist_t
tbx_slist_dup_ext(const p_tbx_slist_t source,
		  p_tbx_slist_dup_func_t dfunc);

static
__inline__ p_tbx_slist_t tbx_slist_dup(const p_tbx_slist_t source);

static
__inline__
void tbx_slist_add_at_head(p_tbx_slist_t slist, void *object);

static
__inline__
void tbx_slist_add_at_tail(p_tbx_slist_t slist, void *object);

static __inline__ void *tbx_slist_remove_from_head(p_tbx_slist_t slist);

static __inline__ void *tbx_slist_remove_from_tail(p_tbx_slist_t slist);

static __inline__ void *tbx_slist_peek_from_head(p_tbx_slist_t slist);

static __inline__ void *tbx_slist_peek_from_tail(p_tbx_slist_t slist);

static
__inline__ void tbx_slist_enqueue(p_tbx_slist_t slist, void *object);

static __inline__ void *tbx_slist_dequeue(p_tbx_slist_t slist);

static __inline__ void tbx_slist_push(p_tbx_slist_t slist, void *object);

static __inline__ void *tbx_slist_pop(p_tbx_slist_t slist);

static __inline__ void tbx_slist_append(p_tbx_slist_t slist, void *object);

static __inline__ void *tbx_slist_extract(p_tbx_slist_t slist);

static
__inline__
p_tbx_slist_t tbx_slist_cons(void *object, const p_tbx_slist_t source);

static
__inline__
void
tbx_slist_merge_before_ext(p_tbx_slist_t destination,
			   const p_tbx_slist_t source,
			   p_tbx_slist_dup_func_t dfunc);

static
__inline__
void
tbx_slist_merge_after_ext(p_tbx_slist_t destination,
			  const p_tbx_slist_t source,
			  p_tbx_slist_dup_func_t dfunc);

static
__inline__
p_tbx_slist_t
tbx_slist_merge_ext(const p_tbx_slist_t destination,
		    const p_tbx_slist_t source,
		    p_tbx_slist_dup_func_t dfunc);

static
__inline__
void
tbx_slist_merge_before(p_tbx_slist_t destination,
		       const p_tbx_slist_t source);

static
__inline__
void
tbx_slist_merge_after(p_tbx_slist_t destination,
		      const p_tbx_slist_t source);

static
__inline__
p_tbx_slist_t
tbx_slist_merge(const p_tbx_slist_t destination,
		const p_tbx_slist_t source);

static
__inline__ void tbx_slist_reverse_list(p_tbx_slist_t slist);

static
__inline__ p_tbx_slist_t tbx_slist_reverse(const p_tbx_slist_t source);

static
__inline__
void *tbx_slist_index_get(const p_tbx_slist_t slist,
			  tbx_slist_index_t indx);

static
__inline__
void *tbx_slist_index_extract(p_tbx_slist_t slist,
			      tbx_slist_index_t indx);

static
__inline__
void
tbx_slist_index_set_ref(p_tbx_slist_t slist, tbx_slist_index_t indx);

static
__inline__
void
tbx_slist_index_set_nref(p_tbx_slist_nref_t nref, tbx_slist_index_t idx);

static
__inline__
tbx_slist_index_t
tbx_slist_search_get_index(const p_tbx_slist_t slist,
			   const p_tbx_slist_search_func_t sfunc,
			   void *object);

static
__inline__
void *tbx_slist_search_and_extract(p_tbx_slist_t slist,
				   const p_tbx_slist_search_func_t
				   sfunc, void *object);

static
__inline__
tbx_bool_t
tbx_slist_search_forward_set_ref(p_tbx_slist_t slist,
				 const p_tbx_slist_search_func_t sfunc,
				 void *object);

static
__inline__
tbx_bool_t
tbx_slist_search_backward_set_ref(p_tbx_slist_t slist,
				  const p_tbx_slist_search_func_t sfunc,
				  void *object);

static
__inline__
tbx_bool_t
tbx_slist_search_next_set_ref(p_tbx_slist_t slist,
			      const p_tbx_slist_search_func_t sfunc,
			      void *object);

static
__inline__
tbx_bool_t
tbx_slist_search_previous_set_ref(p_tbx_slist_t slist,
				  const p_tbx_slist_search_func_t sfunc,
				  void *object);

static
__inline__
tbx_bool_t
tbx_slist_search_forward_set_nref(p_tbx_slist_nref_t nref,
				  p_tbx_slist_search_func_t sfunc,
				  void *object);

static
__inline__
tbx_bool_t
tbx_slist_search_backward_set_nref(p_tbx_slist_nref_t nref,
				   p_tbx_slist_search_func_t sfunc,
				   void *object);

static
__inline__
tbx_bool_t
tbx_slist_search_next_set_nref(p_tbx_slist_nref_t nref,
			       p_tbx_slist_search_func_t sfunc,
			       void *object);

static
__inline__
tbx_bool_t
tbx_slist_search_previous_set_nref(p_tbx_slist_nref_t nref,
				   p_tbx_slist_search_func_t sfunc,
				   void *object);

static
__inline__
tbx_slist_index_t tbx_slist_ref_get_index(const p_tbx_slist_t slist);

static __inline__ void tbx_slist_ref_to_head(p_tbx_slist_t slist);

static __inline__ void tbx_slist_ref_to_tail(p_tbx_slist_t slist);

static __inline__ tbx_bool_t tbx_slist_ref_forward(p_tbx_slist_t slist);

static __inline__ tbx_bool_t tbx_slist_ref_backward(p_tbx_slist_t slist);

static
__inline__
tbx_bool_t
tbx_slist_ref_extract_and_forward(p_tbx_slist_t slist, void **p_object);

static
__inline__
tbx_bool_t
tbx_slist_ref_extract_and_backward(p_tbx_slist_t slist, void **p_object);

static
__inline__
tbx_bool_t
tbx_slist_ref_step_forward(p_tbx_slist_t slist,
			   p_tbx_slist_offset_t offset);

static
__inline__
tbx_bool_t
tbx_slist_ref_step_backward(p_tbx_slist_t slist,
			    p_tbx_slist_offset_t offset);

static
__inline__ void *tbx_slist_ref_get(const p_tbx_slist_t slist);

static
__inline__
TBX_FMALLOC
p_tbx_slist_nref_t tbx_slist_nref_alloc(p_tbx_slist_t slist);

static __inline__ void tbx_slist_nref_free(p_tbx_slist_nref_t nref);

static
__inline__
tbx_slist_index_t tbx_slist_nref_get_index(p_tbx_slist_nref_t nref);

static __inline__ void tbx_slist_nref_to_head(p_tbx_slist_nref_t nref);

static __inline__ void tbx_slist_nref_to_tail(p_tbx_slist_nref_t nref);

static
__inline__ tbx_bool_t tbx_slist_nref_forward(p_tbx_slist_nref_t nref);

static
__inline__ tbx_bool_t tbx_slist_nref_backward(p_tbx_slist_nref_t nref);

static
__inline__
tbx_bool_t
tbx_slist_nref_step_forward(p_tbx_slist_nref_t nref,
			    p_tbx_slist_offset_t offset);

static
__inline__
tbx_bool_t
tbx_slist_nref_step_backward(p_tbx_slist_nref_t nref,
			     p_tbx_slist_offset_t offset);

static
__inline__ void *tbx_slist_nref_get(p_tbx_slist_nref_t nref);

/* @} */

/** \addtogroup htable_interface
 *  @{
 */
/*
 * Hash table management
 * ---------------------
 */

void tbx_htable_manager_init(void);

void
tbx_htable_init(p_tbx_htable_t htable, tbx_htable_bucket_count_t buckets);

p_tbx_htable_t tbx_htable_empty_table(void);

tbx_bool_t tbx_htable_empty(const p_tbx_htable_t htable);

tbx_htable_element_count_t
tbx_htable_get_size(const p_tbx_htable_t htable);

void tbx_htable_add(p_tbx_htable_t htable, const char *key, void *object);

void
tbx_htable_add_ext(p_tbx_htable_t htable,
		   const char *key, const size_t key_len, void *object);

void *tbx_htable_get(const p_tbx_htable_t htable, const char *key);

void *tbx_htable_get_ext(p_tbx_htable_t htable,
			 const char *key, const size_t key_len);

void *tbx_htable_replace(p_tbx_htable_t htable,
			 const char *key, void *object);

void *tbx_htable_replace_ext(p_tbx_htable_t htable,
			     const char *key,
			     const size_t key_len, void *object);

void *tbx_htable_extract(p_tbx_htable_t htable, const char *key);

void *tbx_htable_extract_ext(p_tbx_htable_t htable,
			     const char *key, const size_t key_len);

void tbx_htable_free(p_tbx_htable_t htable);

void tbx_htable_cleanup_and_free(p_tbx_htable_t htable);

void tbx_htable_manager_exit(void);

p_tbx_slist_t tbx_htable_get_key_slist(const p_tbx_htable_t htable);

void tbx_htable_dump_keys(const p_tbx_htable_t htable);

void tbx_htable_dump_keys_strvals(p_tbx_htable_t htable);

void tbx_htable_dump_keys_ptrvals(p_tbx_htable_t htable);

/* @} */

/** \addtogroup string_interface
 *  @{
 */

/*
 * String management
 * -----------------
 */
void tbx_string_manager_init(void);

void tbx_string_manager_exit(void);

char *tbx_strdup(const char *src);

tbx_bool_t tbx_streq(const char *s1, const char *s2);

p_tbx_string_t tbx_string_init(void);

void tbx_string_free(p_tbx_string_t string);

size_t tbx_string_length(const p_tbx_string_t string);

void tbx_string_reset(p_tbx_string_t string);

char *tbx_string_to_cstring(const p_tbx_string_t string);

char *tbx_string_to_cstring_and_free(p_tbx_string_t string);

void tbx_string_set_to_cstring(p_tbx_string_t string, const char *cstring);

void
tbx_string_set_to_cstring_and_free(p_tbx_string_t string, char *cstring);

void tbx_string_set_to_char(p_tbx_string_t string, const int data);

p_tbx_string_t tbx_string_init_to_cstring(const char *cstring);

p_tbx_string_t tbx_string_init_to_cstring_and_free(char *cstring);

p_tbx_string_t tbx_string_init_to_char(const int data);

void tbx_string_append_cstring(p_tbx_string_t string, const char *cstring);

void
tbx_string_append_cstring_and_free(p_tbx_string_t string, char *cstring);

void tbx_string_append_char(p_tbx_string_t string, const int data);

void tbx_string_append_int(p_tbx_string_t string, const int data);

void tbx_string_set_to_int(p_tbx_string_t string, const int data);

p_tbx_string_t tbx_string_init_to_int(const int data);

void
tbx_string_append_uint(p_tbx_string_t string, const unsigned int data);

void
tbx_string_set_to_uint(p_tbx_string_t string, const unsigned int data);

p_tbx_string_t tbx_string_init_to_uint(const unsigned int data);

void tbx_string_reverse(p_tbx_string_t string);

void
tbx_string_set_to_string(p_tbx_string_t dst_string,
			 const p_tbx_string_t src_string);

void
tbx_string_set_to_string_and_free(p_tbx_string_t dst_string,
				  p_tbx_string_t src_string);
void
tbx_string_append_string(p_tbx_string_t dst_string,
			 const p_tbx_string_t src_string);

void
tbx_string_append_string_and_free(p_tbx_string_t dst_string,
				  p_tbx_string_t src_string);

p_tbx_string_t tbx_string_init_to_string(const p_tbx_string_t src_string);

p_tbx_string_t tbx_string_dup(p_tbx_string_t src_string);

p_tbx_string_t
tbx_string_init_to_string_and_free(p_tbx_string_t src_string);

p_tbx_string_t tbx_string_double_quote(const p_tbx_string_t src_string);

p_tbx_string_t tbx_string_double_quote_and_free(p_tbx_string_t src_string);

p_tbx_string_t tbx_string_single_quote(const p_tbx_string_t src_string);

p_tbx_string_t tbx_string_single_quote_and_free(p_tbx_string_t src_string);

p_tbx_slist_t
tbx_string_split(const p_tbx_string_t src_string, const char *IFS);

p_tbx_slist_t
tbx_string_split_and_free(p_tbx_string_t src_string, const char *IFS);

p_tbx_string_t
tbx_string_extract_name_from_pathname(p_tbx_string_t path_name);

long tbx_cstr_to_long(const char *s);

unsigned long tbx_cstr_to_unsigned_long(const char *s);

/* @} */

/** \addtogroup darray_interface
 *  @{
 */
/*
 * Dynamic arrays management
 * ------------------------
 */
void tbx_darray_manager_init(void);

void tbx_darray_manager_exit(void);

static __inline__ p_tbx_darray_t tbx_darray_init(void);

static __inline__ void tbx_darray_free(p_tbx_darray_t darray);

static __inline__ size_t tbx_darray_length(p_tbx_darray_t darray);

static __inline__ void tbx_darray_reset(p_tbx_darray_t darray);

static
__inline__
void *tbx_darray_get(p_tbx_darray_t darray, tbx_darray_index_t idx);

static
__inline__
void
tbx_darray_set(p_tbx_darray_t darray,
	       tbx_darray_index_t idx, void *object);

static
__inline__
void
tbx_darray_expand_and_set(p_tbx_darray_t darray,
			  tbx_darray_index_t idx, void *object);

static
__inline__
void *tbx_darray_expand_and_get(p_tbx_darray_t darray,
				tbx_darray_index_t idx);

static
__inline__ void tbx_darray_append(p_tbx_darray_t darray, void *object);

static
__inline__
void *tbx_darray_first_idx(p_tbx_darray_t darray,
			   p_tbx_darray_index_t idx);

static
__inline__
void *tbx_darray_next_idx(p_tbx_darray_t darray,
			  p_tbx_darray_index_t idx);

/* @} */

#endif				/* __TBX_INTERFACE_H */
