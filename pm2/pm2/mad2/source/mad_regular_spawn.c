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



static void
mad_parse_command_line(int                *argc,
		       char              **argv,
		       p_mad_madeleine_t   madeleine,
		       char               *conf_file,
		       p_tbx_bool_t        master,
		       p_tbx_bool_t        slave,
		       p_tbx_bool_t        debug_mode)
{
  p_mad_adapter_t  current_adapter = madeleine->adapter;
  int              i;
  int              j;
  
  LOG_IN();
  i = j = 1;    
  
  while (i < (*argc))
    {
      if(!strcmp(argv[i], "-master"))
	{
	  *master = tbx_true;
	}
      else if(!strcmp(argv[i], "-slave"))
	{
	  *slave = tbx_true;
	}
      else if(!strcmp(argv[i], "-d"))
	{
	  *debug_mode = tbx_true;
	}
      else if(!strcmp(argv[i], "-rank"))
	{
	  if (i == ((*argc) - 1))
	    FAILURE("-rank option must be followed "
		    "by the rank of the process");

	  madeleine->configuration.local_host_id = atoi(argv[i + 1]);
	  i++;
	}
      else if (!strcmp(argv[i], "-conf"))
	{
	  if (i == ((*argc) - 1))
	    FAILURE("-conf must be followed "
		    "by the path of mad2 configuration file");

	  if (!conf_file)
	    FAILURE("configuration file already specified");
	  
	  sprintf(conf_file, "%s", argv[i + 1]);
	  i++;
	}
      else if (!strcmp(argv[i], "-device"))
	{
	  if(i == ((*argc) - 1))
	    FAILURE(" -device must be followed by a master device parameter");

	  current_adapter->master_parameter
	    = TBX_MALLOC(strlen(argv[i + 1]) + 1);
	  CTRL_ALLOC(current_adapter->master_parameter);
	    
	  strcpy(current_adapter->master_parameter,
		 argv[i + 1]);
	  LOG_STR("mad_init: master_parameter",
		  current_adapter->master_parameter);
	  current_adapter++;
	  i++;
	}
      else if (!strcmp(argv[i], "-cwd"))
	{
	  if(i == ((*argc) - 1))
	    FAILURE("-cwd must be followed "
		    "by the current working directory of the master process");
	  chdir(argv[i + 1]);
	  i++;
	}
      else
	{
	  argv[j++] = argv[i];
	}
      i++;
    }
  *argc = j;
  LOG_OUT();
}

static void
mad_master_spawn(int                    *argc,
		 char                  **argv,
		 p_mad_configuration_t   configuration,
		 tbx_bool_t              conf_spec,
		 char                   *configuration_file)
{
  /* Spawn the master mad2 process */
  int    i;
  char  *cmd     = NULL;
  char  *cwd     = NULL;
  char  *arg_str = NULL;
  char  *arg     = NULL;
  
  LOG_IN();
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
	  perror("getcwd");
	  exit(1);
	}
    }
  
  for (i = 1;
       i < *argc;
       i++)
    {
      sprintf(arg, " %s ", argv[i]);
      strcat(arg_str, arg);
    }

  if (argv[0][0] != '/')
    {
      if (!conf_spec)
	{ 
	  sprintf(cmd,
		  "rsh %s %s/%s -master -cwd %s -rank %d -conf %s %s",
		  configuration->host_name[0],
		  cwd, argv[0], cwd, 0, configuration_file, arg_str);
	}
      else
	{ 
	  sprintf(cmd,
		  "rsh %s %s/%s -master -cwd %s -rank %d %s",
		  configuration->host_name[0],
		  cwd, argv[0], cwd, 0, arg_str);
	}
    }
  else
    {
      if (!conf_spec)
	{
	  sprintf(cmd,
		  "rsh %s %s -master -rank %d -conf %s %s",
		  configuration->host_name[0],
		  argv[0], 0, configuration_file, arg_str);
	}
      else
	{ 
	  sprintf(cmd,
		  "rsh %s %s -master -rank %d %s",
		  configuration->host_name[0], argv[0], 0, arg_str);
	}
    }
  LOG_STR("Loader cmd", cmd);
  system(cmd);
  LOG_OUT();
  exit(EXIT_SUCCESS);  
}

static void
mad_slave_spawn(int                *argc,
		char              **argv,
		tbx_bool_t          conf_spec,
		p_mad_madeleine_t   madeleine,
		char               *configuration_file,
		tbx_bool_t          debug_mode)
{
  p_mad_configuration_t   configuration = &(madeleine->configuration);
  int                     i;
  char                   *cmd           = NULL;
  char                   *arg_str       = NULL;
  char                   *arg           = NULL;
  char                   *cwd           = NULL;
  char                   *prefix        = NULL;
  mad_adapter_id_t        ad;
	
  LOG_IN();
  cmd = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(cmd);
  arg_str = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(arg_str);
  arg = TBX_MALLOC(MAX_ARG_LEN);
  CTRL_ALLOC(arg);

  while (!(cwd = getcwd(NULL, MAX_ARG_LEN)))
    {    
      if(errno != EINTR)
	{
	  perror("getcwd");
	  FAILURE("syscall failed");
	}
    }

  arg_str[0] = 0;

  /* 1: adapters' connection parameter */
  for (ad = 0;
       ad < madeleine->nb_adapter;
       ad++)
    {
      p_mad_adapter_t adapter = &(madeleine->adapter[ad]);

      sprintf(arg,
	      " -device %s ",
	      adapter->parameter);
      strcat (arg_str, arg);
    }

  /* 2: application specific args */
  for (i = 1;
       i < *argc;
       i++)
    {
      sprintf(arg, " %s ", argv[i]);
      strcat(arg_str, arg);
    }

  if (debug_mode)
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

  for (i = 1;
       i < configuration->size;
       i++)
    {
      if (argv[0][0] != '/')
	{
	  if (!conf_spec)
	    {
	      sprintf(cmd,
		      "rsh %s %s %s/%s -slave -cwd %s -rank %d -conf %s %s &",
		      configuration->host_name[i],
		      prefix,
		      cwd,
		      argv[0],
		      cwd,
		      i,  /* rank */
		      configuration_file,
		      arg_str);
	    }
	  else
	    {
	      sprintf(cmd,
		      "rsh %s %s %s/%s -slave -cwd %s -rank %d %s &",
		      configuration->host_name[i],
		      prefix,
		      cwd,
		      argv[0],
		      cwd,
		      i,  /* rank */
		      arg_str);
	    }	  
	}
      else
	{
	  if (!conf_spec)
	    {
	      sprintf(cmd,
		      "rsh %s %s %s -slave -cwd %s -rank %d -conf %s %s &",
		      configuration->host_name[i],
		      prefix,
		      argv[0],
		      cwd,
		      i,  /* rank */
		      configuration_file,
		      arg_str);
	    }
	  else
	    {
	      
	      sprintf(cmd,
		      "rsh %s %s %s -slave -cwd %s -rank %d %s &",
		      configuration->host_name[i],
		      prefix,
		      argv[0],
		      cwd,
		      i,  /* rank */
		      arg_str);
	    }
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

static void
mad_connect_hosts(p_mad_madeleine_t   madeleine,
		  tbx_bool_t          conf_spec,
		  int                *argc,
		  char              **argv,
		  char               *configuration_file,
		  tbx_bool_t          debug_mode)
{
  LOG_IN();
  mad_adapter_init(madeleine);
  if (madeleine->configuration.local_host_id == 0)
    {
      mad_slave_spawn(argc,
		      argv,
		      conf_spec,
		      madeleine,
		      configuration_file,
		      debug_mode);
    }
  mad_adapter_configuration_init(madeleine);
  LOG_OUT();
}

/* ---- */
static void
mad_read_conf(p_mad_configuration_t   configuration,
	      char                   *configuration_file)
{
  /* configuration retrieval */
  FILE *f;
  int i;

  LOG_IN();
  LOG_STR("Configuration file", configuration_file);

  f = fopen(configuration_file, "r");
  if(f == NULL)
    {
      perror("configuration file");
      exit(1);
    }

    {
      char commande[MAX_ARG_STR_LEN];
      int ret;
      
      sprintf(commande, "exit `cat %s | wc -w`", configuration_file);

      ret = system(commande);

      configuration->size = WEXITSTATUS(ret);
    }

  configuration->host_name = TBX_MALLOC(configuration->size * sizeof(char *));
  CTRL_ALLOC(configuration->host_name);

  for (i = 0;
       i < configuration->size;
       i++)
    {
      configuration->host_name[i] = TBX_MALLOC(MAX_HOSTNAME_LEN);
      CTRL_ALLOC(configuration->host_name[i]);
      fscanf(f, "%s", configuration->host_name[i]);
    }

  fclose(f);
  LOG_OUT();
}

/* ---- */

#ifdef PM2
p_mad_madeleine_t
mad2_init(int                  *argc,
	  char                **argv,
	  char                 *configuration_file,
	  p_mad_adapter_set_t   adapter_set)
#else /* PM2 */
p_mad_madeleine_t
mad_init(
	 int                   *argc,
	 char                 **argv,
	 char                  *configuration_file,
	 p_mad_adapter_set_t    adapter_set
	 )
#endif /* PM2 */
{
  p_mad_madeleine_t          madeleine       = &(main_madeleine);
  p_mad_configuration_t      configuration   =
    &(madeleine->configuration);
  ntbx_host_id_t             rank            = -1;
  tbx_bool_t                 master          = tbx_false;
  tbx_bool_t                 slave           = tbx_false;
  char                       conf_file[128];
  tbx_bool_t                 conf_spec       = (configuration_file != NULL);
  tbx_bool_t                 debug_mode      = tbx_false;

  mad_managers_init(argc, argv);

  LOG_IN(); /* After pm2debug_init ... */
  
  if (!conf_spec)
    {
      if (getenv("PM2_CONF_FILE"))
	{
	 configuration_file = conf_file;
	 sprintf(configuration_file, "%s", getenv("PM2_CONF_FILE"));
       }  
    }
  
  madeleine->nb_channel = 0;
  TBX_INIT_SHARED(madeleine);
  mad_driver_fill(madeleine);
  mad_adapter_fill(madeleine, adapter_set);  

  if (configuration_file)
    {      
      mad_parse_command_line(argc,
			     argv,
			     madeleine,
			     NULL,
			     &master,
			     &slave,
			     &debug_mode);
    }
  else
    {
      configuration_file = conf_file;
      mad_parse_command_line(argc,
			     argv,
			     madeleine,
			     configuration_file,
			     &master,
			     &slave,
			     &debug_mode);
    }

  mad_read_conf(configuration, configuration_file);
  rank = configuration->local_host_id;  

  /* Uncomment to disable the first `rsh' step  */
  master = !slave; 
  
  if (!master && !slave)
    {
      mad_master_spawn(argc,
		       argv,
		       configuration,
		       conf_spec,
		       configuration_file);
    }

  /* output redirection */
  if ((rank > 0) && (! debug_mode))
    {
      char output[MAX_ARG_LEN];
      int  f;

      sprintf(output,
	      "/tmp/%s-%s-%d",
	      getenv("USER"),
	      MAD2_LOGNAME, (int)rank);

      f = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0600);
      dup2(f, STDOUT_FILENO);
      dup2(STDOUT_FILENO, STDERR_FILENO);
    }

  mad_driver_init(madeleine);
  mad_connect_hosts(madeleine,
		    conf_spec,
		    argc,
		    argv,
		    configuration_file,
		    debug_mode);
  tbx_list_init(&(madeleine->channel));
  pm2debug_init_ext(argc, argv, PM2DEBUG_CLEAROPT);

  LOG_OUT();
  return madeleine;
}

#endif /* REGULAR_SPAWN */
