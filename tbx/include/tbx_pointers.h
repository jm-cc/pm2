/*! \file tbx_pointers.h
 *  \brief TBX pointer types definition
 *
 *  This file defines the pointer version of any TBX data structure
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
 * Tbx_pointers.h
 * ==============
 */

#ifndef TBX_POINTERS_H
#define TBX_POINTERS_H

typedef struct s_tbx_list_element *p_tbx_list_element_t;
typedef struct s_tbx_list *p_tbx_list_t;
typedef struct s_tbx_list_reference *p_tbx_list_reference_t;
typedef struct s_tbx_slist_nref *p_tbx_slist_nref_t;
typedef struct s_tbx_slist_element *p_tbx_slist_element_t;
typedef struct s_tbx_slist *p_tbx_slist_t;
typedef struct s_tbx_memory *p_tbx_memory_t;
typedef struct s_tbx_htable_element *p_tbx_htable_element_t;
typedef struct s_tbx_htable *p_tbx_htable_t;
typedef struct s_tbx_string *p_tbx_string_t;
typedef struct s_tbx_darray *p_tbx_darray_t;
typedef struct s_tbx_argument_option *p_tbx_argument_option_t;
typedef struct s_tbx_arguments *p_tbx_arguments_t;
typedef struct s_tbx_argument_set *p_tbx_argument_set_t;
typedef struct s_tbx_environment_variable *p_tbx_environment_variable_t;
typedef struct s_tbx_environment *p_tbx_environment_t;
typedef struct s_tbx_command *p_tbx_command_t;

#endif				/* TBX_POINTERS_H */
