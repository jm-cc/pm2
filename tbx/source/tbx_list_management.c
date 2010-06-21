/*! \file tbx_list_management.c
 *  \brief TBX linked-list management routines
 *
 *  This file contains the TBX management functions for the
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
 * tbx_list_management.c
 * ---------------------
 */

#include "tbx.h"

/* MACROS */
#define INITIAL_LIST_ELEMENT 256

/* data structures */
p_tbx_memory_t tbx_list_manager_memory = NULL;

/* functions */

/*
 * Initialization
 * --------------
 */
void tbx_list_manager_init(void)
{
	tbx_malloc_init(&tbx_list_manager_memory,
			sizeof(tbx_list_element_t),
			INITIAL_LIST_ELEMENT, "tbx/list elements");
}

/*
 * Destruction
 * --------------
 */
void tbx_list_manager_exit(void)
{
	tbx_malloc_clean(tbx_list_manager_memory);
}
