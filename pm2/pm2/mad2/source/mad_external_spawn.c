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
$Log: mad_external_spawn.c,v $
Revision 1.2  2000/05/25 00:23:38  vdanjean
marcel_poll with sisci and few bugs fixes

Revision 1.1  2000/05/17 14:34:15  oaumage
- Reorganisation des sources au niveau de mad_init


______________________________________________________________________________
*/

/*
 * Mad_external_spawn.c
 * ====================
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
#ifdef PM2
#include <sys/wait.h>
#endif /* PM2 */

/* #define DEBUG */
/* #define TIMING */
#include "madeleine.h"

#ifdef EXTERNAL_SPAWN
/*
 * Constantes
 * ----------
 */
#define MAX_ARG_LEN 256

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
      
      if (drv == EXTERNAL_SPAWN)
	continue;
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
		       p_tbx_bool_t        slave)
{
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
		    "by the path of mad2 root directory");

	  if (!conf_file)
	    FAILURE("configuration file already specified");
	  
	  sprintf(conf_file, "%s/.mad2_conf", argv[i + 1]);
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
  pm2debug_init_ext(argc, argv, PM2DEBUG_CLEAROPT);
  LOG_OUT();
}

static void
mad_connect_hosts(p_mad_madeleine_t   madeleine)
{
  p_mad_driver_interface_t spawn_interface;
  p_mad_adapter_t          spawn_adapter;
  mad_adapter_id_t ad;
  ntbx_host_id_t rank;

  LOG_IN();
  spawn_adapter   = &(madeleine->adapter[0]);
  spawn_interface = &(madeleine->driver[EXTERNAL_SPAWN].interface);
  rank            = madeleine->configuration.local_host_id;

  for (ad = 1;
       ad < madeleine->nb_adapter;
       ad++)
    {
      p_mad_adapter_t adapter;
      
      adapter = &(madeleine->adapter[ad]);
      adapter->specific = NULL;      

      if (adapter->driver->interface.adapter_init)
	adapter->driver->interface.adapter_init(adapter);
      if (!rank)
	{
	  ntbx_host_id_t host_id;

	  for (host_id = 1;
	       host_id < madeleine->configuration.size;
	       host_id++)
	    {
	      spawn_interface->send_adapter_parameter(spawn_adapter,
						      host_id,
						      adapter->parameter);
	    }
	}
      else
	{
	  spawn_interface->receive_adapter_parameter(spawn_adapter,
						     &(adapter->
						       master_parameter));
	}
      if (adapter->driver->interface.adapter_configuration_init)
	adapter->driver->interface.adapter_configuration_init(adapter);
    }
  LOG_OUT();
}

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
	 char                  *configuration_file __attribute__ ((unused)),
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
  p_mad_driver_t             spawn_driver    = NULL;
  p_mad_driver_interface_t   spawn_interface = NULL;
  p_mad_adapter_t            spawn_adapter   = NULL;
  char                       conf_file[128];
  tbx_bool_t                 conf_spec = tbx_false;

  if (!configuration_file)
    {    
      configuration_file = conf_file;
      sprintf(conf_file, "%s/.mad2_conf", mad_get_mad_root());
      conf_spec = tbx_true;
    }  

#ifdef MARCEL  
  marcel_init_ext(argc, argv, PM2DEBUG_DO_OPT);
#endif /* MARCEL */
  tbx_init(argc, argv, PM2DEBUG_DO_OPT);
  LOG_IN();  /* After pm2debug_init ... */
  
  madeleine->nb_channel = 0;
  TBX_INIT_SHARED(madeleine);
  mad_managers_init();
  mad_driver_fill(madeleine);
  mad_adapter_fill(madeleine, adapter_set);  

  spawn_driver = &(madeleine->driver[EXTERNAL_SPAWN]);
  spawn_interface = &(madeleine->driver[EXTERNAL_SPAWN].interface);
  spawn_driver->specific = NULL;
  spawn_interface->driver_init(spawn_driver);
  if (adapter_set)
    {
      spawn_adapter = &(madeleine->adapter[0]);
    }
  else
    {
      spawn_adapter = &(madeleine->adapter[EXTERNAL_SPAWN]);
    }  
  spawn_adapter->specific = NULL;
  if (spawn_interface->adapter_init)
    spawn_interface->adapter_init(spawn_adapter);
  if (spawn_interface->external_spawn_init)
    spawn_interface->external_spawn_init(spawn_adapter, argc, argv);

  if (conf_spec)
    {      
      mad_parse_command_line(argc,
			     argv,
			     madeleine,
			     conf_file,
			     &master,
			     &slave);
    }
  else
    {
      mad_parse_command_line(argc,
			     argv,
			     madeleine,
			     NULL,
			     &master,
			     &slave);
    }

  spawn_interface->configuration_init(spawn_adapter, configuration);
  if (spawn_interface->adapter_configuration_init)
    spawn_interface->adapter_configuration_init(spawn_adapter);
  rank = configuration->local_host_id;

  if (rank == 0)
    {
      master = tbx_true;
      slave  = tbx_false;
    }
  else
    {
      master = tbx_false;
      slave  = tbx_true;
    }

  /* output redirection */
  if (rank > 0)
    {
      char   output[MAX_ARG_LEN];
      int    f;

      sprintf(output, "/tmp/%s-mad2log-%d", getenv("USER"), (int)rank);
      f = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0600);
      dup2(f, STDOUT_FILENO);
      dup2(STDOUT_FILENO, STDERR_FILENO);
    }

  mad_connect_hosts(madeleine);
  tbx_list_init(&(madeleine->channel));

  LOG_OUT();
  return madeleine;
}

#endif /* EXTERNAL_SPAWN */
