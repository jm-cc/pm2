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
#include <stdio.h>
#include "tbx.h"


static int check_strdup_eq(char *src)
{
	char *dst;

	dst = tbx_strdup(src);
	if (! dst || tbx_false == tbx_streq(src, dst))
		return 0;

	free(dst);
	return 1;
}


int main()
{
	p_tbx_string_t tbxstring;
	char *cstring;

	printf("strdup/streq\n");	
	if (! check_strdup_eq("") || ! check_strdup_eq("abcd"))
		return EXIT_FAILURE;
	

	printf("0-length tbxstring\n");
	tbxstring = tbx_string_init();
	if (0 != tbx_string_length(tbxstring))
		return EXIT_FAILURE;

	cstring = tbx_string_to_cstring(tbxstring);
	if (strcmp(cstring, "") || strlen(cstring))
		return EXIT_FAILURE;

	tbx_string_reverse(tbxstring);
	cstring = tbx_string_to_cstring(tbxstring);
	if (strcmp(cstring, "") || strlen(cstring))
	return EXIT_FAILURE;

	
	printf("tbxstring: a\n");
	tbx_string_append_cstring(tbxstring, "a");
	cstring = tbx_string_to_cstring(tbxstring);
	if (1 != strlen(cstring) || strcmp(cstring, "a"))
		return EXIT_FAILURE;

	tbx_string_append_cstring(tbxstring, "");
	cstring = tbx_string_to_cstring(tbxstring);
	if (1 != strlen(cstring) || strcmp(cstring, "a"))
		return EXIT_FAILURE;

	tbx_string_reverse(tbxstring);
	cstring = tbx_string_to_cstring(tbxstring);
	if (strcmp(cstring, "a") || 1 != strlen(cstring))
		return EXIT_FAILURE;
	
	
	printf("tbxstring: abcd\n");
	tbx_string_append_cstring(tbxstring, "bcd");
	cstring = tbx_string_to_cstring(tbxstring);
	tbx_string_set_to_cstring(tbxstring, "abcd");
	if (strcmp(cstring, tbx_string_to_cstring(tbxstring)))
		return EXIT_FAILURE;


	printf("tbxstring: -1\n");
	tbx_string_set_to_int(tbxstring, -1);
	if (strcmp("-1", tbx_string_to_cstring(tbxstring)))
		return EXIT_FAILURE;


	printf("tbxstring: 1\n");
	tbx_string_set_to_uint(tbxstring, 1);
	if (strcmp("1", tbx_string_to_cstring(tbxstring)))
		return EXIT_FAILURE;

	tbx_string_free(tbxstring);

	printf("tbxstring: to_char VS to_cstring\n");
	if (! tbx_streq(tbx_string_to_cstring_and_free(tbx_string_init_to_cstring("a")),
			tbx_string_to_cstring_and_free(tbx_string_init_to_char('a'))))
		return EXIT_FAILURE;

	printf("tbxstring: reverse\n");
	tbxstring = tbx_string_init_to_cstring("abcde");
	tbx_string_reverse(tbxstring);
	if (! tbx_streq(tbx_string_to_cstring_and_free(tbxstring), "edcba"))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
