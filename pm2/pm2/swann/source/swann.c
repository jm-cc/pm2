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
 * swann.c
 * =======
 */ 
#define DEBUG
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "swann.h"

static
p_swann_t
swann_object_init(void)
{
  p_swann_t       swann  = NULL;
  p_ntbx_client_t client = NULL;

  LOG_IN();
  swann = swann_init();
  
  swann->session = swann_session_init();

  client = ntbx_client_cons();
  ntbx_tcp_client_init(client);
  swann->session->leonie_link = client;
  LOG_OUT();

  return swann;
}

static
p_swann_settings_t
process_command_line(int    argc,
		     char **argv)
{
  p_swann_settings_t settings = NULL;

  tbx_arg_iterator(argc, argv, swann_usage);
  
  LOG_IN();
  if (!tbx_argit_init())
    swann_usage();

  settings = swann_settings_init();

  if (tbx_argit_more_opts())
    {
      if (tbx_argit_opt_equals("--help"))
	{
	  swann_help();
	}
      
      do
	{
	  if (tbx_argit_vopt_equals("--mad_leonie"))
	    {
	      if (!settings->leonie_server_host_name)
		{
		  settings->leonie_server_host_name =
		    tbx_argit_vopt_value_cstr();
		}
	      else
		swann_terminate("duplicate leonie host name definition"); 

	      DISP_STR("Leonie host name",
		       settings->leonie_server_host_name);
	    }
	  else if (tbx_argit_vopt_equals("--mad_link"))
	    {
	      if (!settings->leonie_server_port)
		{
		  settings->leonie_server_port =
		    tbx_argit_vopt_value_cstr();
		}
	      else
		swann_terminate("duplicate leonie link port number definition"); 

	      DISP_STR("Leonie link port number",
		       settings->leonie_server_port);
	    }
	  else if (tbx_argit_arg_equals("-x"))
	    {
	      settings->xterm_mode = tbx_true;
	      settings->log_mode   = tbx_false;
	    }
	  else if (tbx_argit_arg_equals("--x"))
	    {
	      settings->xterm_mode = tbx_false;
	      settings->gdb_mode   = tbx_false;
	    }
	  else if (tbx_argit_arg_equals("-l"))
	    {
	      settings->log_mode   = tbx_true;

	      settings->xterm_mode = tbx_false;
	      settings->gdb_mode   = tbx_false;
	      settings->pause_mode = tbx_false;
	    }
	  else if (tbx_argit_arg_equals("--l"))
	    {
	      settings->log_mode = tbx_false;
	    }
	  else if (tbx_argit_arg_equals("-p"))
	    {
	      settings->pause_mode = tbx_true;

	      settings->gdb_mode   = tbx_false;
	      settings->log_mode   = tbx_false;
	    }
	  else if (tbx_argit_arg_equals("--p"))
	    {
	      settings->pause_mode = tbx_false;
	    }
	  else if (tbx_argit_arg_equals("-d"))
	    {
	      settings->gdb_mode   = tbx_true;
	      
	      settings->xterm_mode = tbx_true;
	      settings->log_mode   = tbx_false;
	      settings->pause_mode = tbx_false;
	    }
	  else if (tbx_argit_arg_equals("--d"))
	    {
	      settings->gdb_mode = tbx_false;
	    }
	  else if (tbx_argit_arg_equals("-smp"))
	    {
	      settings->smp_mode   = tbx_true;
	    }
	  else if (tbx_argit_arg_equals("--smp"))
	    {
	      settings->smp_mode = tbx_false;
	    }
	  else
	    tbx_argit_invalid_arg();
	  
	}
      while (tbx_argit_next_opt());
    }
  else
    {
      DISP("no command line options");
    }

  return settings;
}

static 
void
swann_leonie_link_init(p_swann_t swann)
{
  p_swann_settings_t     settings = NULL;
  p_swann_session_t      session  = NULL;
  p_ntbx_client_t        client   = NULL;
  int                    status   = ntbx_failure;
  ntbx_connection_data_t data;

  LOG_IN();
  settings = swann->settings;
  session  = swann->session;
  client   = session->leonie_link;

  strcpy(data.data, settings->leonie_server_port);
  status = ntbx_tcp_client_connect(client,
				   settings->leonie_server_host_name,
				   &data);
  if (status == ntbx_failure)
    FAILURE("could not setup the Leonie link");

  DISP("Leonie link is up");  
  swann_ntbx_send_string(client, client->local_host);
  session->process_rank = swann_ntbx_receive_int(client);
  DISP_VAL("process rank", session->process_rank);
  LOG_OUT();
}

/*
 * Initialization
 * --------------
 */
int 
main(int   argc,
     char *argv[])
{
  p_swann_t swann = NULL;

  LOG_IN();
  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);
  
  DISP("== Initializing parser");
  leoparse_init(argc, argv);
  leoparse_purge_cmd_line(&argc, argv);
  
  DISP("== Initializing internal objects");
  swann = swann_object_init();

  DISP("== Processing command line");
  swann->settings = process_command_line(argc, argv);

  DISP("== Initializing Leonie link");
  swann_leonie_link_init(swann);
  LOG_OUT();

  return 0;
}
