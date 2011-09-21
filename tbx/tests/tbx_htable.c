/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2011 "the PM2 team" (see AUTHORS file)
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


#include <stdlib.h>
#include "tbx.h"


int main(int argc, char *argv[])
{
	p_tbx_htable_t table;
	p_tbx_slist_t  keylist;
	char *a = "a";
	char *b = "b";
	

	tbx_init(&argc, &argv);

	/** check empty table state */
	table = tbx_htable_empty_table();
	if (tbx_false == tbx_htable_empty(table) || 
	    0 != tbx_htable_get_size(table))
		return EXIT_FAILURE;

	/** check key list of empty table */
	keylist = tbx_htable_get_key_slist(table);
	if (tbx_false == tbx_slist_is_nil(keylist))
		return EXIT_FAILURE;
	tbx_slist_clear_and_free(keylist);

	/** check replace with add */
	tbx_htable_add(table, a, a);
	tbx_htable_add(table, b, b);
	tbx_htable_add(table, b, a);
	if (2 != tbx_htable_get_size(table) ||
	    b != tbx_htable_get(table, b) ||
	    tbx_true == tbx_htable_empty(table))
		return EXIT_FAILURE;

	/** check key list */
	keylist = tbx_htable_get_key_slist(table);
	if (! strcmp(a, tbx_slist_extract(keylist)))
		 return EXIT_FAILURE;
	if (! strcmp(b, tbx_slist_extract(keylist)))
		return EXIT_FAILURE;
	if (tbx_false == tbx_slist_is_nil(keylist))
		return EXIT_FAILURE;
	tbx_slist_clear_and_free(keylist);

	/** extract all elements */
	tbx_htable_extract(table, a);
	tbx_htable_extract(table, b);
	if (0 != tbx_htable_get_size(table) ||
	    tbx_false == tbx_htable_empty(table) ||
	    NULL != tbx_htable_get(table, a))
		return EXIT_FAILURE;

	tbx_htable_free(table);
	
	tbx_exit();

	return EXIT_SUCCESS;
}
