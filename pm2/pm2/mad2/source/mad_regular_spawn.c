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
Revision 1.18  2000/12/11 08:31:18  oaumage
- support Leonie

Revision 1.17  2000/11/16 13:24:07  oaumage
- mise a jour initialisation

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

#ifdef PM2_DEBUG
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

#ifdef PM2_DEBUG
  pm2debug_init_ext(argc, argv, PM2DEBUG_CLEAROPT);  
#endif /* PM2_DEBUG */

  LOG_OUT();

  return madeleine;
}

#endif /* REGULAR_SPAWN */
