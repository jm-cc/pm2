
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
