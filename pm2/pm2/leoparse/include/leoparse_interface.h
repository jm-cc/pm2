
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

#ifndef __LEOPARSE_INTERFACE_H
#define __LEOPARSE_INTERFACE_H

/*
 * Parsing
 * -------
 */
void
leoparse_open_local_parser_file(char *file_name);

p_tbx_htable_t
leoparse_parse_local_file(char* filename);

void
leoparse_close_local_parser_file(void);

#ifdef LEOPARSE_REMOTE
void
leoparse_open_remote_parser_file(p_leoparse_swann_module_t  module,
			    char                 *file_name);

void
leoparse_close_remote_parser_file(void);
#endif /* LEOPARSE_REMOTE */


/*
 * Initialization
 * --------------
 */
void
leoparse_init(int    argc,
	      char **argv);

void
leoparse_purge_cmd_line(int   *argc,
			char **argv);

/*
 * Yacc interface
 * --------------
 */
int
leoparse_yy_input(char *buffer,
		  int   max_size);

/*
 * Objects
 * -------
 */
p_leoparse_object_t
leoparse_get_object(p_leoparse_htable_entry_t entry);

p_tbx_slist_t
leoparse_get_slist(p_leoparse_htable_entry_t entry);

p_tbx_htable_t
leoparse_get_htable(p_leoparse_object_t object);

char *
leoparse_get_string(p_leoparse_object_t object);

char *
leoparse_get_id(p_leoparse_object_t object);

p_tbx_slist_t
leoparse_read_slist(p_tbx_htable_t  htable,
		    char           *key);

char *
leoparse_read_id(p_tbx_htable_t  htable,
		 char           *key);

char *
leoparse_read_string(p_tbx_htable_t  htable,
		     char           *key);

p_tbx_htable_t
leoparse_read_htable(p_tbx_htable_t  htable,
		     char           *key);

#endif /* __LEOPARSE_INTERFACE_H */
