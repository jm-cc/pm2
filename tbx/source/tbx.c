/*! \file tbx.c
 *  \brief TBX setup/cleanup routines
 *
 *  This file contains the TBX general management functions.
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
 * Tbx.c
 * =====
 */
#define   TBX_C
#include "tbx.h"
#include <assert.h>

// number of module using tbx
static volatile int initialized = 0;

int tbx_initialized(void)
{
	return initialized;
}

void tbx_purge_cmd_line()
{
	tbx_pa_free_module_args("tbx");
}

void tbx_init(int *argc, char ***argv)
{
	if (! argc || ! argv)
		return;

	if (! initialized) {
		/* Parser */
		tbx_pa_parse(*argc, *argv, PM2_ENVVARNAME);

		/* List manager */
		tbx_list_manager_init();

		/* Slist manager */
		tbx_slist_manager_init();

		/* Hash table manager */
		tbx_htable_manager_init();

		/* Dynamic array manager */
		tbx_darray_manager_init();

		/** String manager */
		tbx_string_manager_init();
	}

	int _argc = -1;
	char**_argv = NULL;
	tbx_pa_get_args(&_argc, &_argv);
	assert(_argc <= *argc);
	*argc = _argc;
	memcpy(*argv, _argv, (1 + _argc) * sizeof(char**));
	(*argv)[*argc] = NULL;
	initialized++;
}

void tbx_exit(void)
{
	int waiters;

	waiters = tbx_initialized();
	if (waiters) {
		if (1 == waiters) {
			/* Dynamic array manager */
			tbx_darray_manager_exit();

			/* Hash table manager */
			tbx_htable_manager_exit();

			/* Slist manager */
			tbx_slist_manager_exit();

			/* List manager */
			tbx_list_manager_exit();

			/** String manager */
			tbx_string_manager_exit();

			/* Parser */
			tbx_pa_free_args();
		}

		initialized-- ;
	}
}
