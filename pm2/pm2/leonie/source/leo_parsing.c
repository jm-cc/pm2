/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 suppong documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: leo_parsing.c,v $
Revision 1.2  2000/06/09 08:45:58  oaumage
- Progression du code

Revision 1.1  2000/05/15 13:51:55  oaumage
- Reorganisation des sources de Leonie


______________________________________________________________________________
*/

/*
 * Leo_parsing.c
 * =============
 */ 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "leonie.h"

/*
 * Lex/Yacc and related external objects
 * -------------------------------------
 */
extern p_leo_parser_result_t  leo_parser_result;
extern                  FILE *yyin;
extern                  FILE *yyout;
extern                  int   yydebug;

/*
 * Parser file static variables
 * ----------------------------
 */
static leo_file_handle_t     parser_file_handle       = NULL;
static FILE                 *parser_file_ptr          = NULL;
static p_leo_swann_module_t  parser_file_source       = NULL;
static void                 *parser_file_buffer       = NULL;
static int                   parser_buf_bytes_read    = 0;
static int                   parser_buf_bytes_written = 0;

int 
yylex(void);

int
yyparse(void);

/*
 * Parser input
 * ------------
 */
int
leo_yy_input(char *buffer,
	     int   max_size)
{
  LOG_IN();

  if (!parser_file_handle)
    {
      /* Local file */
      if (parser_file_ptr)
	{
	  int status;

	  status = fread(buffer, 1, max_size, parser_file_ptr);

	  if (status)
	    {
	      LOG_OUT();
	      return status;
	    }
	  else
	    {
	      if (feof(parser_file_ptr))
		{
		  LOG_OUT();
		  return 0; /* YY_NULL */
		}
	      else
		{
		  perror("fread");
		  FAILURE("leo_yy_input failed");
		}
	    }
	}
      else
	FAILURE("invalid local file ptr");
    }
  else
    {
      /* Remote file */
      int             len        = 0;

      if (parser_file_ptr)
	FAILURE("invalid local file ptr");

      if (parser_buf_bytes_read < parser_buf_bytes_written)
	{
	  int diff = parser_buf_bytes_written - parser_buf_bytes_read;
	  int size = min(diff, max_size);

	  memcpy(buffer, parser_file_buffer, size);
	  parser_buf_bytes_read += size;
	  buffer                += size;
	  max_size              -= size;
	  len                   += size;
	}
	  
      while(max_size > 0)
	{
	  int size = 0;
	      
	  leo_send_command(parser_file_source, ntbx_command_read_file_block);
	  leo_send_long(parser_file_source, (long)parser_file_handle);
	  parser_buf_bytes_written = leo_receive_raw_int(parser_file_source);
	  parser_buf_bytes_read    = 0;

	  if (!parser_buf_bytes_written)
	    break;

	  leo_receive_block(parser_file_source,
			    parser_file_buffer,
			    parser_buf_bytes_written);
	  leo_send_command(parser_file_source, ntbx_command_completed);
	  size = min(parser_buf_bytes_written, max_size);
	  memcpy(buffer, parser_file_buffer, size);
	  parser_buf_bytes_read  = size;
	  buffer                += size;
	  max_size              -= size;
	  len                   += size;
	}

      LOG_OUT();
      return len;
    }
}

/*
 * Parser file management (local files)
 * ----------------------...............
 */
void
leo_open_local_parser_file(char *file_name)
{
  LOG_IN();
  if (parser_file_ptr)
    FAILURE("invalid file ptr");
  
  if (parser_file_handle)
    FAILURE("invalid file handle");

  do
    {
      parser_file_ptr = fopen(file_name, "r");

      if ((!parser_file_ptr) && (errno != EINTR))
	{
	  perror("fopen");
	  FAILURE("could not open a file for parsing");
	}
    }
  while(!parser_file_ptr);
  LOG_OUT();
}

void
leo_close_local_parser_file(void)
{
  int status;
  
  LOG_IN();
  if (!parser_file_ptr)
    FAILURE("invalid file ptr");
  
  if (parser_file_handle)
    FAILURE("invalid file handle");
  
  do
    {
      status = fclose(parser_file_ptr);
      if (status && errno != EINTR)
	{
	  perror("fclose");
	  FAILURE("could not close file");
	}
    }
  while(status);
  parser_file_ptr = NULL;
  LOG_OUT();
}

/*
 * Parser file management (remote files)
 * ----------------------................
 */
void
leo_open_remote_parser_file(p_leo_swann_module_t  module,
			    char                 *file_name) 
{
  LOG_IN();
  if (parser_file_ptr)
    FAILURE("invalid file ptr");
  
  if (parser_file_handle)
    FAILURE("invalid file handle");

  parser_file_handle = NULL;
  parser_file_ptr    = NULL;
  
  leo_send_command(module, ntbx_command_open_file);
  leo_send_string(module, file_name);
  parser_file_handle = (leo_file_handle_t)leo_receive_long(module);
  leo_send_command(module, ntbx_command_completed);
	      
  parser_file_source       = module;
  parser_buf_bytes_written = 0;
  parser_buf_bytes_read    = 0;
  LOG_OUT();
}

void
leo_close_remote_parser_file(void)
{
  p_leo_swann_module_t module = parser_file_source;
  
  LOG_IN();
  if (parser_file_ptr)
    FAILURE("invalid file ptr");
  
  if (!parser_file_handle)
    FAILURE("invalid file handle");

  leo_send_command(module, ntbx_command_close_file);
  leo_send_long(module, (long)parser_file_handle);
  leo_send_command(module, ntbx_command_completed);

  parser_file_ptr          = NULL;
  parser_file_handle       = NULL;
  parser_file_source       = NULL;
  parser_buf_bytes_written = 0;
  parser_buf_bytes_read    = 0;
  LOG_OUT();
}

/*
 * Cluster description file parsing
 * ------------------------------------
 */
p_leo_clu_cluster_file_t
leo_parse_cluster_file(p_leo_swann_module_t  module,
		       char                 *filename)
{
  if (module)
    leo_open_remote_parser_file(module, filename);  
  else
    leo_open_local_parser_file(filename);  

  if (yyparse())
    FAILURE("parse error (cluster file)");

  if (    leo_parser_result->application
      || !leo_parser_result->cluster_file
      ||  leo_parser_result->protocol_table)
    FAILURE("parse error (cluster file)");

  if (module)
    {
      leo_close_remote_parser_file();
      leo_send_command(module, ntbx_command_terminate);
    }
  else
    leo_close_local_parser_file();

  return leo_parser_result->cluster_file;
}

/*
 * Application description file parsing
 * ------------------------------------
 */
p_leo_app_application_t
leo_parse_application_file(char *filename)
{
  leo_open_local_parser_file(filename);  

  if (yyparse())
    FAILURE("parse error (application file)");

  if (   !leo_parser_result->application
      ||  leo_parser_result->cluster_file
      ||  leo_parser_result->protocol_table)
    FAILURE("parse error (application file)");

  leo_close_local_parser_file();

  return leo_parser_result->application;
}

/*
 * Initialization
 * --------------
 */
void
leo_parser_init(void)
{
  yyin = NULL;
  /*
#ifdef DEBUG
  yydebug = 1;
#endif  DEBUG */

  parser_file_handle = NULL;
  parser_file_ptr    = NULL;
  parser_file_source = NULL;

  parser_file_buffer = TBX_MALLOC(TBX_FILE_TRANSFER_BLOCK_SIZE + 1);
  ((char *)parser_file_buffer)[TBX_FILE_TRANSFER_BLOCK_SIZE] = 0;

  parser_buf_bytes_written = 0;
  parser_buf_bytes_read    = 0;
}
