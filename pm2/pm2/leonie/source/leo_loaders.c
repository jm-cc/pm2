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
 * leonie_loaders.c
 * ================
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "leonie.h"

/*
 * Leonie loaders general modes
 * ============================
 */

#define LEO_SILENT_MODE
// #define LEO_DETACH_MODE
// #define LEO_DEBUG_MODE
// #define LEO_MAD_DEBUG_MODE
// #define LEO_TBX_DEBUG_MODE
// #define LEO_TRACE_MODE
// #define LEO_MARCEL_DEBUG_MODE
#define LEO_XMODE


/*
 * Leonie loaders general modes information
 * ========================================
 */

#ifdef LEO_SILENT_MODE
#warning [1;33m<<< [1;37mLeonie loaders silent mode:       [1;32mactivated [1;33m    >>>[0m
#else
#warning [1;33m<<< [1;37mLeonie loaders silent mode:       [1;31mnot activated [1;33m>>>[0m
#endif // LEO_SILENT_MODE

#ifdef LEO_DETACH_MODE
#warning [1;33m<<< [1;37mLeonie loaders detach mode:       [1;32mactivated [1;33m    >>>[0m
#else
#warning [1;33m<<< [1;37mLeonie loaders detach mode:       [1;31mnot activated [1;33m>>>[0m
#endif // LEO_DETACH_MODE

#ifdef LEO_DEBUG_MODE
#warning [1;33m<<< [1;37mLeonie loaders debug mode:        [1;32mactivated [1;33m    >>>[0m
#else
#warning [1;33m<<< [1;37mLeonie loaders debug mode:        [1;31mnot activated [1;33m>>>[0m
#endif // LEO_DEBUG_MODE

#ifdef LEO_MAD_DEBUG_MODE
#warning [1;33m<<< [1;37mLeonie loaders MAD debug mode:    [1;32mactivated [1;33m    >>>[0m
#else
#warning [1;33m<<< [1;37mLeonie loaders MAD debug mode:    [1;31mnot activated [1;33m>>>[0m
#endif // LEO_MAD_DEBUG_MODE

#ifdef LEO_TBX_DEBUG_MODE
#warning [1;33m<<< [1;37mLeonie loaders TBX debug mode:    [1;32mactivated [1;33m    >>>[0m
#else
#warning [1;33m<<< [1;37mLeonie loaders TBX debug mode:    [1;31mnot activated [1;33m>>>[0m
#endif // LEO_TBX_DEBUG_MODE

#ifdef LEO_MARCEL_DEBUG_MODE
#warning [1;33m<<< [1;37mLeonie loaders Marcel debug mode: [1;32mactivated [1;33m    >>>[0m
#else
#warning [1;33m<<< [1;37mLeonie loaders Marcel debug mode: [1;31mnot activated [1;33m>>>[0m
#endif // LEO_MARCEL_DEBUG_MODE

#ifdef LEO_TRACE_MODE
#warning [1;33m<<< [1;37mLeonie loaders trace mode:        [1;32mactivated [1;33m    >>>[0m
#else
#warning [1;33m<<< [1;37mLeonie loaders trace mode:        [1;31mnot activated [1;33m>>>[0m
#endif // LEO_TRACE_MODE

#ifdef LEO_XMODE
#warning [1;33m<<< [1;37mLeonie loaders Xwindow mode:      [1;32mactivated [1;33m    >>>[0m
#else
#warning [1;33m<<< [1;37mLeonie loaders Xwindow mode:      [1;31mnot activated [1;33m>>>[0m
#endif // LEO_XMODE


//
// PM2 scripts
// -----------
///
static const char *leo_rsh      =      NULL;
static tbx_bool_t  auto_display = tbx_false;

// ... Default loader .................................................. //
static
void
leo_default_loader(char              *application_name,
		   char              *application_flavor,
		   p_ntbx_server_t    net_server,
		   p_tbx_slist_t      process_slist)
{
  LOG_IN();
  TRACE("default loader selected");

  if (tbx_slist_is_nil(process_slist))
    return;
  
  tbx_slist_ref_to_head(process_slist);
      
  do
    {
      p_ntbx_process_t      process              = NULL;
      p_leo_dir_node_t      dir_node             = NULL;
      char                 *host_name            = NULL;
      p_tbx_string_t        main_command_string  = NULL;
      p_tbx_string_t        relay_command_string = NULL;
      p_tbx_string_t        env_command_string   = NULL;
      p_tbx_argument_set_t  arg_set              = NULL;
	  
      process   = tbx_slist_ref_get(process_slist);
      dir_node  = tbx_htable_get(process->ref, "node");
      host_name = dir_node->name;

      {
	p_tbx_command_t   main_command = NULL;
	p_tbx_arguments_t args         = NULL;
	    
	main_command = tbx_command_init_to_c_string(application_name);


	args = main_command->arguments;	    

	tbx_arguments_append_c_strings_option(args, "--mad_leonie", ' ',
					      net_server->local_host);

	tbx_arguments_append_c_strings_option(args, "--mad_link", ' ',
					      net_server->
					      connection_data.data);

#ifdef LEO_MAD_DEBUG_MODE
	tbx_arguments_append_c_strings_option(args, "--debug", ':',
					      "mad3-log");
#endif // LEO_MAD_DEBUG_MODE
#ifdef LEO_TBX_DEBUG_MODE
	tbx_arguments_append_c_strings_option(args, "--debug", ':',
					      "tbx-log");
	tbx_arguments_append_c_strings_option(args, "--debug", ':',
					      "ntbx-log");
#endif // LEO_TBX_DEBUG_MODE
#ifdef LEO_TRACE_MODE
	tbx_arguments_append_c_strings_option(args, "--debug", ':',
					      "mad3-trace");
#endif // LEO_TRACE_MODE
	
#ifdef LEO_MARCEL_DEBUG_MODE
	tbx_arguments_append_c_strings_option(args, "--debug", ':',
					      "marcel-log");
#endif // LEO_MARCEL_DEBUG_MODE
	
	main_command_string = tbx_command_to_string(main_command);

	tbx_command_free(main_command);
	main_command = NULL;
      }
      
      {
	p_tbx_command_t              relay_command = NULL;
	p_tbx_environment_t          env           = NULL;
	p_tbx_arguments_t            args          = NULL;
	p_tbx_environment_variable_t var           = NULL;

	relay_command = tbx_command_init_to_c_string("leo-load");
	env  = relay_command->environment;
	args = relay_command->arguments;

	var = tbx_environment_variable_to_variable("PATH");
	tbx_environment_variable_append_c_string(var, ':', "${PATH}");
	tbx_environment_append_variable(env, var);
	    
	var = tbx_environment_variable_to_variable("PM2_ROOT");
	tbx_environment_append_variable(env, var);
	    
	var = tbx_environment_variable_to_variable("LEO_XTERM");
	tbx_environment_append_variable(env, var);
	
	if (!auto_display)
	  {
	    var = tbx_environment_variable_to_variable("DISPLAY");
	    tbx_environment_append_variable(env, var);
	  }

#ifdef LEO_DEBUG_MODE
	tbx_arguments_append_c_strings_option(args, "-d", 0, NULL);
#endif // LEO_DEBUG_MODE
#ifdef LEO_XMODE
	tbx_arguments_append_c_strings_option(args, "-x", 0, NULL);
#endif // LEO_XMODE

	tbx_arguments_append_c_strings_option(args, "-f", ' ',
					      application_flavor);

	tbx_arguments_append_strings_option(args, main_command_string,
					    0, NULL);
	tbx_string_free(main_command_string);
	main_command_string = NULL;

	relay_command_string = tbx_command_to_string(relay_command);
	tbx_command_free(relay_command);
	relay_command = NULL;
      }
	  
      {
	p_tbx_command_t   env_command = NULL;
	p_tbx_arguments_t args        = NULL;
	    
	env_command = tbx_command_init_to_c_string("env");
	args = env_command->arguments;

	tbx_arguments_append_strings_option(args, relay_command_string,
					    0, NULL);
	tbx_string_free(relay_command_string);
	relay_command_string = NULL;

	env_command_string = tbx_command_to_string(env_command);
	tbx_command_free(env_command);
	env_command = NULL;
      }
	  
      {
	p_tbx_command_t   rsh_command = NULL;
	p_tbx_arguments_t args        = NULL;

	LOG_STR("leo_rsh", leo_rsh);
	rsh_command = tbx_command_init_to_c_string(leo_rsh);
	args = rsh_command->arguments;

	tbx_arguments_append_c_strings_option(args, host_name, 0, NULL);

	tbx_arguments_append_strings_option(args, env_command_string,
					    0, NULL);
	tbx_string_free(env_command_string);
	env_command_string = NULL;

	arg_set = tbx_command_to_argument_set(rsh_command);
	tbx_command_free(rsh_command);
	rsh_command = NULL;
      }

      {
	pid_t pid = -1;

	pid = fork();

	if (pid == -1)
	  {
	    leo_error("fork");
	  }

	if (!pid)
	  {
#if defined(LEO_DETACH_MODE) || defined(LEO_SILENT_MODE)
	    close(STDIN_FILENO);
	    close(STDOUT_FILENO);
	    close(STDERR_FILENO);
#endif // LEO_DETACH_MODE || LEO_SILENT_MODE

#ifdef LEO_DETACH_MODE
	    setpgrp();
#endif // LEO_DETACH_MODE

	    TRACE_STR("execvp command", arg_set->argv[0]);
	    execvp(arg_set->argv[0], arg_set->argv);
	    leo_error("execvp");
	  }
	
	process->pid = pid;
#if defined (LEO_DEBUG_MODE) || defined (LEO_XMODE)
	// Necessary to avoid Xlib multiple connection overflow
	sleep(2);
#endif // LEO_DEBUG_MODE / LEO_XMODE
      }
    }
  while (tbx_slist_ref_forward(process_slist));
  LOG_OUT();
}

static
void
leo_default_loader_register(p_tbx_htable_t loaders)
{
  p_leo_loader_t loader = NULL;

  LOG_IN();
  loader = leo_loader_init();

  loader->name        = strdup("default");
  loader->loader_func = leo_default_loader;

  tbx_htable_add(loaders, loader->name, loader);

  if (!(leo_rsh = getenv("LEO_RSH")))
    {
      leo_rsh = "rsh";
    }

  if (strstr(leo_rsh, "ssh"))
    {
      auto_display = tbx_true;
    }

  LOG_OUT();
}

// ... Loaders registration ............................................ //
p_tbx_htable_t
leo_loaders_register(void)
{
  p_tbx_htable_t loaders = NULL;
  
  LOG_IN();
  loaders = leo_htable_init();
  
  leo_default_loader_register(loaders);
  LOG_OUT();
  
  return loaders;
}
