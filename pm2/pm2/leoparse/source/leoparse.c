
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
leoparse_init(int    argc,
	      char **argv)
{
  LOG_IN();
  if (!initialized)
    {
      initialized = tbx_true;
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
    }
  
  LOG_OUT();
}

void
leoparse_purge_cmd_line(int   *argc,
			char **argv)
{
  LOG_IN();
  /* --- */
  LOG_OUT();
}
