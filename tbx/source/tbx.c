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

// number of module using tbx
static volatile int initialized = 0;

int tbx_initialized(void)
{
	return initialized;
}

void
tbx_dump(unsigned char *p,
         size_t	n) {
        int		c = 0;
        size_t	l = 0;

	while (n) {
		if (!c) {
			PM2_DISP("%016zx ", 16 * l);
		}

		PM2_DISP("%02x ", *p++);

		c++;
		if (c == 8) {
			PM2_DISP(" ");
		} else if (c == 16) {
			l++;
			c = 0;
			PM2_DISP("\n");
			if (l >= 4) {
				PM2_DISP("...\n");
				break;
			}
		} else if (n == 1) {
			PM2_DISP("\n");
		}

		n--;
	}
}

void tbx_purge_cmd_line(void)
{
	tbx_pa_free_module_args("tbx");
}

void tbx_init(int *argc, char ***argv)
{
	if (! initialized) {
		/* Parser */
		tbx_pa_parse(*argc, *argv, PM2_ENVVARNAME);

		/* Timer */
		tbx_timing_init();

		/* List manager */
		tbx_list_manager_init();

		/* Slist manager */
		tbx_slist_manager_init();

		/* Hash table manager */
		tbx_htable_manager_init();

		/* String manager */
		tbx_string_manager_init();

		/* Dynamic array manager */
		tbx_darray_manager_init();
	}

	tbx_pa_get_args(argc, argv);
	initialized++;
}

void tbx_exit(void)
{
	if (initialized) {
		if (1 == initialized) {
			/* Dynamic array manager */
			tbx_darray_manager_exit();

			/* String manager */
			tbx_string_manager_exit();

			/* Hash table manager */
			tbx_htable_manager_exit();

			/* Slist manager */
			tbx_slist_manager_exit();

			/* List manager */
			tbx_list_manager_exit();

			/* Timer */
			tbx_timing_exit();

			/* Parser */
			tbx_pa_free_args();
		}

		initialized-- ;
	}
}
