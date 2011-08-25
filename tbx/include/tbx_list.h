/*! \file tbx_list.h
 *  \brief TBX linked-list data structures
 *
 *  This file contains the TBX management data structures for the
 *  Madeleine-specialized linked list.
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
 * Tbx_list.h
 * ==========
 */

#ifndef TBX_LIST_H
#define TBX_LIST_H

/** \defgroup list_interface single-linked list interface
 *
 * This is the single-linked list interface
 *
 * @{
 */

typedef void (*p_tbx_list_foreach_func_t) (void *ptr);

typedef struct s_tbx_list_element {
	void *object;
	p_tbx_list_element_t next;
	p_tbx_list_element_t previous;
} tbx_list_element_t;

typedef struct s_tbx_list {
	tbx_list_length_t length;
	p_tbx_list_element_t first;
	p_tbx_list_element_t last;
	p_tbx_list_element_t mark;
	tbx_list_mark_position_t mark_position;
	tbx_bool_t mark_next;
	tbx_bool_t read_only;
} tbx_list_t;

/* list reference: 
 * allow additional cursor/mark over a given list
 * NOTE: the validity of cursor & mark element cannot
 *       be enforced by the list manager
 */
typedef struct s_tbx_list_reference {
	p_tbx_list_t list;
	p_tbx_list_element_t reference;
	tbx_bool_t after_end;
} tbx_list_reference_t;

/* @} */

#endif				/* TBX_LIST_H */
