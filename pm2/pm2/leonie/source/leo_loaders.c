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

/*
 * Leonie loaders general modes information
 * ========================================
 */

//
// PM2 scripts
// -----------
///
static const char *leo_rsh      =      NULL;
static tbx_bool_t  auto_display = tbx_false;

// ... Default loader .................................................. //
static
void
leo_default_loader(p_leo_application_settings_t settings,
		   p_ntbx_server_t              net_server,
		   p_tbx_slist_t                process_slist)
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

      /* Main command */
      {
	p_tbx_command_t     main_command = NULL;
	p_tbx_arguments_t   args         = NULL;
	p_tbx_environment_t env          = NULL;
	    
	main_command =
	  tbx_command_init_to_cstring(settings->name);
	env  = main_command->environment;
	args = main_command->arguments;

	if (settings->args)
	  {
	    tbx_arguments_append_arguments(args, settings->args);
	  }
	
	tbx_arguments_append_cstring_ext(args, "--mad_leonie", ' ',
						 net_server->local_host);

	tbx_arguments_append_cstring_ext(args, "--mad_link", ' ',
					      net_server->
					      connection_data.data);

	main_command_string = tbx_command_to_string(main_command);

	tbx_command_free(main_command);
	main_command = NULL;
      }

      /* Relay command */
      {
	p_tbx_command_t              relay_command = NULL;
	p_tbx_environment_t          env           = NULL;
	p_tbx_arguments_t            args          = NULL;
	p_tbx_environment_variable_t var           = NULL;

	relay_command = tbx_command_init_to_cstring("leo-load");
	env  = relay_command->environment;
	args = relay_command->arguments;

	var = tbx_environment_variable_to_variable("PATH");
	tbx_environment_variable_append_cstring(var, ':', "${PATH}");
	tbx_environment_append_variable(env, var);
	    
	var = tbx_environment_variable_to_variable("PM2_ROOT");
	tbx_environment_append_variable(env, var);
	    
	var = tbx_environment_variable_to_variable("LEO_XTERM");
	tbx_environment_append_variable(env, var);

	if (settings->smp_mode)
	  {
	    var =
	      tbx_environment_variable_init_to_cstrings("LEO_LD_PRELOAD",
							"${HOME}/lib/libpthread.so");
	    tbx_environment_append_variable(env, var);
	  }
	
	if (!auto_display)
	  {
	    var = tbx_environment_variable_to_variable("DISPLAY");
	    tbx_environment_append_variable(env, var);
	  }

	if (settings->gdb_mode)
	  {
	    tbx_arguments_append_cstring(args, "-d");
	  }

	if (settings->xterm_mode)
	  {
	    tbx_arguments_append_cstring(args, "-x");
	  }
	
	tbx_arguments_append_cstring_ext(args, "-f", ' ', settings->flavor);

	tbx_arguments_append_string(args, main_command_string);
	tbx_string_free(main_command_string);
	main_command_string = NULL;

	relay_command_string = tbx_command_to_string(relay_command);
	tbx_command_free(relay_command);
	relay_command = NULL;
      }
	  
      /* Relay command environment */
      {
	p_tbx_command_t   env_command = NULL;
	p_tbx_arguments_t args        = NULL;
	    
	env_command = tbx_command_init_to_cstring("env");
	args = env_command->arguments;

	tbx_arguments_append_string(args, relay_command_string);
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
	rsh_command = tbx_command_init_to_cstring(leo_rsh);
	args = rsh_command->arguments;

	if (strstr(leo_rsh, "ssh"))
	  {
	    // tbx_arguments_append_cstring(args, "-f");
	    tbx_arguments_append_cstring(args, "-n");
	  }

	tbx_arguments_append_cstring(args, host_name);

	tbx_arguments_append_string(args, env_command_string);
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
	    {
	      int i = 0;

	      while (arg_set->argv[i])
		{
		  TRACE_STR("execvp command", arg_set->argv[i]);
		  i++;
		}
	    }

	    TRACE_STR("execvp command", arg_set->argv[0]);
	    execvp(arg_set->argv[0], arg_set->argv);
	    leo_error("execvp");
	  }	    
	
	process->pid = pid;

	if (settings->xterm_mode)
	  {
	    // Necessary to avoid Xlib multiple connection overflow
	    sleep(2);
	  }
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
  LOG_OUT();
}

// ... BIP loader ...................................................... //
static
void
leo_bipload_loader(p_leo_application_settings_t settings,
		   p_ntbx_server_t              net_server,
		   p_tbx_slist_t                process_slist)
{
  char *master_host_name = NULL;

  LOG_IN();
  TRACE("default loader selected");

  if (tbx_slist_is_nil(process_slist))
    return;
  
  /*
   * bipconf step
   * ------------
   */
  {
    p_tbx_string_t        relay_command_string = NULL;
    p_tbx_string_t        env_command_string   = NULL;
    p_tbx_argument_set_t  arg_set              = NULL;
    
    {
      p_tbx_command_t              relay_command = NULL;
      p_tbx_environment_t          env           = NULL;
      p_tbx_arguments_t            args          = NULL;
      p_tbx_environment_variable_t var           = NULL;

      relay_command = tbx_command_init_to_cstring("bipconf");
      env  = relay_command->environment;
      args = relay_command->arguments;

      var = tbx_environment_variable_to_variable("PATH");
      tbx_environment_variable_append_cstring(var, ':', "${PATH}");
      tbx_environment_append_variable(env, var);
	    
      tbx_slist_ref_to_head(process_slist);
      do
	{
	  p_ntbx_process_t  process   = NULL;
	  p_leo_dir_node_t  dir_node  = NULL;
	  char             *host_name = NULL;
	  
	  process   = tbx_slist_ref_get(process_slist);
	  dir_node  = tbx_htable_get(process->ref, "node");
	  host_name = dir_node->name;      

	  if (!master_host_name)
	    {
	      master_host_name = host_name;
	    }

	  host_name = strdup(host_name);
	    
	  {
	    char *ptr = NULL;
	      
	    ptr = strchr(host_name, '.');
	    if (ptr)
	      {
		*ptr = '\0';
	      }
	  }
	    
	  tbx_arguments_append_cstring(args, host_name);
	  free(host_name);
	  host_name = NULL;
	}
      while (tbx_slist_ref_forward(process_slist));

      relay_command_string = tbx_command_to_string(relay_command);
      tbx_command_free(relay_command);
      relay_command = NULL;
    }
	  
    {
      p_tbx_command_t   env_command = NULL;
      p_tbx_arguments_t args        = NULL;
	    
      env_command = tbx_command_init_to_cstring("env");
      args = env_command->arguments;

      tbx_arguments_append_string(args, relay_command_string);
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
      rsh_command = tbx_command_init_to_cstring(leo_rsh);
      args = rsh_command->arguments;

      if (strstr(leo_rsh, "ssh"))
	{
	  // tbx_arguments_append_cstring(args, "-f");
	  tbx_arguments_append_cstring(args, "-n");
	}

      tbx_arguments_append_cstring(args, master_host_name);
      tbx_arguments_append_string(args, env_command_string);
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
	  {
	    int i = 0;

	    while (arg_set->argv[i])
	      {
		TRACE_STR("execvp command", arg_set->argv[i]);
		i++;
	      }
	  }
	  execvp(arg_set->argv[0], arg_set->argv);
	  leo_error("execvp");
	}	
      else
	{
	  pid_t result;
	  int   status;

	  result = waitpid(pid, &status, 0);

	  if (result == -1)
	    {
	      leo_error("waitpid");
	    }

	  if (WIFEXITED(status))
	    {
	      int code = 0;
		
	      code = WEXITSTATUS(status);
	      if (code)
		{
		  leo_terminate("bipconf failed");
		}
	    }
	  else
	    FAILURE("unexpected behaviour");
	}
    }
  }
  
  /*
   * bipload step
   * ------------
   */
      
  {
    p_tbx_string_t       main_command_string  = NULL;
    p_tbx_string_t       relay_command_string = NULL;
    p_tbx_string_t       env_command_string   = NULL;
    p_tbx_argument_set_t arg_set              = NULL;
	  
    {
      p_tbx_command_t   main_command = NULL;
      p_tbx_arguments_t args         = NULL;
	    
      main_command = tbx_command_init_to_cstring(settings->name);

      args = main_command->arguments;	    

      if (settings->args)
	{
	  tbx_arguments_append_arguments(args, settings->args);
	}

      tbx_arguments_append_cstring_ext(args, "--mad_leonie", ' ',
				       net_server->local_host);

      tbx_arguments_append_cstring_ext(args, "--mad_link", ' ',
				       net_server->
				       connection_data.data);

	
      main_command_string = tbx_command_to_string(main_command);

      tbx_command_free(main_command);
      main_command = NULL;
    }
      
    {
      p_tbx_command_t              relay_command = NULL;
      p_tbx_environment_t          env           = NULL;
      p_tbx_arguments_t            args          = NULL;
      p_tbx_environment_variable_t var           = NULL;

      relay_command = tbx_command_init_to_cstring("bipload");
      env  = relay_command->environment;
      args = relay_command->arguments;

      var = tbx_environment_variable_to_variable("PATH");
      tbx_environment_variable_append_cstring(var, ':', "${PATH}");
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

      if (settings->smp_mode)
	{
	  var =
	    tbx_environment_variable_init_to_cstrings("LEO_LD_PRELOAD",
						      "${HOME}/lib/libpthread.so");
	  tbx_environment_append_variable(env, var);
	}
	
	
      //	tbx_arguments_append_cstring(args, "-v");
      tbx_arguments_append_cstring(args, "-mget");
      tbx_arguments_append_cstring(args, "-hwflow");
      tbx_arguments_append_cstring(args, "leo-load");

      if (settings->gdb_mode)
	{
	  tbx_arguments_append_cstring(args, "-d");
	}

      if (settings->xterm_mode)
	{
	  tbx_arguments_append_cstring(args, "-x");
	}
	
      tbx_arguments_append_cstring_ext(args, "-f", ' ', settings->flavor);

      tbx_arguments_append_string(args, main_command_string);
      tbx_string_free(main_command_string);
      main_command_string = NULL;

      relay_command_string = tbx_command_to_string(relay_command);
      tbx_command_free(relay_command);
      relay_command = NULL;
    }
	  
    {
      p_tbx_command_t   env_command = NULL;
      p_tbx_arguments_t args        = NULL;
	    
      env_command = tbx_command_init_to_cstring("env");
      args = env_command->arguments;

      tbx_arguments_append_string(args, relay_command_string);
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
      rsh_command = tbx_command_init_to_cstring(leo_rsh);
      args = rsh_command->arguments;

      if (strstr(leo_rsh, "ssh"))
	{
	  // tbx_arguments_append_cstring(args, "-f");
	  tbx_arguments_append_cstring(args, "-n");
	}

      tbx_arguments_append_cstring(args, master_host_name);
      tbx_arguments_append_string(args, env_command_string);
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
	  {
	    int i = 0;

	    while (arg_set->argv[i])
	      {
		TRACE_STR("execvp command", arg_set->argv[i]);
		i++;
	      }
	  }
	    
	  execvp(arg_set->argv[0], arg_set->argv);
	  leo_error("execvp");
	}
	
      tbx_slist_ref_to_head(process_slist);
      do
	{
	  p_ntbx_process_t process = NULL;
	  
	  process      = tbx_slist_ref_get(process_slist);
	  process->pid = pid;
	}
      while (tbx_slist_ref_forward(process_slist));

      if (settings->xterm_mode)
	{
	  // Necessary to avoid Xlib multiple connection overflow
	  sleep(2);
	}
    }
  }

  LOG_OUT();
}

static
void
leo_bipload_loader_register(p_tbx_htable_t loaders)
{
  p_leo_loader_t loader = NULL;

  LOG_IN();
  loader = leo_loader_init();

  loader->name        = strdup("bipload");
  loader->loader_func = leo_bipload_loader;

  tbx_htable_add(loaders, loader->name, loader);

  LOG_OUT();
}

// ... Loaders registration ............................................ //
p_tbx_htable_t
leo_loaders_register(void)
{
  p_tbx_htable_t loaders = NULL;
  
  LOG_IN();
  if (!(leo_rsh = getenv("LEO_RSH")))
    {
      leo_rsh = "rsh";
    }

  if (strstr(leo_rsh, "ssh"))
    {
      auto_display = tbx_true;
    }

  loaders = leo_htable_init();
  
  leo_default_loader_register(loaders);
  leo_bipload_loader_register(loaders);
  LOG_OUT();
  
  return loaders;
}
