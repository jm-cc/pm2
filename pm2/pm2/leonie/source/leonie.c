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
 * leonie.c
 * ========
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "leonie.h"

/*
 * Functions
 * =========
 */

// process_command_line: analyses the contents of the 'argv' array using
// the TBX argument iterator.
// Note: this function relies on the 'local functions' gcc extension.
static
p_leo_settings_t
process_command_line(int    argc,
		     char **argv)
{
  p_leo_settings_t settings = NULL;

  tbx_arg_iterator(argc, argv, leo_usage);

  LOG_IN();
  if (!tbx_argit_init())
    leo_usage();

  settings = leo_settings_init();

  if (tbx_argit_more_opts())
    {
      if (tbx_argit_opt_equals("--help"))
	{
	  leo_help();
	}

      do
	{
	  if (tbx_argit_vopt_equals("--env")) //GM
	    {		
	      if (!settings->env)
		{
		  settings->env_mode = tbx_true;
		  settings->env = tbx_argit_vopt_value_cstr();
		}		
	      else
		leo_terminate("duplicate environment");
	    }
	   else if (tbx_argit_vopt_equals("--appli"))
	    {
	      if (!settings->name)
		{
		  settings->name = tbx_argit_vopt_value_cstr();
		}
	      else
		leo_terminate("duplicate application name definition");

	    }
	  else if (tbx_argit_vopt_equals("--flavor"))
	    {
	      if (!settings->flavor)
		{
		  settings->flavor = tbx_argit_vopt_value_cstr();
		}
	      else
		leo_terminate("duplicate application name definition");
	    }
	  else if (tbx_argit_vopt_equals("--net"))
	    {
	      char *network_file = NULL;

	      network_file = tbx_argit_vopt_value_cstr();

	      if (!settings->network_file_slist)
		{
		  settings->network_file_slist = tbx_slist_nil();
		}

	      {
		p_leoparse_object_t  object = NULL;
		char                *base   = NULL;
		char                *ptr    = NULL;
		char                *file   = NULL;

		base = network_file;

		while ((ptr = strchr(network_file, ',')))
		  {
		    *ptr   = '\0';
		    file   = tbx_strdup(base);
		    object = leoparse_init_id_object(file, NULL);
		    file = NULL;
		    tbx_slist_append(settings->network_file_slist, object);
		    base = ptr + 1;
		  }

		file   = tbx_strdup(base);
		object = leoparse_init_id_object(file, NULL);
		file   = NULL;
		tbx_slist_append(settings->network_file_slist, object);
		TBX_FREE(network_file);
	      }
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
	      settings->pause_mode  = tbx_true;

	      settings->gdb_mode    = tbx_false;
	      settings->log_mode    = tbx_false;
	    }
	  else if (tbx_argit_arg_equals("--p"))
	    {
	      settings->pause_mode  = tbx_false;
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
	      settings->gdb_mode    = tbx_false;
	    }
	  else if (tbx_argit_arg_equals("-smp"))
	    {
	      settings->smp_mode    = tbx_true;
	    }
	  else if (tbx_argit_arg_equals("--smp"))
	    {
	      settings->smp_mode    = tbx_false;
	    }
	  else if (tbx_argit_arg_equals("-e"))
	    {
	      settings->export_mode = tbx_true;
	    }
	  else if (tbx_argit_arg_equals("--e"))
	    {
	      settings->export_mode = tbx_false;
	    }
	  else if (tbx_argit_arg_equals("-w"))
	    {
	      settings->wait_mode = tbx_true;
	    }
	  else if (tbx_argit_arg_equals("--w"))
	    {
	      settings->wait_mode = tbx_false;
	    }
	  else
	    tbx_argit_invalid_arg();

	}
      while (tbx_argit_next_opt());
    }

  settings->config_file = tbx_argit_arg_cstr();

  if (tbx_argit_next_arg())
    {
      p_tbx_arguments_t args = NULL;

      args = tbx_arguments_init();

      do
	{
	  char *arg = NULL;

	  arg = tbx_argit_arg_cstr();
	  tbx_arguments_append_cstring(args, arg);
	}
      while (tbx_argit_next_arg());

      settings->args = args;
    }

  LOG_OUT();

  return settings;
}

static
p_leonie_t
init_leonie_object(int    argc,
		   char **argv)
{
  p_leonie_t leonie = NULL;

  LOG_IN();
  leonie = leonie_init();

  TRACE("== Registering loaders");
  leonie->loaders = leo_loaders_register();

  TRACE("== Processing command line");
  leonie->settings = process_command_line(argc, argv);

  TRACE("== Initializing internal structures");
  leonie->net_server    = leo_net_server_init();
  leonie->networks      = leo_networks_init();
  leonie->directory     = leo_directory_init();
  leonie->spawn_groups  = leo_spawn_groups_init();
  LOG_OUT();

  return leonie;
}

void
parse_application_file(p_leonie_t leonie)
{
  p_leo_settings_t settings                = NULL;
  p_tbx_htable_t   application_file_htable = NULL;

  LOG_IN();
  settings                   = leonie->settings;
  application_file_htable    =
    leoparse_parse_local_file(settings->config_file);
  leonie->application_htable =
    leoparse_extract_htable(application_file_htable, "application");

  if (!tbx_htable_empty(application_file_htable))
    FAILURE("unexpected datas in application configuration file");

  tbx_htable_free(application_file_htable);
  application_file_htable = NULL;
  LOG_OUT();
}

int
main(int    argc,
     char **argv)
{
  p_leonie_t leonie = NULL;

  LOG_IN();
  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);

  TRACE("== Initializing parser");
  leoparse_init(argc, argv);
  leoparse_purge_cmd_line(&argc, argv);
  TRACE("== Initializing root data structure");
  leonie = init_leonie_object(argc, argv);
  TRACE("== Parsing configuration file");
  parse_application_file(leonie);
  TRACE("== Processing configuration");
  process_application(leonie);

  TRACE("== Launching processes");
  spawn_processes(leonie);
  TRACE("== Transmitting directory");
  send_directory(leonie);

  TRACE("== Initializing drivers");
  init_drivers(leonie);
  TRACE("== Initializing channels");
  init_channels(leonie);
  init_fchannels(leonie);
  init_vchannels(leonie);
  init_xchannels(leonie);

  TRACE("== Standing by");
  exit_session(leonie);

  TRACE("== Disconnecting virtual channels");
  dir_vchannel_disconnect(leonie);

  TRACE("== Disconnecting multiplexing channels");
  dir_xchannel_disconnect(leonie);

  TRACE("== Freeing data structures");
  directory_exit(leonie);

  if (leonie->settings->pause_mode || leonie->settings->gdb_mode)
    {
      DISP("Session cleaned");
      getchar();
      leonie_processes_cleanup();
    }
  LOG_OUT();

  TRACE("== Leonie server shutdown");
  return 0;
}
