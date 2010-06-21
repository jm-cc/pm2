
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
 * Leoparse.c
 * ==========
 */ 

/* #define TRACING */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "tbx.h"
#include "leoparse.h"

static volatile tbx_bool_t initialized = tbx_false;

/*
 * Lex/Yacc and related external objects
 * -------------------------------------
 */
extern p_tbx_htable_t  leoparse_result;
extern FILE           *yyin;
extern FILE           *yyout;
extern int             yydebug;

/*
 * Lex/Yacc and related global objects
 * -------------------------------------
 */
const char *parser_filename   = NULL;
int         parser_line_num   =    1;
int         parser_column_num =    0;

/*
 * Parser file static variables
 * ----------------------------
 */
static FILE *parser_file_ptr = NULL;

int 
yylex(void);

int
yyparse(void);

/*
 * Parser input
 * ------------
 */
int
leoparse_yy_input(char         *buffer,
		  unsigned int  max_size)
{

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
	      return status;
	    }
	  else
	    {
	      if (feof(parser_file_ptr))
		{
		  return 0; /* YY_NULL */
		}
	      else
		{
		  perror("fread");
		  TBX_FAILURE("leoparse_yy_input failed");
		}
	    }
	}
      else
	TBX_FAILURE("invalid local file ptr");

#ifdef LEOPARSE_REMOTE
    }
  else
    {
      /* Remote file */
      int             len        = 0;

      if (parser_file_ptr)
	TBX_FAILURE("invalid local file ptr");

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

      return len;
    }
#endif /* LEOPARSE_REMOTE */
}

/*
 * Parser file management (local files)
 * ----------------------...............
 */
void
leoparse_open_local_parser_file(const char *file_name)
{
  if (parser_file_ptr)
    TBX_FAILURE("invalid file ptr");

#ifdef LEOPARSE_REMOTE  
  if (parser_file_handle)
    TBX_FAILURE("invalid file handle");
#endif /* LEOPARSE_REMOTE */

  do
    {
      parser_file_ptr = fopen(file_name, "r");

      if ((!parser_file_ptr) && (errno != EINTR))
	{
	  fprintf(stderr, "Unable to open file : %s\n", file_name);
	  perror("fopen");
	  TBX_FAILURE("could not open a file for parsing");
	}
    }
  while(!parser_file_ptr);
  parser_filename   = file_name;
  parser_line_num   = 1;
  parser_column_num = 0;
}

void
leoparse_close_local_parser_file(void)
{
  int status;
  
  if (!parser_file_ptr)
    TBX_FAILURE("invalid file ptr");
  
  do
    {
      status = fclose(parser_file_ptr);
      if (status && errno != EINTR)
	{
	  perror("fclose");
	  TBX_FAILURE("could not close file");
	}
    }
  while(status);
  parser_filename = NULL;
  parser_file_ptr = NULL;
}

/*
 * Parser file management (remote files)
 * ----------------------................
 */

/*
 * Parsing entry point
 * ------------------------------------
 */
p_tbx_htable_t
leoparse_parse_local_file(const char* filename)
{
  leoparse_open_local_parser_file(filename);  

  if (yyparse())
    TBX_FAILURE("parse error");

  leoparse_close_local_parser_file();

  return leoparse_result;
}

/*
 * Initialization
 * --------------
 */
void
leoparse_init(int    argc TBX_UNUSED,
	      char **argv TBX_UNUSED)
{
  if (!initialized)
    {
      initialized        = tbx_true;
      yyin               = NULL;
      parser_filename  = NULL;

      /*
	#ifdef PM2DEBUG
	yydebug = 1;
	#endif  PM2DEBUG
	*/

      parser_file_ptr    = NULL;

    }
  
}

void
leoparse_purge_cmd_line(int   *argc TBX_UNUSED,
			char **argv TBX_UNUSED)
{
  /* --- */
}
