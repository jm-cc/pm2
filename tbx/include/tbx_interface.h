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
int
tbx_initialized(void);

void
tbx_init(int    argc,
	 char **argv);

void
tbx_purge_cmd_line(int   *argc,
		   char **argv);

void
tbx_exit(void);

void
tbx_dump(unsigned char *p,
         size_t	n);


/*
 * Destructors
 * -----------
 */
void
tbx_default_specific_dest(void *specific);

/** \addtogroup timing_interface
 *  @{
 */
/*
 * Timing
 * ------
 */
void
tbx_timing_init(void);

void
tbx_timing_exit(void);

double
tbx_tick2usec(long long t);

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
void *
tbx_aligned_malloc(const size_t      size,
		   const tbx_align_t align);

static __inline__
void
tbx_aligned_free (void              *ptr,
		  const tbx_align_t  align);

/*
 * Safe malloc
 * -----------
 */

void
tbx_set_print_stats_mode(tbx_bool_t);

tbx_bool_t
tbx_get_print_stats_mode(void);

void
tbx_safe_malloc_init(void);

void
tbx_safe_malloc_exit(void);

TBX_FMALLOC
void *
tbx_safe_malloc(const size_t    size,
		const char     *file,
		const unsigned  line);

TBX_FMALLOC
void *
tbx_safe_calloc(const size_t    nmemb,
		const size_t    size,
		const char     *file,
		const unsigned  line);

void
tbx_safe_free(void           *ptr,
              const char     *file,
              const unsigned  line);

void
tbx_safe_malloc_check(const tbx_safe_malloc_mode_t mode);

TBX_FMALLOC
void *
tbx_safe_realloc(void     *ptr,
		 const size_t    size,
		 const char     *file,
		 const unsigned  line);


/*
 * Fast malloc
 * -----------
 */
static
__inline__
void
tbx_malloc_init(p_tbx_memory_t *mem,
		const size_t    block_len,
		long            initial_block_number,
                const char     *name);

TBX_FMALLOC
static
__inline__
void *
tbx_malloc(p_tbx_memory_t mem);

static
__inline__
void
tbx_free(p_tbx_memory_t  mem,
         void           *ptr);

static
__inline__
void
tbx_malloc_clean(p_tbx_memory_t memory);


/* @} */


/** \addtogroup list_interface
 *  @{
 */
/*
 * List management
 * ---------------
 */
void
tbx_list_manager_init(void);


static
__inline__
void
tbx_list_init(p_tbx_list_t list);

static
__inline__
void
tbx_foreach_destroy_list(p_tbx_list_t                    list,
			 const p_tbx_list_foreach_func_t func);

void
tbx_list_manager_exit(void);


static
__inline__
void
tbx_destroy_list(p_tbx_list_t list);

static
__inline__
void
tbx_append_list(p_tbx_list_t   list,
		void*          object);
static
__inline__
void *
tbx_get_list_object(const p_tbx_list_t list);

static
__inline__
void
tbx_mark_list(p_tbx_list_t list);

static
__inline__
void
tbx_duplicate_list(const p_tbx_list_t source,
		   p_tbx_list_t destination);

static
__inline__
void
tbx_extract_sub_list(const p_tbx_list_t source,
		     p_tbx_list_t destination);

static
__inline__
tbx_bool_t
tbx_empty_list(const p_tbx_list_t list);

static
__inline__
void
tbx_list_reference_init(p_tbx_list_reference_t ref,
			const p_tbx_list_t     list);

static
__inline__
void *
tbx_get_list_reference_object(const p_tbx_list_reference_t ref);

static
__inline__
tbx_bool_t
tbx_forward_list_reference(p_tbx_list_reference_t ref);

static
__inline__
void
tbx_reset_list_reference(p_tbx_list_reference_t ref);

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

void
tbx_slist_manager_init(void);

void
tbx_slist_manager_exit(void);

static
__inline__
p_tbx_slist_t
tbx_slist_nil(void);

static
__inline__
void
tbx_slist_free(p_tbx_slist_t slist);

static
__inline__
void
tbx_slist_clear(p_tbx_slist_t slist);

static
__inline__
void
tbx_slist_clear_and_free(p_tbx_slist_t slist);

static
__inline__
TBX_FMALLOC
p_tbx_slist_element_t
tbx_slist_alloc_element(void *object);

static
__inline__
tbx_slist_length_t
tbx_slist_get_length(const p_tbx_slist_t slist);

static
__inline__
tbx_bool_t
tbx_slist_is_nil(const p_tbx_slist_t slist);

static
__inline__
p_tbx_slist_t
tbx_slist_dup_ext(const p_tbx_slist_t    source,
		  p_tbx_slist_dup_func_t dfunc);

static
__inline__
p_tbx_slist_t
tbx_slist_dup(const p_tbx_slist_t source);

static
__inline__
void
tbx_slist_add_at_head(p_tbx_slist_t  slist,
		      void          *object);

static
__inline__
void
tbx_slist_add_at_tail(p_tbx_slist_t  slist,
		      void          *object);

static
__inline__
void *
tbx_slist_remove_from_head(p_tbx_slist_t slist);

static
__inline__
void *
tbx_slist_remove_from_tail(p_tbx_slist_t slist);

static
__inline__
void *
tbx_slist_peek_from_head(p_tbx_slist_t slist);

static
__inline__
void *
tbx_slist_peek_from_tail(p_tbx_slist_t slist);

static
__inline__
void
tbx_slist_enqueue(p_tbx_slist_t  slist,
		  void          *object);

static
__inline__
void *
tbx_slist_dequeue(p_tbx_slist_t slist);

static
__inline__
void
tbx_slist_push(p_tbx_slist_t  slist,
	       void          *object);

static
__inline__
void *
tbx_slist_pop(p_tbx_slist_t slist);

static
__inline__
void
tbx_slist_append(p_tbx_slist_t  slist,
		 void          *object);

static
__inline__
void *
tbx_slist_extract(p_tbx_slist_t slist);

static
__inline__
p_tbx_slist_t
tbx_slist_cons(void                *object,
	       const p_tbx_slist_t  source);

static
__inline__
void
tbx_slist_merge_before_ext(p_tbx_slist_t          destination,
			   const p_tbx_slist_t    source,
			   p_tbx_slist_dup_func_t dfunc);

static
__inline__
void
tbx_slist_merge_after_ext(p_tbx_slist_t          destination,
			  const p_tbx_slist_t    source,
			  p_tbx_slist_dup_func_t dfunc);

static
__inline__
p_tbx_slist_t
tbx_slist_merge_ext(const p_tbx_slist_t    destination,
		    const p_tbx_slist_t    source,
		    p_tbx_slist_dup_func_t dfunc);

static
__inline__
void
tbx_slist_merge_before(      p_tbx_slist_t destination,
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
__inline__
void
tbx_slist_reverse_list(p_tbx_slist_t slist);

static
__inline__
p_tbx_slist_t
tbx_slist_reverse(const p_tbx_slist_t source);

static
__inline__
void *
tbx_slist_index_get(const p_tbx_slist_t slist,
		    tbx_slist_index_t   indx);

static
__inline__
void *
tbx_slist_index_extract(p_tbx_slist_t     slist,
			tbx_slist_index_t indx);

static
__inline__
void
tbx_slist_index_set_ref(p_tbx_slist_t     slist,
			tbx_slist_index_t indx);

static
__inline__
void
tbx_slist_index_set_nref(p_tbx_slist_nref_t nref,
			 tbx_slist_index_t  idx);

static
__inline__
tbx_slist_index_t
tbx_slist_search_get_index(const p_tbx_slist_t              slist,
			   const p_tbx_slist_search_func_t  sfunc,
			   void                            *object);

static
__inline__
void *
tbx_slist_search_and_extract(p_tbx_slist_t                    slist,
			     const p_tbx_slist_search_func_t  sfunc,
			     void                            *object);

static
__inline__
tbx_bool_t
tbx_slist_search_forward_set_ref(p_tbx_slist_t                    slist,
				 const p_tbx_slist_search_func_t  sfunc,
				 void                            *object);

static
__inline__
tbx_bool_t
tbx_slist_search_backward_set_ref(p_tbx_slist_t                    slist,
				  const p_tbx_slist_search_func_t  sfunc,
				  void                            *object);

static
__inline__
tbx_bool_t
tbx_slist_search_next_set_ref(p_tbx_slist_t                    slist,
			      const p_tbx_slist_search_func_t  sfunc,
			      void                            *object);

static
__inline__
tbx_bool_t
tbx_slist_search_previous_set_ref(p_tbx_slist_t                    slist,
				  const p_tbx_slist_search_func_t  sfunc,
				  void                            *object);

static
__inline__
tbx_bool_t
tbx_slist_search_forward_set_nref(p_tbx_slist_nref_t         nref,
				  p_tbx_slist_search_func_t  sfunc,
				  void                      *object);

static
__inline__
tbx_bool_t
tbx_slist_search_backward_set_nref(p_tbx_slist_nref_t        nref,
				   p_tbx_slist_search_func_t  sfunc,
				   void                      *object);

static
__inline__
tbx_bool_t
tbx_slist_search_next_set_nref(p_tbx_slist_nref_t         nref,
			       p_tbx_slist_search_func_t  sfunc,
			       void                      *object);

static
__inline__
tbx_bool_t
tbx_slist_search_previous_set_nref(p_tbx_slist_nref_t         nref,
				   p_tbx_slist_search_func_t  sfunc,
				   void                      *object);

static
__inline__
tbx_slist_index_t
tbx_slist_ref_get_index(const p_tbx_slist_t slist);

static
__inline__
void
tbx_slist_ref_to_head(p_tbx_slist_t slist);

static
__inline__
void
tbx_slist_ref_to_tail(p_tbx_slist_t slist);

static
__inline__
tbx_bool_t
tbx_slist_ref_forward(p_tbx_slist_t slist);

static
__inline__
tbx_bool_t
tbx_slist_ref_backward(p_tbx_slist_t slist);

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
tbx_slist_ref_step_forward(p_tbx_slist_t        slist,
			   p_tbx_slist_offset_t offset);

static
__inline__
tbx_bool_t
tbx_slist_ref_step_backward(p_tbx_slist_t        slist,
			    p_tbx_slist_offset_t offset);

static
__inline__
void *
tbx_slist_ref_get(const p_tbx_slist_t slist);

static
__inline__
TBX_FMALLOC
p_tbx_slist_nref_t
tbx_slist_nref_alloc(p_tbx_slist_t slist);

static
__inline__
void
tbx_slist_nref_free(p_tbx_slist_nref_t nref);

static
__inline__
tbx_slist_index_t
tbx_slist_nref_get_index(p_tbx_slist_nref_t nref);

static
__inline__
void
tbx_slist_nref_to_head(p_tbx_slist_nref_t nref);

static
__inline__
void
tbx_slist_nref_to_tail(p_tbx_slist_nref_t nref);

static
__inline__
tbx_bool_t
tbx_slist_nref_forward(p_tbx_slist_nref_t nref);

static
__inline__
tbx_bool_t
tbx_slist_nref_backward(p_tbx_slist_nref_t nref);

static
__inline__
tbx_bool_t
tbx_slist_nref_step_forward(p_tbx_slist_nref_t   nref,
			    p_tbx_slist_offset_t offset);

static
__inline__
tbx_bool_t
tbx_slist_nref_step_backward(p_tbx_slist_nref_t   nref,
			     p_tbx_slist_offset_t offset);

static
__inline__
void *
tbx_slist_nref_get(p_tbx_slist_nref_t nref);

/* @} */

/** \addtogroup htable_interface
 *  @{
 */
/*
 * Hash table management
 * ---------------------
 */
void
tbx_htable_manager_init(void);

void
tbx_htable_init(p_tbx_htable_t            htable,
		tbx_htable_bucket_count_t buckets);

p_tbx_htable_t
tbx_htable_empty_table(void);

tbx_bool_t
tbx_htable_empty(const p_tbx_htable_t htable);

tbx_htable_element_count_t
tbx_htable_get_size(const p_tbx_htable_t htable);

void
tbx_htable_add(p_tbx_htable_t  htable,
	       const char     *key,
	       void           *object);

void *
tbx_htable_get(const p_tbx_htable_t  htable,
	       const char           *key);

void *
tbx_htable_replace(p_tbx_htable_t  htable,
		   const char     *key,
		   void           *object);

void *
tbx_htable_extract(p_tbx_htable_t  htable,
		   const char     *key);

void
tbx_htable_free(p_tbx_htable_t htable);

void
tbx_htable_cleanup_and_free(p_tbx_htable_t htable);

void
tbx_htable_manager_exit(void);

p_tbx_slist_t
tbx_htable_get_key_slist(const p_tbx_htable_t htable);

void
tbx_htable_dump_keys(const p_tbx_htable_t htable);

void
tbx_htable_dump_keys_strvals(p_tbx_htable_t htable);

void
tbx_htable_dump_keys_ptrvals(p_tbx_htable_t htable);

/* @} */

/** \addtogroup string_interface
 *  @{
 */

/*
 * String management
 * -----------------
 */
void
tbx_string_manager_init(void);

void
tbx_string_manager_exit(void);

char *
tbx_strdup(const char *src);

tbx_bool_t
tbx_streq(const char *s1,
	  const char *s2);

p_tbx_string_t
tbx_string_init(void);

void
tbx_string_free(p_tbx_string_t string);

size_t
tbx_string_length(const p_tbx_string_t string);

void
tbx_string_reset(p_tbx_string_t string);

char *
tbx_string_to_cstring(const p_tbx_string_t string);

char *
tbx_string_to_cstring_and_free(p_tbx_string_t string);

void
tbx_string_set_to_cstring(p_tbx_string_t  string,
			   const char     *cstring);

void
tbx_string_set_to_cstring_and_free(p_tbx_string_t  string,
				    char           *cstring);

void
tbx_string_set_to_char(p_tbx_string_t string,
		       const int      data);

p_tbx_string_t
tbx_string_init_to_cstring(const char *cstring);

p_tbx_string_t
tbx_string_init_to_cstring_and_free(char *cstring);

p_tbx_string_t
tbx_string_init_to_char(const int data);

void
tbx_string_append_cstring(p_tbx_string_t  string,
			   const char     *cstring);

void
tbx_string_append_cstring_and_free(p_tbx_string_t  string,
				    char           *cstring);

void
tbx_string_append_char(p_tbx_string_t string,
		       const int      data);

void
tbx_string_append_int(p_tbx_string_t string,
		      const int      data);

void
tbx_string_set_to_int(p_tbx_string_t string,
		      const int      data);

p_tbx_string_t
tbx_string_init_to_int(const int data);

void
tbx_string_append_uint(p_tbx_string_t     string,
		       const unsigned int data);

void
tbx_string_set_to_uint(p_tbx_string_t     string,
		       const unsigned int data);

p_tbx_string_t
tbx_string_init_to_uint(const unsigned int data);

void
tbx_string_reverse(p_tbx_string_t string);

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

p_tbx_string_t
tbx_string_init_to_string(const p_tbx_string_t src_string);

p_tbx_string_t
tbx_string_dup(p_tbx_string_t src_string);

p_tbx_string_t
tbx_string_init_to_string_and_free(p_tbx_string_t src_string);

p_tbx_string_t
tbx_string_double_quote(const p_tbx_string_t src_string);

p_tbx_string_t
tbx_string_double_quote_and_free(p_tbx_string_t src_string);

p_tbx_string_t
tbx_string_single_quote(const p_tbx_string_t src_string);

p_tbx_string_t
tbx_string_single_quote_free(p_tbx_string_t src_string);

p_tbx_slist_t
tbx_string_split(const p_tbx_string_t  src_string,
		 const char           *IFS);

p_tbx_slist_t
tbx_string_split_and_free(p_tbx_string_t  src_string,
			  const char     *IFS);

p_tbx_string_t
tbx_string_extract_name_from_pathname(p_tbx_string_t path_name);

long
tbx_cstr_to_long(const char *s);

unsigned long
tbx_cstr_to_unsigned_long(const char *s);

/* @} */

/** \addtogroup darray_interface
 *  @{
 */
/*
 * Dynamic arrays management
 * ------------------------
 */
void
tbx_darray_manager_init(void);

void
tbx_darray_manager_exit(void);

static
__inline__
p_tbx_darray_t
tbx_darray_init(void);

static
__inline__
void
tbx_darray_free(p_tbx_darray_t darray);

static
__inline__
size_t
tbx_darray_length(p_tbx_darray_t darray);

static
__inline__
void
tbx_darray_reset(p_tbx_darray_t darray);

static
__inline__
void *
tbx_darray_get(p_tbx_darray_t     darray,
	       tbx_darray_index_t idx);

static
__inline__
void
tbx_darray_set(p_tbx_darray_t      darray,
	       tbx_darray_index_t  idx,
	       void               *object);

static
__inline__
void
tbx_darray_expand_and_set(p_tbx_darray_t      darray,
			  tbx_darray_index_t  idx,
			  void               *object);

static
__inline__
void *
tbx_darray_expand_and_get(p_tbx_darray_t     darray,
			  tbx_darray_index_t idx);

static
__inline__
void
tbx_darray_append(p_tbx_darray_t  darray,
		  void           *object);

static
__inline__
void *
tbx_darray_first_idx(p_tbx_darray_t       darray,
		     p_tbx_darray_index_t idx);

static
__inline__
void *
tbx_darray_next_idx(p_tbx_darray_t       darray,
		    p_tbx_darray_index_t idx);

/* @} */

/** \addtogroup param_interface
 *  @{
 */
/*
 * Parameter management
 * --------------------
 */
void
tbx_parameter_manager_init(void);

void
tbx_parameter_manager_exit(void);

/*  ... Argument option ................................................. // */
p_tbx_argument_option_t
tbx_argument_option_init_to_cstring_ext(const char *option,
					 const char  separator,
					 const char *value);

p_tbx_argument_option_t
tbx_argument_option_init_to_cstring(const char *option);

p_tbx_argument_option_t
tbx_argument_option_init_ext(const p_tbx_string_t option,
			     const char           separator,
			     const p_tbx_string_t value);

p_tbx_argument_option_t
tbx_argument_option_init(const p_tbx_string_t option);

p_tbx_argument_option_t
tbx_argument_option_dup(p_tbx_argument_option_t arg);

void
tbx_argument_option_free(p_tbx_argument_option_t arg);

p_tbx_string_t
tbx_argument_option_to_string(const p_tbx_argument_option_t arg);

/*  ... Arguments ....................................................... // */
p_tbx_arguments_t
tbx_arguments_init(void);

p_tbx_arguments_t
tbx_arguments_dup(p_tbx_arguments_t src_args);

void
tbx_arguments_append_arguments(p_tbx_arguments_t dst_args,
			       p_tbx_arguments_t src_args);

void
tbx_arguments_free(p_tbx_arguments_t args);

void
tbx_arguments_append_option(p_tbx_arguments_t             args,
			    const p_tbx_argument_option_t arg);

void
tbx_arguments_append_string_ext(p_tbx_arguments_t args,
				p_tbx_string_t    option,
				const char        separator,
				p_tbx_string_t    value);

void
tbx_arguments_append_string(p_tbx_arguments_t args,
				   p_tbx_string_t    option);

void
tbx_arguments_append_cstring_ext(p_tbx_arguments_t  args,
				  const char        *option,
				  const char         separator,
				  const char        *value);
void
tbx_arguments_append_cstring(p_tbx_arguments_t  args,
			      const char        *option);

p_tbx_string_t
tbx_arguments_to_string(p_tbx_arguments_t args,
			const int         IFS);

/*  ... Argument set .................................................... // */
p_tbx_argument_set_t
tbx_argument_set_build(const p_tbx_arguments_t args);

void
tbx_argument_set_free(p_tbx_argument_set_t args);

p_tbx_argument_set_t
tbx_argument_set_build_ext(const p_tbx_arguments_t args,
			   const p_tbx_string_t    command);

p_tbx_argument_set_t
tbx_argument_set_build_ext_c_command(const p_tbx_arguments_t  args,
				     const char              *command);

/*  ... Environment variable ............................................ // */
p_tbx_environment_variable_t
tbx_environment_variable_init(const p_tbx_string_t name,
			      const p_tbx_string_t value);

p_tbx_environment_variable_t
tbx_environment_variable_init_to_cstrings(const char *name,
					   const char *value);

p_tbx_environment_variable_t
tbx_environment_variable_to_variable(const char *name);

void
tbx_environment_variable_append_cstring(p_tbx_environment_variable_t  var,
					 const char                    sep,
					 const char                   *data);

void
tbx_environment_variable_free(p_tbx_environment_variable_t variable);

p_tbx_string_t
tbx_environment_variable_to_string(const
				   p_tbx_environment_variable_t variable);

void
tbx_environment_variable_set_to_string(p_tbx_environment_variable_t variable,
				       const p_tbx_string_t         value);

void
tbx_environment_variable_set_to_cstring(p_tbx_environment_variable_t variable,
					 const char                  *value);


/*  ... Environments .................................................... // */
p_tbx_environment_t
tbx_environment_init(void);

void
tbx_environment_free(p_tbx_environment_t env);

void
tbx_environment_append_variable(p_tbx_environment_t                env,
				const p_tbx_environment_variable_t var);

p_tbx_string_t
tbx_environment_to_string(const p_tbx_environment_t env);


/*  ... Command ......................................................... // */
p_tbx_command_t
tbx_command_init(const p_tbx_string_t command_name);

p_tbx_command_t
tbx_command_init_to_cstring(const char *command_name);

void
tbx_command_free(p_tbx_command_t command);

p_tbx_string_t
tbx_command_to_string(const p_tbx_command_t command);

p_tbx_argument_set_t
tbx_command_to_argument_set(const p_tbx_command_t command);

/* @} */

#endif /* __TBX_INTERFACE_H */
