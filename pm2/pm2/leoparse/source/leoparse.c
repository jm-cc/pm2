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
$Log: leoparse.c,v $
Revision 1.1  2000/11/02 14:25:04  oaumage
Leoparse

Revision 1.12  2000/06/09 12:40:37  oaumage
- Progression du code

Revision 1.11  2000/06/09 08:45:59  oaumage
- Progression du code

Revision 1.10  2000/06/05 11:37:11  oaumage
- Progression du code

Revision 1.9  2000/05/31 14:17:19  oaumage
- Leonie

Revision 1.8  2000/05/23 15:33:14  oaumage
- extension de la partie analyse de la configuration

Revision 1.7  2000/05/18 11:34:18  oaumage
- remplacement des types `tableau' par des types `structure de tableau'
  par securite

Revision 1.6  2000/05/17 12:41:00  oaumage
- reorganisation du code de demarrage de Leonie

Revision 1.5  2000/05/15 13:51:56  oaumage
- Reorganisation des sources de Leonie


______________________________________________________________________________
*/

/*
 * Leoparse.c
 * ==========
 */ 
/* #define DEBUG */
/* #define TRACING */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "leoparse.h"

/*
 * Lex/Yacc and related external objects
 * -------------------------------------
 */
extern p_tbx_htable_t  leoparse_result;
extern FILE           *yyin;
extern FILE           *yyout;
extern int             yydebug;

/*
 * Parser file static variables
 * ----------------------------
 */
static FILE                      *parser_file_ptr          = NULL;
#ifdef LEOPARSE_REMOTE
static leoparse_file_handle_t     parser_file_handle       = NULL;
static p_leoparse_swann_module_t  parser_file_source       = NULL;
static void                      *parser_file_buffer       = NULL;
static int                        parser_buf_bytes_read    = 0;
static int                        parser_buf_bytes_written = 0;
#endif /* LEOPARSE_REMOTE */

int 
yylex(void);

int
yyparse(void);

/*
 * Parser input
 * ------------
 */
int
leoparse_yy_input(char *buffer,
		  int   max_size)
{
  LOG_IN();

#ifdef LEOPARSE_REMOTE
  if (!parser_file_handle)
    {
#endif /* LEOPARSE_REMOTE */

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
		  FAILURE("leoparse_yy_input failed");
		}
	    }
	}
      else
	FAILURE("invalid local file ptr");

#ifdef LEOPARSE_REMOTE
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
	      
	  leoparse_send_command(parser_file_source, ntbx_command_read_file_block);
	  leoparse_send_long(parser_file_source, (long)parser_file_handle);
	  parser_buf_bytes_written = leoparse_receive_raw_int(parser_file_source);
	  parser_buf_bytes_read    = 0;

	  if (!parser_buf_bytes_written)
	    break;

	  leoparse_receive_block(parser_file_source,
			    parser_file_buffer,
			    parser_buf_bytes_written);
	  leoparse_send_command(parser_file_source, ntbx_command_completed);
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
#endif /* LEOPARSE_REMOTE */
}

/*
 * Parser file management (local files)
 * ----------------------...............
 */
void
leoparse_open_local_parser_file(char *file_name)
{
  LOG_IN();
  if (parser_file_ptr)
    FAILURE("invalid file ptr");

#ifdef LEOPARSE_REMOTE  
  if (parser_file_handle)
    FAILURE("invalid file handle");
#endif /* LEOPARSE_REMOTE */

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
leoparse_close_local_parser_file(void)
{
  int status;
  
  LOG_IN();
  if (!parser_file_ptr)
    FAILURE("invalid file ptr");
  
#ifdef LEOPARSE_REMOTE
  if (parser_file_handle)
    FAILURE("invalid file handle");
#endif /* LEOPARSE_REMOTE */  
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
#ifdef LEOPARSE_REMOTE
void
leoparse_open_remote_parser_file(p_leoparse_swann_module_t  module,
			    char                 *file_name) 
{
  LOG_IN();
  if (parser_file_ptr)
    FAILURE("invalid file ptr");
  
  if (parser_file_handle)
    FAILURE("invalid file handle");

  parser_file_handle = NULL;
  parser_file_ptr    = NULL;
  
  leoparse_send_command(module, ntbx_command_open_file);
  leoparse_send_string(module, file_name);
  parser_file_handle = (leoparse_file_handle_t)leoparse_receive_long(module);
  leoparse_send_command(module, ntbx_command_completed);
	      
  parser_file_source       = module;
  parser_buf_bytes_written = 0;
  parser_buf_bytes_read    = 0;
  LOG_OUT();
}

void
leoparse_close_remote_parser_file(void)
{
  p_leoparse_swann_module_t module = parser_file_source;
  
  LOG_IN();
  if (parser_file_ptr)
    FAILURE("invalid file ptr");
  
  if (!parser_file_handle)
    FAILURE("invalid file handle");

  leoparse_send_command(module, ntbx_command_close_file);
  leoparse_send_long(module, (long)parser_file_handle);
  leoparse_send_command(module, ntbx_command_completed);

  parser_file_ptr          = NULL;
  parser_file_handle       = NULL;
  parser_file_source       = NULL;
  parser_buf_bytes_written = 0;
  parser_buf_bytes_read    = 0;
  LOG_OUT();
}
#endif /* LEOPARSE_REMOTE */

/*
 * Parsing entry point
 * ------------------------------------
 */
p_tbx_htable_t
leoparse_parse_local_file(char* filename)
{
  leoparse_open_local_parser_file(filename);  

  if (yyparse())
    FAILURE("parse error");

  leoparse_close_local_parser_file();

  return leoparse_result;
}

/*
 * Initialization
 * --------------
 */
void
leoparse_init(void)
{
  LOG_IN();
  yyin = NULL;

  /*
#ifdef DEBUG
  yydebug = 1;
#endif  DEBUG
  */

  parser_file_ptr    = NULL;

#ifdef LEOPARSE_REMOTE
  parser_file_source = NULL;
  parser_file_handle = NULL;
  parser_file_buffer = TBX_MALLOC(TBX_FILE_TRANSFER_BLOCK_SIZE + 1);
  ((char *)parser_file_buffer)[TBX_FILE_TRANSFER_BLOCK_SIZE] = 0;

  parser_buf_bytes_written = 0;
  parser_buf_bytes_read    = 0;
#endif /* LEOPARSE_REMOTE */
  LOG_OUT();
}

