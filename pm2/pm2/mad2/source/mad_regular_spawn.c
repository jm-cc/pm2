
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
#include <malloc.h>
#include <errno.h>
#include <netdb.h>
#include <sys/wait.h>

/* #define DEBUG */
/* #define TIMING */
#include "madeleine.h"

#include "pm2debug.h"

#ifdef REGULAR_SPAWN

/*
 * Constantes
 * ----------
 */
#define MAX_ARG_STR_LEN  1024
#define MAX_ARG_LEN       256

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
  int                    i;
  mad_adapter_id_t       ad;

  LOG_IN();
  if (configuration->local_host_id)
    {
      LOG_OUT();
      return;
    }
  
  cmd = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(cmd);

  arg_str = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(arg_str);

  arg = TBX_MALLOC(MAX_ARG_LEN);
  CTRL_ALLOC(arg);

  while (!(cwd = getcwd(NULL, MAX_ARG_LEN)))
    {    
      if (errno != EINTR)
	{
	  ERROR("getcwd");
	}
    }

  arg_str[0] = 0;

  /* 1: adapters' connection parameter */
  for (ad = 0; ad < madeleine->nb_adapter; ad++)
    {
      p_mad_adapter_t adapter = &(madeleine->adapter[ad]);

      sprintf(arg, " -device %s ", adapter->parameter);
      strcat (arg_str, arg);
    }

  /* 2: application specific args */
  for (i = 1; i < argc; i++)
    {
      sprintf(arg, " %s ", argv[i]);
      strcat(arg_str, arg);
    }

  if (settings->debug_mode)
    {
      char *display;
      
      prefix = TBX_MALLOC(MAX_ARG_LEN);
      CTRL_ALLOC(prefix);

      display = getenv("DISPLAY");

      if (!display)
	FAILURE("DISPLAY variable undefined");

      sprintf(prefix, "env DISPLAY=%s pm2_gdb_load", display);
    }
  else
    {
      prefix = TBX_MALLOC(1);
      CTRL_ALLOC(prefix);
      prefix[0] = 0;
    }

  for (i = 1; i < configuration->size; i++)
    {
      if (argv[0][0] != '/')
	{
	  sprintf(cmd,
		  "%s %s %s %s/%s -cwd %s -rank %d -conf %s %s &",
		  settings->rsh_cmd,
		  configuration->host_name[i],
		  prefix,
		  cwd,
		  argv[0],
		  cwd,
		  i,  /* rank */
		  settings->configuration_file,
		  arg_str);
	}
      else
	{
	  sprintf(cmd,
		  "%s %s %s %s -cwd %s -rank %d -conf %s %s &",
		  settings->rsh_cmd,
		  configuration->host_name[i],
		  prefix,
		  argv[0],
		  cwd,
		  i,  /* rank */
		  settings->configuration_file,
		  arg_str);
	}
	    
      LOG_STR("mad_init: Spawn", cmd);
      system(cmd);
    }

  TBX_FREE(prefix);
  TBX_FREE(cwd);
  TBX_FREE(cmd);
  TBX_FREE(arg_str);
  TBX_FREE(arg);  
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
#endif /* PM2_DEBUG */

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
