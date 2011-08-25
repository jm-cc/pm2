/*! \file tbx_string_management.c
 *  \brief TBX string object management routines.
 *
 *  This file contains the TBX string object management functions.
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
 * tbx_string_management.c
 * :::::::::::::::::::::::////////////////////////////////////////////////////
 */

#include "tbx.h"
#include <errno.h>
#include <string.h>
#include <limits.h>

/*
 *  Macros
 * ___________________________________________________________________________
 */
#define MAX_NUMBER_LENGTH 256

/*
 *  Functions
 * ___________________________________________________________________________
 */


/*
 * Initialization
 * --------------
 */
void tbx_string_manager_init(void)
{
	PM2_LOG_IN();
	PM2_LOG_OUT();
}

void tbx_string_manager_exit(void)
{
	PM2_LOG_IN();
	PM2_LOG_OUT();
}

char *tbx_strdup(const char *src)
{
	char *dst = NULL;
	size_t length = 0;

	PM2_LOG_IN();
	length = strlen(src);
	dst = TBX_MALLOC(length + 1);
	strcpy(dst, src);
	PM2_LOG_OUT();

	return dst;
}

tbx_bool_t tbx_streq(const char *s1, const char *s2)
{
	tbx_bool_t result = tbx_false;

	PM2_LOG_IN();
	if (!strcmp(s1, s2)) {
		result = tbx_true;
	}
	PM2_LOG_OUT();

	return result;
}

p_tbx_string_t tbx_string_init(void)
{
	p_tbx_string_t string = NULL;

	PM2_LOG_IN();
	string = TBX_CALLOC(1, sizeof(tbx_string_t));
	string->length = 0;
	string->allocated_length = 0;
	string->data = NULL;
	PM2_LOG_OUT();

	return string;
}

void tbx_string_free(p_tbx_string_t string)
{
	PM2_LOG_IN();
	tbx_string_reset(string);
	TBX_FREE(string);
	PM2_LOG_OUT();
}

size_t tbx_string_length(p_tbx_string_t string)
{
	size_t length = 0;

	PM2_LOG_IN();
	length = string->length;
	PM2_LOG_OUT();

	return length;
}

void tbx_string_reset(p_tbx_string_t string)
{
	PM2_LOG_IN();
	if (string->allocated_length) {
		TBX_FREE(string->data);
	}

	string->length = 0;
	string->allocated_length = 0;
	string->data = NULL;
	PM2_LOG_OUT();
}

char *tbx_string_to_cstring(p_tbx_string_t string)
{
	char *cstring = NULL;
	size_t length = 0;

	PM2_LOG_IN();
	length = string->length;

	cstring = TBX_MALLOC(length + 1);

	if (length) {
		memcpy(cstring, string->data, length);
	}

	cstring[length] = '\0';
	PM2_LOG_OUT();

	return cstring;
}

char *tbx_string_to_cstring_and_free(p_tbx_string_t string)
{
	char *cstring = NULL;

	PM2_LOG_IN();
	cstring = tbx_string_to_cstring(string);
	tbx_string_free(string);
	PM2_LOG_OUT();

	return cstring;
}

void tbx_string_set_to_cstring(p_tbx_string_t string, const char *cstring)
{
	size_t length = 0;

	PM2_LOG_IN();
	tbx_string_reset(string);
	if ((length = strlen(cstring))) {
		string->data = TBX_MALLOC(length);
		memcpy(string->data, cstring, length);
		string->length = length;
		string->allocated_length = length;
	}
	PM2_LOG_OUT();
}

void
tbx_string_set_to_cstring_and_free(p_tbx_string_t string, char *cstring)
{
	PM2_LOG_IN();
	tbx_string_set_to_cstring(string, cstring);
	TBX_FREE(cstring);
	PM2_LOG_OUT();
}

void tbx_string_set_to_char(p_tbx_string_t string, int data)
{
	PM2_LOG_IN();
	tbx_string_reset(string);
	string->data = TBX_MALLOC(1);
	string->data[0] = (char)data;
	string->length = 1;
	string->allocated_length = 1;
	PM2_LOG_OUT();
}

p_tbx_string_t tbx_string_init_to_cstring(const char *cstring)
{
	p_tbx_string_t string = NULL;

	PM2_LOG_IN();
	string = tbx_string_init();
	tbx_string_set_to_cstring(string, cstring);
	PM2_LOG_OUT();

	return string;
}

p_tbx_string_t tbx_string_init_to_cstring_and_free(char *cstring)
{
	p_tbx_string_t string = NULL;

	PM2_LOG_IN();
	string = tbx_string_init();
	tbx_string_set_to_cstring_and_free(string, cstring);
	PM2_LOG_OUT();

	return string;
}

p_tbx_string_t tbx_string_init_to_char(const int data)
{
	p_tbx_string_t string = NULL;

	PM2_LOG_IN();
	string = tbx_string_init();
	tbx_string_set_to_char(string, data);
	PM2_LOG_OUT();

	return string;
}

void tbx_string_append_cstring(p_tbx_string_t string, const char *cstring)
{
	PM2_LOG_IN();
	if (string->allocated_length) {
		size_t length = 0;

		if ((length = strlen(cstring))) {
			if (string->length + length >
			    string->allocated_length) {
				string->allocated_length =
				    tbx_max(2 *
					    (string->allocated_length + 1),
					    string->allocated_length +
					    length);
				string->data =
				    TBX_REALLOC(string->data,
						string->allocated_length);
			}

			memcpy(string->data + string->length, cstring,
			       length);
			string->length += length;
		}
	} else {
		tbx_string_set_to_cstring(string, cstring);
	}
	PM2_LOG_OUT();
}

void
tbx_string_append_cstring_and_free(p_tbx_string_t string, char *cstring)
{
	PM2_LOG_IN();
	tbx_string_append_cstring(string, cstring);
	TBX_FREE(cstring);
	PM2_LOG_OUT();
}

void tbx_string_append_char(p_tbx_string_t string, int data)
{
	PM2_LOG_IN();
	if (string->allocated_length) {
		string->length++;
		if (string->length > string->allocated_length) {
			string->allocated_length =
			    2 * (string->allocated_length + 1);
			string->data =
			    TBX_REALLOC(string->data,
					string->allocated_length);
		}

		string->data[string->length - 1] = (char) data;
	} else {
		tbx_string_set_to_char(string, data);
	}
	PM2_LOG_OUT();
}

void tbx_string_append_int(p_tbx_string_t string, int data)
{
	char temp[MAX_NUMBER_LENGTH + 1];
	size_t offset = 0;
	tbx_bool_t neg = (data < 0) ? tbx_true : tbx_false;

	PM2_LOG_IN();
	if (neg) {
		data = -data;
	}

	do {
		temp[offset++] = (char)('0' + data % 10);
		data /= 10;
	}
	while (data);

	if (neg) {
		temp[offset++] = '-';
	}

	do {
		tbx_string_append_char(string, temp[--offset]);
	}
	while (offset);
	PM2_LOG_OUT();
}

void tbx_string_set_to_int(p_tbx_string_t string, int data)
{
	PM2_LOG_IN();
	tbx_string_reset(string);
	tbx_string_append_int(string, data);
	PM2_LOG_OUT();
}

p_tbx_string_t tbx_string_init_to_int(int data)
{
	p_tbx_string_t string = NULL;

	PM2_LOG_IN();
	string = tbx_string_init();
	tbx_string_set_to_int(string, data);
	PM2_LOG_OUT();

	return string;
}

void tbx_string_append_uint(p_tbx_string_t string, unsigned int data)
{
	char temp[MAX_NUMBER_LENGTH + 1];
	size_t offset = 0;

	PM2_LOG_IN();
	do {
		temp[offset++] = (char)('0' + data % 10);
		data /= 10;
	}
	while (data);

	do {
		tbx_string_append_char(string, temp[--offset]);
	}
	while (offset);
	PM2_LOG_OUT();
}

void tbx_string_set_to_uint(p_tbx_string_t string, unsigned int data)
{
	PM2_LOG_IN();
	tbx_string_reset(string);
	tbx_string_append_uint(string, data);
	PM2_LOG_OUT();
}

p_tbx_string_t tbx_string_init_to_uint(unsigned int data)
{
	p_tbx_string_t string = NULL;

	PM2_LOG_IN();
	string = tbx_string_init();
	tbx_string_set_to_uint(string, data);
	PM2_LOG_OUT();

	return string;
}

void tbx_string_reverse(p_tbx_string_t string)
{
	size_t offset_left = 0;
	size_t offset_right = string->length - 1;

	PM2_LOG_IN();
	while (offset_left < offset_right) {
		string->data[offset_left] = (char)(string->data[offset_left] ^ string->data[offset_right]);
		string->data[offset_right] = (char)(string->data[offset_left] ^ string->data[offset_right]);
		string->data[offset_left] = (char)(string->data[offset_left] ^ string->data[offset_right]);
		offset_left++;
		offset_right--;
	}
	PM2_LOG_OUT();
}

void
tbx_string_set_to_string(p_tbx_string_t dst_string,
			 p_tbx_string_t src_string)
{
	PM2_LOG_IN();
	tbx_string_reset(dst_string);
	if (src_string->length) {
		dst_string->data = TBX_MALLOC(src_string->length);
		memcpy(dst_string->data, src_string->data,
		       src_string->length);
		dst_string->length = src_string->length;
		dst_string->allocated_length = src_string->length;
	}
	PM2_LOG_OUT();
}

void
tbx_string_set_to_string_and_free(p_tbx_string_t dst_string,
				  p_tbx_string_t src_string)
{
	PM2_LOG_IN();
	tbx_string_set_to_string(dst_string, src_string);
	tbx_string_free(src_string);
	PM2_LOG_OUT();
}

void
tbx_string_append_string(p_tbx_string_t dst_string,
			 p_tbx_string_t src_string)
{
	PM2_LOG_IN();
	if (src_string->length) {
		if (dst_string->allocated_length) {
			if (dst_string->length + src_string->length >
			    dst_string->allocated_length) {
				dst_string->allocated_length =
				    tbx_max(2 *
					    (dst_string->allocated_length +
					     1),
					    dst_string->allocated_length +
					    src_string->length);
				dst_string->data =
				    TBX_REALLOC(dst_string->data,
						dst_string->
						allocated_length);
			}

			memcpy(dst_string->data + dst_string->length,
			       src_string->data, src_string->length);
			dst_string->length += src_string->length;
		} else {
			tbx_string_set_to_string(dst_string, src_string);
		}
	}
	PM2_LOG_OUT();
}

void
tbx_string_append_string_and_free(p_tbx_string_t dst_string,
				  p_tbx_string_t src_string)
{
	PM2_LOG_IN();
	tbx_string_append_string(dst_string, src_string);
	tbx_string_free(src_string);
	PM2_LOG_OUT();
}

p_tbx_string_t tbx_string_init_to_string(p_tbx_string_t src_string)
{
	p_tbx_string_t dst_string = NULL;

	PM2_LOG_IN();
	dst_string = tbx_string_init();
	tbx_string_set_to_string(dst_string, src_string);
	PM2_LOG_OUT();

	return dst_string;
}

p_tbx_string_t tbx_string_dup(p_tbx_string_t src_string)
{
	p_tbx_string_t dst_string = NULL;

	PM2_LOG_IN();
	dst_string = tbx_string_init_to_string(src_string);
	PM2_LOG_OUT();

	return dst_string;
}

p_tbx_string_t
tbx_string_init_to_string_and_free(p_tbx_string_t src_string)
{
	p_tbx_string_t dst_string = NULL;

	PM2_LOG_IN();
	dst_string = tbx_string_init_to_string(src_string);
	tbx_string_free(src_string);
	PM2_LOG_OUT();

	return dst_string;
}

p_tbx_string_t tbx_string_double_quote(p_tbx_string_t src_string)
{
	char *src_data = src_string->data;
	size_t src_length = src_string->length;
	p_tbx_string_t dst_string = NULL;
	size_t offset = 0;

	PM2_LOG_IN();
	// Leading double quote
	dst_string = tbx_string_init_to_char('"');

	while (offset < src_length) {
		switch (src_data[offset]) {
		case '\\':
		case '"':
			{
				tbx_string_append_char(dst_string, '\\');
			}
			break;
		default:
			{
				tbx_string_append_char(dst_string,
						       src_data[offset++]);
			}
			break;
		}
	}


	// Trailing double quote
	tbx_string_append_char(dst_string, '"');
	PM2_LOG_OUT();

	return dst_string;
}

p_tbx_string_t tbx_string_double_quote_and_free(p_tbx_string_t src_string)
{
	p_tbx_string_t dst_string = NULL;

	PM2_LOG_IN();
	dst_string = tbx_string_double_quote(src_string);
	tbx_string_free(src_string);
	PM2_LOG_OUT();

	return dst_string;
}

p_tbx_string_t tbx_string_single_quote(p_tbx_string_t src_string)
{
	p_tbx_string_t dst_string = NULL;

	PM2_LOG_IN();
	dst_string = tbx_string_init_to_char('\'');
	tbx_string_append_string(dst_string, src_string);
	tbx_string_append_char(dst_string, '\'');
	PM2_LOG_OUT();

	return dst_string;
}

p_tbx_string_t tbx_string_single_quote_free(p_tbx_string_t src_string)
{
	p_tbx_string_t dst_string = NULL;

	PM2_LOG_IN();
	dst_string = tbx_string_single_quote(src_string);
	tbx_string_free(src_string);
	PM2_LOG_OUT();

	return dst_string;
}

p_tbx_slist_t tbx_string_split(p_tbx_string_t src_string, const char *IFS)
{
	p_tbx_slist_t slist = NULL;

	slist = tbx_slist_nil();

	PM2_LOG_IN();
	if (src_string->length) {
		p_tbx_string_t dst_string = NULL;
		char *data = src_string->data;
		size_t length = src_string->length;
		size_t offset = 0;
		tbx_bool_t single_mode = tbx_false;
		tbx_bool_t double_mode = tbx_false;
		tbx_bool_t copy_mode = tbx_false;
		tbx_bool_t quoting_mode = tbx_false;

		IFS = (IFS) ? : " \t\n";

		do {
			if (copy_mode) {
				if (!single_mode && !double_mode && !quoting_mode) {
					const char *ptr = IFS;

					while (*ptr) {
						if (*ptr == data[offset]) {
							offset++;
							copy_mode = tbx_false;

							tbx_slist_append(slist,
									 dst_string);
							dst_string = NULL;

							goto next_1;
						}

						ptr++;
					}
				}

				switch (data[offset]) {
				case '\\':
					{
						if (!single_mode) {
							quoting_mode = (tbx_bool_t)!quoting_mode;
						}
					}
					break;
				case '\'':
					{
						if (!quoting_mode && !double_mode) {
							single_mode = (tbx_bool_t)!single_mode;
						}
					}
					break;
				case '"':
					{
						if (!quoting_mode && !single_mode) {
							double_mode = (tbx_bool_t)!double_mode;
						}
					}
					break;
				default:
					{
						/* Nothing */
					}
					break;
				}

				tbx_string_append_char(dst_string,
						       data[offset++]);
			} else {
				const char *ptr = IFS;

				while (*ptr) {
					if (*ptr == data[offset]) {
						offset++;
						goto next_1;
					}

					ptr++;
				}

				dst_string = tbx_string_init();
				copy_mode = tbx_true;
			}
		      next_1:
			;
		}
		while (offset < length);

		if (dst_string) {
			tbx_slist_append(slist, dst_string);
			dst_string = NULL;
		}
	}
	PM2_LOG_OUT();

	return slist;
}

p_tbx_slist_t
tbx_string_split_and_free(p_tbx_string_t src_string, const char *IFS)
{
	p_tbx_slist_t slist = NULL;

	PM2_LOG_IN();
	slist = tbx_string_split(src_string, IFS);
	tbx_string_free(src_string);
	PM2_LOG_OUT();

	return slist;
}

p_tbx_string_t
tbx_string_extract_name_from_pathname(p_tbx_string_t path_name)
{
	p_tbx_string_t name = NULL;
	size_t idx = 0;

	PM2_LOG_IN();
	name = tbx_string_init();
	idx = path_name->length;

	if (idx) {
		while ((idx > 0) && (path_name->data[idx - 1] != '\\')) {
			idx--;
		}

		name->length = path_name->length - idx;
		if (name->length) {
			name->allocated_length = name->length;
			name->data = TBX_MALLOC(name->allocated_length);

			memcpy(name->data, path_name->data + idx,
			       name->length);

			path_name->length = (idx > 0) ? idx - 1 : idx;

			if (!path_name->length) {
				TBX_FREE(path_name->data);
				path_name->data = NULL;
				path_name->allocated_length = 0;
			}
		}
	}
	PM2_LOG_OUT();

	return name;
}

long tbx_cstr_to_long(const char *s)
{
	long val = 0;
	char *p = NULL;

	PM2_LOG_IN();
	val = strtol(s, &p, 0);

	if (p == s)
		TBX_FAILUREF("failed to convert string '%s' to <long>", s);

	if (val == LONG_MIN && errno == ERANGE)
		TBX_FAILUREF
		    ("underflow error while converting string '%s' to <long>",
		     s);

	if (val == LONG_MAX && errno == ERANGE)
		TBX_FAILUREF
		    ("overflow error while converting string '%s' to <long>",
		     s);

	PM2_LOG_OUT();

	return val;
}

unsigned long tbx_cstr_to_unsigned_long(const char *s)
{
	unsigned long val = 0;
	char *p = NULL;

	PM2_LOG_IN();
	val = strtoul(s, &p, 0);

	if (p == s)
		TBX_FAILUREF("failed to convert string '%s' to <long>", s);

	if (val == LONG_MAX && errno == ERANGE)
		TBX_FAILUREF
		    ("overflow error while converting string '%s' to <long>",
		     s);

	PM2_LOG_OUT();

	return val;
}
