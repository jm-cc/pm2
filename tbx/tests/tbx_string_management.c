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


#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "tbx.h"


static int check_strdup_eq(char *src)
{
	char *dst;

	dst = tbx_strdup(src);
	assert(dst && tbx_true == tbx_streq(src, dst));
	free(dst);
	return 1;
}


int main(int argc, char *argv[])
{
	p_tbx_string_t tbxstring, tbxstring2;
	p_tbx_slist_t  tbxsplit;
	char *cstring;

	tbx_init(&argc, &argv);

	printf("tbxstring: strdup/streq\n");	
	assert(check_strdup_eq("") && check_strdup_eq("abcd"));
	

	printf("tbxstring: empty\n");
	tbxstring = tbx_string_init();
	assert(0 == tbx_string_length(tbxstring));

	cstring = tbx_string_to_cstring(tbxstring);
	assert(! strcmp(cstring, "") && 0 == strlen(cstring));

	tbx_string_reverse(tbxstring);
	cstring = tbx_string_to_cstring(tbxstring);
	assert(! strcmp(cstring, "") && 0 == strlen(cstring));

	
	printf("tbxstring: a\n");
	tbx_string_append_cstring(tbxstring, "a");
	cstring = tbx_string_to_cstring(tbxstring);
	assert(1 == strlen(cstring) && ! strcmp(cstring, "a"));

	tbx_string_append_cstring(tbxstring, "");
	cstring = tbx_string_to_cstring(tbxstring);
	assert(1 == strlen(cstring) && ! strcmp(cstring, "a"));

	tbx_string_reverse(tbxstring);
	cstring = tbx_string_to_cstring(tbxstring);
	assert(! strcmp(cstring, "a") && 1 == strlen(cstring));
	
	
	printf("tbxstring: abcd\n");
	tbx_string_append_cstring(tbxstring, "bcd");
	cstring = tbx_string_to_cstring(tbxstring);
	tbx_string_set_to_cstring(tbxstring, "abcd");
	assert(! strcmp(cstring, tbx_string_to_cstring_and_free(tbxstring)));


	printf("tbxstring: -1\n");
	tbxstring = tbx_string_init_to_int(-1);
	assert(! strcmp("-1", tbx_string_to_cstring_and_free(tbxstring)));


	printf("tbxstring: 1\n");
	tbxstring = tbx_string_init_to_uint(1);
	assert(! strcmp("1", tbx_string_to_cstring(tbxstring)));


	printf("tbxstring: to_char VS to_cstring\n");
	tbx_string_free(tbxstring);
	assert(tbx_streq(tbx_string_to_cstring_and_free(tbx_string_init_to_cstring("a")),
			 tbx_string_to_cstring_and_free(tbx_string_init_to_char('a'))));

	printf("tbxstring: reverse\n");
	tbxstring = tbx_string_init_to_cstring("abcde");
	tbx_string_reverse(tbxstring);
	assert(tbx_streq(tbx_string_to_cstring_and_free(tbxstring), "edcba"));

	tbxstring = tbx_string_init_to_cstring("abcd");
	tbx_string_reverse(tbxstring);
	assert(tbx_streq(tbx_string_to_cstring_and_free(tbxstring), "dcba"));


	printf("tbxstring: init_to_string/append_string\n");
	tbxstring2 = tbx_string_init_to_cstring("a");
	tbxstring  = tbx_string_init_to_string_and_free(tbxstring2);
	tbxstring2 = tbx_string_init_to_string(tbxstring);
	tbx_string_append_string_and_free(tbxstring, tbxstring2);
	assert(tbx_streq(tbx_string_to_cstring_and_free(tbxstring), "aa"));

	tbxstring  = tbx_string_init();
	tbxstring2 = tbx_string_init();
	tbx_string_append_string_and_free(tbxstring, tbxstring2);
	assert(tbx_streq(tbx_string_to_cstring_and_free(tbxstring), ""));
	
	
	printf("tbxstring: double quotes\n");
	tbxstring  = tbx_string_init();
	tbxstring2 = tbx_string_double_quote_and_free(tbxstring);
	assert(tbx_streq(tbx_string_to_cstring_and_free(tbxstring2), "\"\"")); // print: ""

	tbxstring  = tbx_string_init_to_cstring("a\"b");
	tbxstring2 = tbx_string_double_quote_and_free(tbxstring);
	assert(tbx_streq(tbx_string_to_cstring_and_free(tbxstring2), "\"a\\\"b\"")); // print "a\"b"


	printf("tbxstring: simple quotes\n");
	tbxstring  = tbx_string_init();
	tbxstring2 = tbx_string_single_quote_and_free(tbxstring);
	assert(tbx_streq(tbx_string_to_cstring_and_free(tbxstring2), "''")); // print: ''

	tbxstring  = tbx_string_init_to_cstring("a'b");
	tbxstring2 = tbx_string_single_quote_and_free(tbxstring);
	assert(tbx_streq(tbx_string_to_cstring_and_free(tbxstring2), "'a\\\'b'")); // print "a\'b"
	
	
	printf("tbxstring: split\n");
	tbxstring = tbx_string_init_to_cstring("a,b\\,'c");
	tbxsplit  = tbx_string_split_and_free(tbxstring, ",");
	tbxstring = (typeof(tbxstring))tbx_slist_remove_from_head(tbxsplit);
	assert(tbx_streq(tbx_string_to_cstring_and_free(tbxstring), "a"));

	tbxstring = (typeof(tbxstring))tbx_slist_remove_from_head(tbxsplit);
	assert(tbx_streq(tbx_string_to_cstring_and_free(tbxstring), "b\\"));

	tbxstring = (typeof(tbxstring))tbx_slist_remove_from_head(tbxsplit);
	assert(tbx_streq(tbx_string_to_cstring_and_free(tbxstring), "'c"));

	tbxstring = tbx_string_init_to_cstring("a,'b,c'");
	tbxsplit  = tbx_string_split_and_free(tbxstring, ",");
	tbxstring = (typeof(tbxstring))tbx_slist_remove_from_head(tbxsplit);
	assert(tbx_streq(tbx_string_to_cstring_and_free(tbxstring), "a"));

	tbxstring = (typeof(tbxstring))tbx_slist_remove_from_head(tbxsplit);
	assert(tbx_streq(tbx_string_to_cstring_and_free(tbxstring), "'b,c'"));

	tbxstring = tbx_string_init_to_cstring("");
	tbxsplit  = tbx_string_split_and_free(tbxstring, ",");
	assert(tbx_slist_is_nil(tbxsplit));

		
	tbx_exit();

	return EXIT_SUCCESS;
}
