
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
 * Mad_regular_spawn.c
 * ===================
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/wait.h>

/* #define DEBUG */
/* #define TIMING */
#include "madeleine.h"

#include "tbx_debug.h"

#ifdef REGULAR_SPAWN

/*
 * Constantes
 * ----------
 */
#define MAX_ARG_STR_LEN  2048
#define MAX_ARG_LEN       512

void
mad_slave_spawn(p_mad_madeleine_t   madeleine,
		int                 argc,
		char              **argv)
{
  p_mad_configuration_t  configuration = madeleine->configuration;
  p_mad_settings_t       settings      = madeleine->settings;
  char                  *cmd           = NULL;
  char                  *arg_str       = NULL;
  char                  *arg           = NULL;
  char                  *cwd           = NULL;
  char                  *prefix        = NULL;
  char                  *exec          = NULL;
  int                    i;
  mad_adapter_id_t       ad;

  LOG_IN();
  if (configuration->local_host_id)
    {
      LOG_OUT();
      return;
    }

  if (settings->app_cmd)
    {
      exec = settings->app_cmd;
    }
  else
    {
      char *s = getenv("PM2_CMD_NAME");

      if(s)
	exec = s;
      else
	exec = argv[0];
    }


  /////////////////////////////

  cmd = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(cmd);

  arg_str = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(arg_str);

  arg = TBX_MALLOC(MAX_ARG_LEN);
  CTRL_ALLOC(arg);

  cwd = TBX_MALLOC(MAX_ARG_LEN);
  CTRL_ALLOC(cwd);

  while (!(cwd = getcwd(cwd, MAX_ARG_LEN)))
    {
      if (errno != EINTR)
	{
	  TBX_ERROR("getcwd");
	}
    }

  /* pré-script de lancement */
  prefix = getenv("PM2_CMD_PREFIX");

  if (!prefix)
    TBX_FAILURE("PM2_CMD_PREFIX variable undefined");

  arg_str[0] = '\0';

  /* 1: adapters' connection parameter */
  for (ad = 0; ad < madeleine->nb_adapter; ad++)
    {
      p_mad_adapter_t adapter = &(madeleine->adapter[ad]);

      sprintf(arg, " --mad-device %s ", adapter->parameter);
      strcat (arg_str, arg);
    }

  /* 2: application specific args */
  for (i = 1; i < argc; i++)
    {
      sprintf(arg, " %s ", argv[i]);
      strcat(arg_str, arg);
    }

  for (i = 1; i < configuration->size; i++)
    {
#if 0
      if (exec[0] != '/')
	{
	  sprintf(cmd,
		  "%s %s %s %s/%s --mad-cwd %s --mad-rank %d --mad-conf %s %s &",
		  settings->rsh_cmd,
		  configuration->host_name[i],
		  prefix,
		  cwd,
		  exec,
		  cwd,
		  i,  /* rank */
		  settings->configuration_file,
		  arg_str);
	}
      else
#endif
	{
	  char *s = getenv("PM2_USE_LOCAL_FLAVOR");

	  if(s && !strcmp(s,"on")) {
	    sprintf(cmd,
		    "%s %s %s %s --mad-cwd %s --mad-rank %d %s &",
		    settings->rsh_cmd,
		    configuration->host_name[i],
		    prefix,
		    exec,
		    cwd,
		    i,  /* rank */
		    arg_str);
	  } else {
	    sprintf(cmd,
		    "%s %s %s %s --mad-cwd %s --mad-rank %d --mad-conf %s %s &",
		    settings->rsh_cmd,
		    configuration->host_name[i],
		    prefix,
		    exec,
		    cwd,
		    i,  /* rank */
		    settings->configuration_file,
		    arg_str);
	  }
	}

      LOG_STR("mad_init: Spawn", cmd);

      system(cmd);
    }

  tbx_safe_malloc_check(tbx_safe_malloc_VERBOSE);

  TBX_FREE(cwd);
  TBX_FREE(cmd);
  TBX_FREE(arg_str);
  TBX_FREE(arg);

  exec = NULL;

  LOG_OUT();
}

p_mad_madeleine_t
mad_init(int                  *argc,
	 char                **argv,
	 char                 *configuration_file,
	 p_mad_adapter_set_t   adapter_set)
{
  p_mad_madeleine_t madeleine = NULL;
  
  LOG_IN();

#ifdef PM2DEBUG
  pm2debug_init_ext(argc, argv, PM2DEBUG_DO_OPT);  
#endif /* PM2DEBUG */

  tbx_init(*argc, argv);
  ntbx_init(*argc, argv);
  
  mad_memory_manager_init(*argc, argv);

  madeleine = mad_object_init(*argc, argv, configuration_file, adapter_set);

  mad_cmd_line_init(madeleine, *argc, argv);
  mad_output_redirection_init(madeleine, *argc, argv);
  mad_configuration_init(madeleine, *argc, argv);
  mad_network_components_init(madeleine, *argc, argv);

  mad_purge_command_line(madeleine, argc, argv);

  mad_slave_spawn(madeleine, *argc, argv);
  mad_connect(madeleine, *argc, argv);

  ntbx_purge_cmd_line(argc, argv);
  tbx_purge_cmd_line(argc, argv);

#ifdef PM2DEBUG
  pm2debug_init_ext(argc, argv, PM2DEBUG_CLEAROPT);  
#endif /* PM2DEBUG */

  LOG_OUT();

  return madeleine;
}

#endif /* REGULAR_SPAWN */
