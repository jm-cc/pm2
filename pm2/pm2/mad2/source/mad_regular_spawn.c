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
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: mad_regular_spawn.c,v $
Revision 1.16  2000/11/16 11:11:49  rnamyst
Bug fixed in mad_purge_command_line + small changes in pm2-config (handling of 'common').

Revision 1.15  2000/11/10 14:17:57  oaumage
- nouvelle procedure d'initialisation

Revision 1.14  2000/10/30 10:29:41  oaumage
*** empty log message ***

Revision 1.13  2000/10/26 13:54:33  oaumage
- support de ssh

Revision 1.12  2000/09/12 15:29:19  oaumage
- correction -d

Revision 1.11  2000/09/12 14:55:14  rnamyst
Added support for generating .i files in Makefiles

Revision 1.10  2000/09/12 14:40:00  oaumage
- support -d

Revision 1.9  2000/09/12 09:39:39  oaumage
- correction des problemes de log

Revision 1.8  2000/07/12 07:55:14  oaumage
- Correction de la logique de localisation du fichier de configuration

Revision 1.7  2000/06/18 13:23:33  oaumage
- Correction de l'appel a mad_managers_init

Revision 1.6  2000/06/16 14:03:51  oaumage
- Mise a jour par rapport au nouveau fonctionnement de pm2conf

Revision 1.5  2000/06/16 13:47:49  oaumage
- Mise a jour des routines d'initialisation de Madeleine II
- Progression du code de mad_leonie_spawn.c

Revision 1.4  2000/06/15 08:45:03  rnamyst
pm2load/pm2conf/pm2logs are now handled by pm2.

Revision 1.3  2000/06/06 15:58:00  oaumage
- suppression des fonctions *exit redondantes

Revision 1.2  2000/05/25 00:23:38  vdanjean
marcel_poll with sisci and few bugs fixes

Revision 1.1  2000/05/17 14:34:15  oaumage
- Reorganisation des sources au niveau de mad_init


______________________________________________________________________________
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
#define MAX_HOSTNAME_LEN  256
#define MAX_ARG_STR_LEN  1024
#define MAX_ARG_LEN       256

/*
 * Objet Madeleine
 * ---------------
 */
static mad_madeleine_t main_madeleine;

/*
 * Initialisation des drivers
 * --------------------------
 */
static void 
mad_driver_init(p_mad_madeleine_t madeleine)
{
  mad_driver_id_t drv;
    
  LOG_IN();
  for (drv = 0;
       drv < madeleine->nb_driver;
       drv++)
    {
      p_mad_driver_t driver;
      
      driver = &(madeleine->driver[drv]);      
      driver->specific = NULL;
      driver->interface.driver_init(driver);
    }
  LOG_OUT();
}

p_mad_madeleine_t
mad_object_init(int                   argc,
		char                **argv,
		char                 *configuration_file,
		p_mad_adapter_set_t   adapter_set)
{
  p_mad_madeleine_t     madeleine     = &(main_madeleine);
  p_mad_settings_t      settings      = NULL;
  p_mad_configuration_t configuration = NULL;

  LOG_IN();
 
  /* Structure initialization */
  TBX_INIT_SHARED(madeleine);
  madeleine->nb_driver     =    0;
  madeleine->driver        = NULL;
  madeleine->nb_adapter    =    0;
  madeleine->adapter       = NULL;
  madeleine->nb_channel    =    0;
  tbx_list_init(&(madeleine->channel));
  madeleine->settings      = NULL;
  madeleine->configuration = NULL;

  settings = TBX_MALLOC(sizeof(mad_settings_t));
  CTRL_ALLOC(settings);

  settings->rsh_cmd            = NULL;
  settings->configuration_file = NULL;
  settings->debug_mode         = tbx_false;

  settings->rsh_cmd = getenv("PM2_RSH");
  if (!settings->rsh_cmd)
    {
      settings->rsh_cmd = "rsh";
    }  

  if (configuration_file)
    {
      settings->configuration_file = TBX_MALLOC(1 + strlen(configuration_file));
      CTRL_ALLOC(settings->configuration_file);
      strcpy(settings->configuration_file, configuration_file);
    }
  else if (getenv("PM2_CONF_FILE"))
    {
      settings->configuration_file =
	TBX_MALLOC(1 + strlen(getenv("PM2_CONF_FILE")));
      CTRL_ALLOC(settings->configuration_file);
      strcpy(settings->configuration_file, getenv("PM2_CONF_FILE"));
    }
  else
    {
      settings->configuration_file = NULL;
    }

  madeleine->settings = settings;
  
  configuration = TBX_MALLOC(sizeof(mad_configuration_t));
  CTRL_ALLOC(configuration);
  
  configuration->size          = 1;
  configuration->local_host_id = 0;
  configuration->host_name     = NULL;
  configuration->program_name  = NULL;
  
  madeleine->configuration = configuration;

  /* Network components pre-initialization */
  mad_driver_fill(madeleine);
  mad_adapter_fill(madeleine, adapter_set);

  LOG_OUT();
  
  return madeleine;
}

void
mad_cmd_line_init(p_mad_madeleine_t   madeleine,
		  int                 argc,
		  char              **argv)
{
  p_mad_adapter_t       adapter       = madeleine->adapter;
  p_mad_configuration_t configuration = madeleine->configuration;
  p_mad_settings_t      settings      = madeleine->settings;

  LOG_IN();

  argc--; argv++;

  while (argc)
    {
      if (!strcmp(*argv, "-d"))
	{
	  settings->debug_mode = tbx_true;
	}
      else if (!strcmp(*argv, "-rank"))
	{
	  argc--; argv++;

	  if (!argc)
	    FAILURE("rank argument not found");

	  configuration->local_host_id = atoi(*argv);
	}
      else if (!strcmp(*argv, "-conf"))
	{
	  argc--; argv++;

	  if (!argc)
	    FAILURE("conf argument not found");

	  if (settings->configuration_file)
	    {
	      TBX_FREE(settings->configuration_file);
	    }

	  settings->configuration_file = TBX_MALLOC(strlen(*argv) + 1);
	  CTRL_ALLOC(settings->configuration_file);
	      
	  strcpy(settings->configuration_file, *argv);
	}
      else if (!strcmp(*argv, "-device"))
	{
	  argc--; argv++;

	  if (!argc)
	    FAILURE("device argument not found");

	  adapter->master_parameter = TBX_MALLOC(strlen(*argv) + 1);
	  CTRL_ALLOC(adapter->master_parameter);
	  
	  strcpy(adapter->master_parameter, *argv);
	  adapter++;
	}
      else if (!strcmp(*argv, "-cwd"))
	{
	  argc--; argv++;

	  if (!argc)
	    FAILURE("cwd argument not found");

	  chdir(*argv);
	}

      argc--; argv++;
    }

  LOG_OUT();
}

void
mad_configuration_init(p_mad_madeleine_t   madeleine,
		       int                 argc,
		       char              **argv)
{
  p_mad_configuration_t  configuration = madeleine->configuration;
  p_mad_settings_t       settings      = madeleine->settings;
  FILE                  *f             = NULL;
  int                    i;

  LOG_IN();
  f = fopen(settings->configuration_file, "r");
    
  if (!f)
    {
      ERROR("fopen");
    }

  {
    char cmd[MAX_ARG_STR_LEN];
    int  ret;
    
    sprintf(cmd, "exit `cat %s | wc -w`", settings->configuration_file);
    ret = system(cmd);

    if (ret == -1)
      ERROR("system");
    
    configuration->size = WEXITSTATUS(ret);
  }

  configuration->host_name = TBX_MALLOC(configuration->size * sizeof(char *));
  CTRL_ALLOC(configuration->host_name);

   for (i = 0; i < configuration->size; i++)
    {
      configuration->host_name[i] = TBX_MALLOC(MAX_HOSTNAME_LEN);
      CTRL_ALLOC(configuration->host_name[i]);
      fscanf(f, "%s", configuration->host_name[i]);
    }

   fclose(f);
   LOG_OUT();
}

void
mad_output_redirection_init(p_mad_madeleine_t   madeleine,
			    int                 argc,
			    char              **argv)
{
  p_mad_configuration_t  configuration = madeleine->configuration;
  p_mad_settings_t       settings      = madeleine->settings;

  LOG_IN();
  if (configuration->local_host_id && !settings->debug_mode)
    {
      char output[MAX_ARG_LEN];
      int  f;

      sprintf(output,
	      "/tmp/%s-%s-%d",
	      getenv("USER"),
	      MAD2_LOGNAME, (int)configuration->local_host_id);

      f = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0600);
      if (f < 0)
	ERROR("open");
      
      if (dup2(f, STDOUT_FILENO) < 0)
	ERROR("dup2");

      if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0)
	ERROR("dup2");
    }
  LOG_OUT();
}

void
mad_network_components_init(p_mad_madeleine_t   madeleine,
			    int                 argc,
			    char              **argv)
{
  LOG_IN();
  mad_driver_init(madeleine);
  mad_adapter_init(madeleine);
  LOG_OUT();
}

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
		  "%s %s %s %s/%s -slave -cwd %s -rank %d -conf %s %s &",
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
		  "%s %s %s %s -slave -cwd %s -rank %d -conf %s %s &",
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

void  
mad_connect(p_mad_madeleine_t   madeleine,
	    int                 argc,
	    char              **argv)
{
  LOG_IN();
  mad_adapter_configuration_init(madeleine);
  LOG_OUT();
}

void
mad_purge_command_line(p_mad_madeleine_t   madeleine,
		       int                *_argc,
		       char              **_argv)
{
  int     argc = *_argc;
  char ** argv =  _argv;
  LOG_IN();

  argv++; _argv++; argc--;
  
  while (argc)
    {
      if (!strcmp(*_argv, "-d"))
	{
	  _argv++; (*_argc)--;
	}
      else if (!strcmp(*_argv, "-slave"))
	{
	  _argv++; (*_argc)--;
	}
      else if (!strcmp(*_argv, "-rank"))
	{
	  _argv++; (*_argc)--; argc--;

	  if (!argc)
	    FAILURE("rank argument disappeared");
	  
	  _argv++; (*_argc)--;

	}
      else if (!strcmp(*_argv, "-conf"))
	{
	  _argv++; (*_argc)--; argc--;

	  if (!argc)
	    FAILURE("conf argument disappeared");
	  
	  _argv++; (*_argc)--;
	}
      else if (!strcmp(*_argv, "-device"))
	{
	  _argv++; (*_argc)--; argc--;

	  if (!argc)
	    FAILURE("device argument disappeared");
	  
	  _argv++; (*_argc)--;
	}
      else if (!strcmp(*_argv, "-cwd"))
	{
	  _argv++; (*_argc)--; argc--;

	  if (!argc)
	    FAILURE("cwd argument disappeared");
	  
	  _argv++; (*_argc)--;
	}
      else
	{
	  *argv++ = *_argv++;
	}

      argc--;
    }
  
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

  LOG_OUT();

  return madeleine;
}

#endif /* REGULAR_SPAWN */
