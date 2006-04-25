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
 * Leoparse_interface.h
 * ====================
 */

#ifndef LEOPARSE_INTERFACE_H
#define LEOPARSE_INTERFACE_H

/*
 * Parsing
 * -------
 */
void
leoparse_open_local_parser_file(const char *file_name);

p_tbx_htable_t
leoparse_parse_local_file(const char *filename);

void
leoparse_close_local_parser_file(void);


/*
 *Initialization
 *--------------
 */
void
leoparse_init(int    argc,
	      char **argv);

void
leoparse_purge_cmd_line(int   *argc,
			char **argv);

/*
 *Yacc interface
 *--------------
 */
int
leoparse_yy_input(char         *buffer,
		  unsigned int  max_size);

/*
 *Objects
 *-------
 */
p_leoparse_object_t
leoparse_init_object(void);

p_leoparse_object_t
leoparse_init_slist_object(p_tbx_slist_t slist);

p_leoparse_object_t
leoparse_init_htable_object(p_tbx_htable_t htable);

p_leoparse_object_t
leoparse_init_id_object(char                  *id,
			p_leoparse_modifier_t  modifier);

p_leoparse_object_t
leoparse_init_string_object(char *string);

p_leoparse_object_t
leoparse_init_val_object(int value);

p_leoparse_object_t
leoparse_init_range_object(p_leoparse_range_t range);

void
leoparse_release_object(p_leoparse_object_t object);

p_tbx_slist_t
leoparse_get_slist(p_leoparse_object_t object);

p_tbx_slist_t
leoparse_get_as_slist(p_leoparse_object_t object);

p_tbx_htable_t
leoparse_get_htable(p_leoparse_object_t object);

char *
leoparse_get_string(p_leoparse_object_t object);

char *
leoparse_get_id(p_leoparse_object_t object);

int
leoparse_get_val(p_leoparse_object_t object);

p_leoparse_range_t
leoparse_get_range(p_leoparse_object_t object);

p_tbx_slist_t
leoparse_read_slist(p_tbx_htable_t  htable,
		    const char     *key);

p_tbx_slist_t
leoparse_read_as_slist(p_tbx_htable_t  htable,
		       const char     *key);

char *
leoparse_read_id(p_tbx_htable_t  htable,
		 const char     *key);

char *
leoparse_read_string(p_tbx_htable_t  htable,
		     const char     *key);

p_tbx_htable_t
leoparse_read_htable(p_tbx_htable_t  htable,
		     const char     *key);

int
leoparse_read_val(p_tbx_htable_t  htable,
		  const char     *key);

p_leoparse_range_t
leoparse_read_range(p_tbx_htable_t  htable,
		    const char     *key);

p_tbx_slist_t
leoparse_extract_slist(p_tbx_htable_t  htable,
		       const char     *key);

p_tbx_slist_t
leoparse_extract_as_slist(p_tbx_htable_t  htable,
			  const char     *key);

char *
leoparse_extract_id(p_tbx_htable_t  htable,
		    const char     *key);

char *
leoparse_extract_string(p_tbx_htable_t  htable,
			const char     *key);

p_tbx_htable_t
leoparse_extract_htable(p_tbx_htable_t  htable,
			const char     *key);

int
leoparse_extract_val(p_tbx_htable_t  htable,
		     const char     *key);

p_leoparse_range_t
leoparse_extract_range(p_tbx_htable_t  htable,
		       const char     *key);

p_tbx_slist_t
leoparse_try_get_slist(p_leoparse_object_t object);

p_tbx_slist_t
leoparse_try_get_as_slist(p_leoparse_object_t object);

p_tbx_htable_t
leoparse_try_get_htable(p_leoparse_object_t object);

char *
leoparse_try_get_string(p_leoparse_object_t object);

char *
leoparse_try_get_id(p_leoparse_object_t object);

int
leoparse_try_get_val(p_leoparse_object_t object,
		     const int           default_value);

p_leoparse_range_t
leoparse_try_get_range(p_leoparse_object_t object);

p_tbx_slist_t
leoparse_try_read_slist(p_tbx_htable_t  htable,
			const char     *key);

p_tbx_slist_t
leoparse_try_read_as_slist(p_tbx_htable_t  htable,
			   const char     *key);

char *
leoparse_try_read_id(p_tbx_htable_t  htable,
		     const char     *key);

char *
leoparse_try_read_string(p_tbx_htable_t  htable,
			 const char     *key);

p_tbx_htable_t
leoparse_try_read_htable(p_tbx_htable_t  htable,
			 const char     *key);

int
leoparse_try_read_val(p_tbx_htable_t  htable,
		      const char     *key,
		      const int       default_value);

p_leoparse_range_t
leoparse_try_read_range(p_tbx_htable_t  htable,
			const char     *key);

void
leoparse_convert_to_slist(p_tbx_htable_t  htable,
			  const char     *key);

void
leoparse_write_slist(p_tbx_htable_t  htable,
		     const char     *key,
		     p_tbx_slist_t   slist);

void
leoparse_write_id(p_tbx_htable_t  htable,
		  const char     *key,
		  char           *id);

void
leoparse_write_string(p_tbx_htable_t  htable,
		      const char     *key,
		      char           *string);

void
leoparse_write_htable(p_tbx_htable_t  htable,
		      const char     *key,
		      p_tbx_htable_t  table);

void
leoparse_write_val(p_tbx_htable_t  htable,
		   const char     *key,
		   int             val);

void
leoparse_write_range(p_tbx_htable_t      htable,
		     const char         *key,
		     p_leoparse_range_t  range);


void
leoparse_dump_object_slist(p_tbx_slist_t l);

void
leoparse_dump_object_htable(p_tbx_htable_t h);

void
leoparse_dump_object(p_leoparse_object_t object);
#endif /* LEOPARSE_INTERFACE_H */
