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
*/

/*
 * Leonie.c
 * ========
 */ 
/* #define DEBUG */
/* #define TRACING */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "leonie.h"

/*
 * Constants and Macros
 * --------------------
 */
#define MAX_FILENAME_SIZE 1024
#define MAX_ARG_STR_LEN   1024
#define MAX_ARG_LEN        256

/*
 * Lex/Yacc and related external objects
 * -------------------------------------
 */
extern p_leo_parser_result_t  leo_parser_result;
extern                  FILE *yyin;
extern                  FILE *yyout;
extern                  int   yydebug;


int 
yylex(void);

int
yyparse(void);

/*
 * Static Variables
 * ----------------
 */

static p_leonie_t main_leonie = NULL;


/*
 * Functions
 * ==========================================================================
 */

/*
 * Network raw data transfer
 * -------------------------
 */

void
leo_send_block(p_ntbx_client_t  client,
	       void            *ptr,
	       size_t           length)
{
  LOG_IN();
  while (!ntbx_tcp_write_poll(1, &client));
  ntbx_tcp_write_block(client, ptr, length);
  LOG_OUT();
}

void
leo_receive_block(p_ntbx_client_t  client,
		  void            *ptr,
		  size_t           length)
{
  LOG_IN();
  while (!ntbx_tcp_read_poll(1, &client));
  ntbx_tcp_read_block(client, ptr, length);
  LOG_OUT();
}

void
leo_send_string(p_ntbx_client_t  client,
		char            *data)
{
  ntbx_pack_buffer_t pack_buffer;
  int                len;
  
  LOG_IN();
  len = strlen(data) + 1;
  ntbx_pack_int(len, &pack_buffer);
  leo_send_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  leo_send_block(client, data, len);
  LOG_OUT();
}

char *
leo_receive_string(p_ntbx_client_t client)
{
  ntbx_pack_buffer_t  pack_buffer;
  int                 len;
  char               *buffer;
  
  LOG_IN();
  leo_receive_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  len = ntbx_unpack_int(&pack_buffer);

  buffer = malloc(len);
  CTRL_ALLOC(buffer);
  
  leo_receive_block(client, buffer, len);
  LOG_OUT();
  return buffer;
}

void
leo_send_raw_int(p_ntbx_client_t client,
		 int             data)
{
  ntbx_pack_buffer_t  pack_buffer;

  LOG_IN();
  ntbx_pack_int(data, &pack_buffer);
  leo_send_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  LOG_OUT();
}

int
leo_receive_raw_int(p_ntbx_client_t client)
{
  ntbx_pack_buffer_t pack_buffer;
  int                data;
  
  LOG_IN();
  leo_receive_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  data = ntbx_unpack_int(&pack_buffer);
  LOG_OUT();

  return data;
}

void
leo_send_raw_long(p_ntbx_client_t client,
		  long            data)
{
  ntbx_pack_buffer_t  pack_buffer;

  LOG_IN();
  ntbx_pack_long(data, &pack_buffer);
  leo_send_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  LOG_OUT();
}

long
leo_receive_raw_long(p_ntbx_client_t client)
{
  ntbx_pack_buffer_t pack_buffer;
  long               data;
  
  LOG_IN();
  leo_receive_block(client, pack_buffer, sizeof(ntbx_pack_buffer_t));
  data = ntbx_unpack_long(&pack_buffer);
  LOG_OUT();

  return data;
}

/*
 * Network data transfer
 * ---------------------
 */
void
leo_send_int(p_ntbx_client_t client,
	     int             data)
{
  LOG_IN();
  leo_send_raw_int(client, sizeof(ntbx_pack_buffer_t));
  leo_send_raw_int(client, data);
  LOG_OUT();
}

int
leo_receive_int(p_ntbx_client_t client)
{
  int check;
  int data;

  LOG_IN();
  check = leo_receive_raw_int(client);
  if (check != sizeof(ntbx_pack_buffer_t))
    FAILURE("invalid data size");

  data = leo_receive_raw_int(client);

  LOG_OUT();
  return data;
}

void
leo_send_long(p_ntbx_client_t client,
	      long            data)
{
  LOG_IN();
  leo_send_raw_int(client, sizeof(ntbx_pack_buffer_t));
  leo_send_raw_long(client, data);
  LOG_OUT();
}

long
leo_receive_long(p_ntbx_client_t client)
{
  int check;
  int data;

  LOG_IN();
  check = leo_receive_raw_int(client);
  if (check != sizeof(ntbx_pack_buffer_t))
    FAILURE("invalid data size");

  data = leo_receive_raw_long(client);

  LOG_OUT();
  return data;
}

void
leo_send_command(p_leo_swann_module_t module,
		 ntbx_command_code_t  code)
{
  p_ntbx_client_t net_client = module->net_client;

  LOG_IN();
  leo_send_raw_int(net_client, module->id);
  leo_send_int(net_client, code);
  LOG_OUT();
}

/*
 * Parser input
 * ------------
 */
int
leo_yy_input(char *buffer,
	     int   max_size)
{
  LOG_IN();
  if (main_leonie)
    {
      leo_file_handle_t   file_handle =
	main_leonie->parser_file_handle;
      FILE               *file_ptr     =
	main_leonie->parser_file_ptr;
      
      if (!file_handle)
	{
	  /* Local file */
	  if (file_ptr)
	    {
	      int status;

	      status = fread(buffer, 1, max_size, file_ptr);

	      if (status)
		{
		  LOG_OUT();
		  return status;
		}
	      else
		{
		  if (feof(file_ptr))
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
	  p_leo_swann_module_t module     = main_leonie->parser_file_source;
	  p_ntbx_client_t      net_client = module->net_client;
	  int                  len        = 0;

	  TRACE_IN();
	  if (file_ptr)
	    FAILURE("invalid local file ptr");

	  if (main_leonie->parser_buf_bytes_read
	      < main_leonie->parser_buf_bytes_written)
	    {
	      int diff =
		main_leonie->parser_buf_bytes_written
		- main_leonie->parser_buf_bytes_read;
	      int size = min(diff, max_size);

	      memcpy(buffer, main_leonie->parser_file_buffer, size);
	      main_leonie->parser_buf_bytes_read += size;
	      buffer                        += size;
	      max_size                      -= size;
	      len                           += size;
	    }
	  
	  while(max_size > 0)
	    {
	      int size = 0;
	      
	      leo_send_command(module, ntbx_command_read_file_block);
	      TRACE_PTR("File handle", main_leonie->parser_file_handle);
	      leo_send_long(net_client, (long)main_leonie->parser_file_handle);
	      main_leonie->parser_buf_bytes_written =
		leo_receive_raw_int(net_client);
	      main_leonie->parser_buf_bytes_read    = 0;

	      if (!main_leonie->parser_buf_bytes_written)
		break;

	      leo_receive_block(net_client,
				main_leonie->parser_file_buffer,
				main_leonie->parser_buf_bytes_written);
	      leo_send_command(module, ntbx_command_completed);
	      size = min(main_leonie->parser_buf_bytes_written, max_size);
	      memcpy(buffer, main_leonie->parser_file_buffer, size);
	      main_leonie->parser_buf_bytes_read  = size;
	      buffer                             += size;
	      max_size                           -= size;
	      len                                += size;
	    }

	  TRACE_OUT();
	  return len;
	}
    }
  else
    FAILURE("Leonie uninitialized");
}

/*
 * Parser file management (local files)
 * ----------------------...............
 */
static void
leo_open_local_parser_file(p_leonie_t  leonie,
			   char       *file_name)
{
  FILE                    *file_ptr    = NULL;

  LOG_IN();
  if (leonie->parser_file_ptr)
    FAILURE("invalid file ptr");
  
  if (leonie->parser_file_handle)
    FAILURE("invalid file handle");

  do
    {
      file_ptr = fopen(file_name, "r");

      if ((!file_ptr) && (errno != EINTR))
	{
	  perror("fopen");
	  FAILURE("could not open a file for parsing");
	}
    }
  while(!file_ptr);

  leonie->parser_file_handle = NULL;
  leonie->parser_file_ptr    = file_ptr;
  LOG_OUT();
}

static void
leo_close_local_parser_file(p_leonie_t leonie)
{
  int status;
  
  LOG_IN();
  if (!leonie->parser_file_ptr)
    FAILURE("invalid file ptr");
  
  if (leonie->parser_file_handle)
    FAILURE("invalid file handle");
  
  do
    {
      status = fclose(leonie->parser_file_ptr);
      if (status && errno != EINTR)
	{
	  perror("fclose");
	  FAILURE("could not close file");
	}
    }
  while(status);

  leonie->parser_file_ptr    = NULL;
  leonie->parser_file_handle = NULL;
  LOG_OUT();
}

/*
 * Parser file management (remote files)
 * ----------------------................
 */
static void
leo_open_remote_parser_file(p_leonie_t            leonie,
			    p_leo_swann_module_t  module,
			    char                 *file_name) __attribute__((unused));

static void
leo_open_remote_parser_file(p_leonie_t            leonie,
			    p_leo_swann_module_t  module,
			    char                 *file_name) 
{
  LOG_IN();
  if (leonie->parser_file_ptr)
    FAILURE("invalid file ptr");
  
  if (leonie->parser_file_handle)
    FAILURE("invalid file handle");

  leonie->parser_file_handle = NULL;
  leonie->parser_file_ptr    = NULL;
  
  leo_send_command(module, ntbx_command_open_file);
  leo_send_string(module->net_client, file_name);
  leonie->parser_file_handle =
    (leo_file_handle_t)leo_receive_long(module->net_client);
  leo_send_command(module, ntbx_command_completed);
	      
  leonie->parser_file_source       = module;
  leonie->parser_buf_bytes_written = 0;
  leonie->parser_buf_bytes_read    = 0;
  LOG_OUT();
}

static void
leo_close_remote_parser_file(p_leonie_t leonie) __attribute__((unused));

static void
leo_close_remote_parser_file(p_leonie_t leonie)
{
  p_leo_swann_module_t module = main_leonie->parser_file_source;
  
  LOG_IN();
  if (leonie->parser_file_ptr)
    FAILURE("invalid file ptr");
  
  if (!leonie->parser_file_handle)
    FAILURE("invalid file handle");

  leo_send_command(module, ntbx_command_close_file);
  leo_send_long(module->net_client, (long)leonie->parser_file_handle);
  leo_send_command(module, ntbx_command_completed);

  leonie->parser_file_ptr          = NULL;
  leonie->parser_file_handle       = NULL;
  leonie->parser_file_source       = NULL;
  leonie->parser_buf_bytes_written = 0;
  leonie->parser_buf_bytes_read    = 0;
  LOG_OUT();
}

/*
 * List processing
 * ---------------
 */
static char *
leo_find_string(p_tbx_list_t  list,
		char         *string)
{
  tbx_list_reference_t ref;

  LOG_IN();
  if (tbx_empty_list(list))
    return NULL;
  
  tbx_list_reference_init(&ref, list);
  do
    {
      char *str = tbx_get_list_reference_object(&ref);

      if (!strcmp(string, str))
	{
	  LOG_OUT();
	  return str;
	}
    }
  while(tbx_forward_list_reference(&ref));

  LOG_OUT();
  return NULL;
}

/*
 * Cluster configuration analyser
 * ------------------------------
 */
static p_leo_clu_host_name_t
leo_get_cluster_entry_point(p_leo_clu_cluster_t  cluster,
			    char                *cluster_id)
{
  tbx_list_reference_t ref;

  LOG_IN();
  if (tbx_empty_list(cluster->hosts))
    FAILURE("empty cluster");
  
  tbx_list_reference_init(&ref, cluster->hosts);
  do
    {
      p_leo_clu_host_name_t host_description =
	tbx_get_list_reference_object(&ref);

      if (leo_find_string(host_description->name_list, cluster_id))
	{
	  LOG_OUT();
	  return host_description;
	}
    }
  while(tbx_forward_list_reference(&ref));
  
  LOG_OUT();
  return NULL;
}

/*
 * Swann interfacing
 * -----------------
 */
static p_leo_swann_module_t
leo_launch_swann_module(p_leonie_t             leonie,
			char                  *cluster_id,
			p_leo_clu_host_name_t  host_description)
{
  p_leo_clu_cluster_file_t  remote_cluster_def = NULL;
  char                     *filename           = NULL;
  p_leo_swann_module_t      module             = NULL;
  char                     *cmd                = NULL;
  ntbx_status_t             status;  
  /* int                    n; */
  
  LOG_IN();
  cmd = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(cmd);

  module = malloc(sizeof(leo_swann_module_t));
  CTRL_ALLOC(module);

  module->id         = ++leonie->cluster_counter;
  module->cluster_id = malloc(strlen(cluster_id) + 1);
  CTRL_ALLOC(module->cluster_id);
  strcpy(module->cluster_id, cluster_id);
  
  module->net_client = malloc(sizeof(ntbx_client_t));
  CTRL_ALLOC(module->net_client);
  module->net_client->state = ntbx_client_state_uninitialized;
  ntbx_tcp_client_init(module->net_client);

  sprintf(cmd,
	  "rsh %s swann -id %d -cnx %s -master %s &",
	  cluster_id,
	  module->id,
	  leonie->net_server->connection_data,
	  module->net_client->local_host);
  DISP("Launching swann module: %s", cmd);
  system(cmd);
  status = ntbx_tcp_server_accept(leonie->net_server, module->net_client);
  if (status != ntbx_success)
    FAILURE("could not establish a connection with the swann module");

  filename = malloc(MAX_FILENAME_SIZE);
  leo_open_remote_parser_file(main_leonie, module, "~/.pm2/leo_cluster");

  DISP("Parsing local cluster description file ...");
  
  if (yyparse())
    FAILURE("parse error (cluster file)");

  if (    leo_parser_result->application
      || !leo_parser_result->cluster_file
      ||  leo_parser_result->protocol_table)
    FAILURE("parse error (cluster file)");

  remote_cluster_def = leo_parser_result->cluster_file;
  DISP("Remote cluster id is %s", remote_cluster_def->cluster->id);
  leo_close_remote_parser_file(main_leonie);
  leo_send_command(module, ntbx_command_terminate);
 
  free(filename);
  LOG_OUT();
  return module;
}

/*
 * Session management
 * ------------------
 */
static void
leo_cluster_setup(p_leonie_t               leonie,
		  p_leo_app_application_t  application,
		  p_leo_clu_cluster_file_t local_cluster_def)
{
  tbx_list_reference_t ref;
  
  LOG_IN();
  if (tbx_empty_list(application->cluster_list))
    FAILURE("empty cluster");
  
  tbx_list_reference_init(&ref, application->cluster_list);
  do
    {
      p_leo_app_cluster_t app_cluster =
	tbx_get_list_reference_object(&ref);

      if (!strcmp(local_cluster_def->cluster->id, app_cluster->id))
	{
	  /* cluster is local */
	  DISP("Cluster %s is local", app_cluster->id);
	}
      else
	{
	  /* cluster is remote */
	  p_leo_clu_host_name_t host_description;

	  DISP("Cluster %s is remote", app_cluster->id);
	  DISP("Looking for %s entry point", app_cluster->id);

	  host_description =
	    leo_get_cluster_entry_point(local_cluster_def->cluster,
					app_cluster->id);
	  if (!host_description)
	    FAILURE("could not find cluster entry point");

	  DISP("Entry point found");

	  leo_launch_swann_module(leonie,
				  app_cluster->id,
				  host_description);
	}
    }
  while(tbx_forward_list_reference(&ref));
  LOG_OUT();
}

/*
 * Initialization
 * --------------
 */
int
main (int argc, char *argv[])
{
  p_leo_app_application_t  application       = NULL;
  p_leo_clu_cluster_file_t local_cluster_def = NULL;
  p_ntbx_server_t          net_server        = NULL;
  char                    *filename          = NULL;

  LOG_IN();
  tbx_init();
  ntbx_init();

  main_leonie = TBX_MALLOC(sizeof(leonie_t));
  CTRL_ALLOC(main_leonie);
  main_leonie->parser_file_handle       = NULL;
  main_leonie->parser_file_ptr          = NULL;
  main_leonie->parser_file_source       = NULL;
  main_leonie->parser_file_buffer       =
    TBX_MALLOC(TBX_FILE_TRANSFER_BLOCK_SIZE + 1);
  ((char *)main_leonie->parser_file_buffer)[TBX_FILE_TRANSFER_BLOCK_SIZE] = 0;
  main_leonie->parser_buf_bytes_written = 0;
  main_leonie->parser_buf_bytes_read    = 0;
  main_leonie->cluster_counter          = 0;
  tbx_list_init(&main_leonie->swann_modules);

  yyin = NULL;
#ifdef DEBUG
  yydebug = 1;
#endif /* DEBUG */
 
  if (argc < 2)
    FAILURE("no application description file specified");

  /*
   * application description file parsing
   * ------------------------------------
   */
  leo_open_local_parser_file(main_leonie, argv[1]);  

  DISP("Parsing application description file ...");
  
  if (yyparse())
    FAILURE("parse error (application file)");

  if (   !leo_parser_result->application
      ||  leo_parser_result->cluster_file
      ||  leo_parser_result->protocol_table)
    FAILURE("parse error (application file)");

  application = leo_parser_result->application;
  leo_close_local_parser_file(main_leonie);

  DISP("Starting application %s", application->id);

  /*
   * Net server initialization
   * -------------------------
   */
  net_server = malloc(sizeof(ntbx_server_t));
  CTRL_ALLOC(net_server);
  net_server->state = ntbx_server_state_uninitialized;
  ntbx_tcp_server_init(net_server);
  DISP("net server ready and listening at %s",
       net_server->connection_data);
  main_leonie->net_server = net_server;

  /*
   * Local cluster information retrieval
   * -----------------------------------
   */
  filename = malloc(MAX_FILENAME_SIZE);
  strcpy(filename, getenv("HOME"));
  strcat(filename, "/.pm2/leo_cluster");
  
  leo_open_local_parser_file(main_leonie, filename);  

  DISP("Parsing local cluster description file ...");
  
  if (yyparse())
    FAILURE("parse error (cluster file)");

  if (    leo_parser_result->application
      || !leo_parser_result->cluster_file
      ||  leo_parser_result->protocol_table)
    FAILURE("parse error (cluster file)");

  local_cluster_def = leo_parser_result->cluster_file;
  DISP("Local cluster id is %s", local_cluster_def->cluster->id);
  leo_close_local_parser_file(main_leonie);

  /*
   * Cluster intialization
   * ---------------------
   */
  DISP("Starting application %s", application->id);

  leo_cluster_setup(main_leonie,
		    application,
		    local_cluster_def);  
  LOG_OUT();
  return 0;
}
