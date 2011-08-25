/*! \file tbx_slist_management.c
 *  \brief TBX double-linked search list management routines
 *
 *  This file contains the TBX double-linked multi-purpose search list
 *  management functions.
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
 * tbx_slist_management.c
 * :::::::::::::::::::::://///////////////////////////////////////////////////
 */

#include "tbx.h"

/*
 *  Macros
 * ___________________________________________________________________________
 */
#define INITIAL_SLIST_ELEMENT 256
#define INITIAL_SLIST_NREF    64
#define INITIAL_SLIST_NUMBER 64

/*
 * data structures
 * ___________________________________________________________________________
 */
p_tbx_memory_t tbx_slist_element_manager_memory = NULL;
p_tbx_memory_t tbx_slist_nref_manager_memory = NULL;
p_tbx_memory_t tbx_slist_struct_manager_memory = NULL;

/*
 *  Functions
 * ___________________________________________________________________________
 */


/*
 * Initialization
 * --------------
 */
void tbx_slist_manager_init(void)
{
	PM2_LOG_IN();
	tbx_malloc_extended_init(&tbx_slist_element_manager_memory,
				 sizeof(tbx_slist_element_t),
				 INITIAL_SLIST_ELEMENT,
				 "tbx/slist elements", 1);

	tbx_malloc_init(&tbx_slist_nref_manager_memory,
			sizeof(tbx_slist_nref_t),
			INITIAL_SLIST_NREF, "tbx/slist nrefs");
	tbx_malloc_init(&tbx_slist_struct_manager_memory,
			sizeof(tbx_slist_t),
			INITIAL_SLIST_NUMBER, "tbx/slists");
	PM2_LOG_OUT();
}

void tbx_slist_manager_exit(void)
{
	PM2_LOG_IN();
	tbx_malloc_clean(tbx_slist_element_manager_memory);
	tbx_malloc_clean(tbx_slist_nref_manager_memory);
	tbx_malloc_clean(tbx_slist_struct_manager_memory);
	PM2_LOG_OUT();
}
