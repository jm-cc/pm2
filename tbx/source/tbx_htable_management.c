/*! \file tbx_htable_management.c
 *  \brief TBX hash-table management routines
 *
 *  This file implements hash-table TBX associative arrays.
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
 * tbx_htable_management.c
 * -----------------------
 */

#include "tbx.h"

/*
 * Macros
 * ------
 */
#define INITIAL_HTABLE_ELEMENT 256
#define DEFAULT_BUCKET_COUNT    17

/*
 * Data structures
 * ---------------
 */
static p_tbx_memory_t tbx_htable_manager_memory = NULL;

/*
 * Functions
 * =========
 */

/*
 * Initialization
 * --------------
 */
void tbx_htable_manager_init(void)
{
	PM2_LOG_IN();
	tbx_malloc_init(&tbx_htable_manager_memory,
			sizeof(tbx_htable_element_t),
			INITIAL_HTABLE_ELEMENT, "tbx/htable elements");
	PM2_LOG_OUT();
}

void
tbx_htable_init(p_tbx_htable_t htable, tbx_htable_bucket_count_t buckets)
{
	PM2_LOG_IN();
	if (!buckets) {
		buckets = DEFAULT_BUCKET_COUNT;
	}

	htable->nb_bucket = buckets;
	htable->bucket_array =
	    TBX_MALLOC(buckets * sizeof(tbx_htable_element_t));
	htable->nb_element = 0;

	while (buckets--) {
		htable->bucket_array[buckets] = NULL;
	}
	PM2_LOG_OUT();
}

p_tbx_htable_t tbx_htable_empty_table(void)
{
	p_tbx_htable_t table = NULL;

	PM2_LOG_IN();
	table = TBX_CALLOC(1, sizeof(tbx_htable_t));
	CTRL_ALLOC(table);

	tbx_htable_init(table, 0);
	PM2_LOG_OUT();

	return table;
}

tbx_bool_t tbx_htable_empty(p_tbx_htable_t htable)
{
	tbx_bool_t test;

	PM2_LOG_IN();
	test = (htable->nb_element == 0) ? tbx_true : tbx_false;
	PM2_LOG_OUT();

	return test;
}

tbx_htable_element_count_t tbx_htable_get_size(p_tbx_htable_t htable)
{
	tbx_htable_element_count_t count = -1;

	PM2_LOG_IN();
	count = htable->nb_element;
	PM2_LOG_OUT();

	return count;
}

static
    tbx_htable_bucket_count_t
__tbx_htable_get_bucket(p_tbx_htable_t htable,
			const char *key, size_t key_len)
{
	tbx_htable_bucket_count_t bucket = 0;

	PM2_LOG_IN();
	while (key_len--) {
		bucket += key[key_len];
	}

	bucket %= htable->nb_bucket;
	PM2_LOG_OUT();

	return bucket;
}

void
tbx_htable_add_ext(p_tbx_htable_t htable,
		   const char *key, const size_t key_len, void *object)
{
	tbx_htable_bucket_count_t bucket = -1;
	p_tbx_htable_element_t element = NULL;

	PM2_LOG_IN();
	bucket = __tbx_htable_get_bucket(htable, key, key_len);
	element = tbx_malloc(tbx_htable_manager_memory);
	element->key = TBX_MALLOC(key_len);
	memcpy(element->key, key, key_len);
	element->key_len = key_len;
	element->object = object;
	element->next = htable->bucket_array[bucket];

	htable->bucket_array[bucket] = element;
	htable->nb_element++;
	PM2_LOG_OUT();
}

void tbx_htable_add(p_tbx_htable_t htable, const char *key, void *object)
{
	tbx_htable_add_ext(htable, key, strlen(key) + 1, object);
}

static
    p_tbx_htable_element_t
__tbx_htable_get_element(p_tbx_htable_t htable,
			 const char *key, const size_t key_len)
{
	tbx_htable_bucket_count_t bucket = -1;
	p_tbx_htable_element_t element = NULL;

	PM2_LOG_IN();
	bucket = __tbx_htable_get_bucket(htable, key, key_len);
	element = htable->bucket_array[bucket];

	while (element) {
		if (element->key_len != key_len)
			goto next;

		if (!memcmp(key, element->key, key_len))
			goto end;

	      next:
		element = element->next;
	}

      end:
	PM2_LOG_OUT();

	return element;
}

void *tbx_htable_get_ext(p_tbx_htable_t htable,
			 const char *key, const size_t key_len)
{
	p_tbx_htable_element_t element = NULL;
	void *result = NULL;

	PM2_LOG_IN();

	PM2_LOG("tbx_htable_get: key = <%s>", key);

	element = __tbx_htable_get_element(htable, key, key_len);

	if (element) {
		result = element->object;
	}
	PM2_LOG_OUT();

	return result;
}

void *tbx_htable_get(p_tbx_htable_t htable, const char *key)
{
	return tbx_htable_get_ext(htable, key, strlen(key) + 1);
}

void *tbx_htable_replace_ext(p_tbx_htable_t htable,
			     const char *key,
			     const size_t key_len, void *object)
{
	p_tbx_htable_element_t element = NULL;
	void *result = NULL;

	PM2_LOG_IN();
	element = __tbx_htable_get_element(htable, key, key_len);

	if (element) {
		result = element->object;
		element->object = object;
	} else {
		tbx_htable_add(htable, key, object);
	}

	PM2_LOG_OUT();

	return result;
}

void *tbx_htable_replace(p_tbx_htable_t htable,
			 const char *key, void *object)
{
	return tbx_htable_replace_ext(htable, key, strlen(key) + 1,
				      object);
}

void *tbx_htable_extract_ext(p_tbx_htable_t htable,
			     const char *key, const size_t key_len)
{
	tbx_htable_bucket_count_t bucket = -1;
	p_tbx_htable_element_t *element = NULL;

	PM2_LOG_IN();
	bucket = __tbx_htable_get_bucket(htable, key, key_len);
	element = &(htable->bucket_array[bucket]);

	while (*element) {
		if ((*element)->key_len != key_len
		    || memcmp(key, (*element)->key, key_len)) {
			element = &((*element)->next);
		} else {
			p_tbx_htable_element_t tmp_element = NULL;
			void *object = NULL;

			tmp_element = *element;

			object = tmp_element->object;
			tmp_element->object = NULL;

			*element = tmp_element->next;
			tmp_element->next = NULL;

			TBX_FREE(tmp_element->key);
			tmp_element->key = NULL;
			tmp_element->key_len = 0;

			tbx_free(tbx_htable_manager_memory, tmp_element);

			htable->nb_element--;

			PM2_LOG_OUT();
			return object;
		}
	}

	PM2_LOG_OUT();
	return NULL;
}

void *tbx_htable_extract(p_tbx_htable_t htable, const char *key)
{
	return tbx_htable_extract_ext(htable, key, strlen(key) + 1);
}

void tbx_htable_free(p_tbx_htable_t htable)
{
	PM2_LOG_IN();
	if (htable->nb_element)
		TBX_FAILURE("htable is not empty");

	TBX_FREE(htable->bucket_array);
	htable->bucket_array = NULL;
	htable->nb_bucket = 0;
	TBX_FREE(htable);
	PM2_LOG_OUT();
}

void tbx_htable_cleanup_and_free(p_tbx_htable_t htable)
{
	PM2_LOG_IN();
	if (htable->nb_element) {
		while (htable->nb_bucket--) {
			p_tbx_htable_element_t element = NULL;

			element = htable->bucket_array[htable->nb_bucket];

			while (element) {
				p_tbx_htable_element_t temp = NULL;

				temp = element->next;

				TBX_FREE(element->key);
				element->key = NULL;
				element->object = NULL;
				element->next = NULL;

				tbx_free(tbx_htable_manager_memory,
					 element);

				htable->nb_element--;
				element = temp;
			}

			htable->bucket_array[htable->nb_bucket] = NULL;
		}
	}

	if (htable->nb_element)
		TBX_FAILURE("inconsistent htable state");

	TBX_FREE(htable->bucket_array);
	htable->bucket_array = NULL;
	TBX_FREE(htable);
	PM2_LOG_OUT();
}

void tbx_htable_manager_exit(void)
{
	PM2_LOG_IN();
	tbx_malloc_clean(tbx_htable_manager_memory);
	PM2_LOG_OUT();
}

p_tbx_slist_t tbx_htable_get_key_slist(p_tbx_htable_t htable)
{
	p_tbx_slist_t slist = NULL;

	PM2_LOG_IN();
	slist = tbx_slist_nil();

	if (htable->nb_element) {
		tbx_htable_bucket_count_t bucket = htable->nb_bucket;

		while (bucket--) {
			p_tbx_htable_element_t element = NULL;

			element = htable->bucket_array[bucket];

			while (element) {
				char *key_copy = NULL;

				key_copy = tbx_strdup(element->key);
				tbx_slist_append(slist, key_copy);

				element = element->next;
			}
		}
	}
	PM2_LOG_OUT();

	return slist;
}

static
void tbx_htable_keystr(char *key, size_t key_len, char *buf)
{
	char t[] = "0123456789abcdef";

	while (key_len) {
		*buf++ = t[((unsigned char) *key) >> 8];
		*buf++ = t[(((unsigned char) *key) >> 8) + 1];
		key++;
	}

	*buf = '\0';
}

void tbx_htable_dump_keys(p_tbx_htable_t htable)
{
	PM2_LOG_IN();
	if (htable->nb_element) {
		tbx_htable_bucket_count_t bucket = htable->nb_bucket;

		while (bucket--) {
			p_tbx_htable_element_t element = NULL;

			element = htable->bucket_array[bucket];

			while (element) {
				char keystr[element->key_len * 2 + 1];

				tbx_htable_keystr(element->key,
						  element->key_len,
						  keystr);
				PM2_DISP("%s", keystr);
				element = element->next;
			}
		}
	}
	PM2_LOG_OUT();
}

void tbx_htable_dump_keys_strvals(p_tbx_htable_t htable)
{
	PM2_LOG_IN();
	if (htable->nb_element) {
		tbx_htable_bucket_count_t bucket = htable->nb_bucket;

		while (bucket--) {
			p_tbx_htable_element_t element = NULL;

			element = htable->bucket_array[bucket];

			while (element) {
				char keystr[element->key_len * 2 + 1];

				tbx_htable_keystr(element->key,
						  element->key_len,
						  keystr);
				PM2_DISP("%s: '%s'", keystr,
					 (char *) (element->object));
				element = element->next;
			}
		}
	}
	PM2_LOG_OUT();
}

void tbx_htable_dump_keys_ptrvals(p_tbx_htable_t htable)
{
	PM2_LOG_IN();
	if (htable->nb_element) {
		tbx_htable_bucket_count_t bucket = htable->nb_bucket;

		while (bucket--) {
			p_tbx_htable_element_t element = NULL;

			element = htable->bucket_array[bucket];

			while (element) {
				char keystr[element->key_len * 2 + 1];

				tbx_htable_keystr(element->key,
						  element->key_len,
						  keystr);
				PM2_DISP("%s: %p", keystr,
					 element->object);
				element = element->next;
			}
		}
	}
	PM2_LOG_OUT();
}
