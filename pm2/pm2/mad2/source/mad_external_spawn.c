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
Revision 1.12  2000/11/16 14:21:51  oaumage
- correction external spawn

Revision 1.11  2000/11/16 14:09:13  oaumage
- corrections diverses

Revision 1.10  2000/11/16 14:01:50  oaumage
- correction external spawn

Revision 1.9  2000/11/16 13:56:41  oaumage
- correction external spawn

Revision 1.8  2000/11/16 13:24:06  oaumage
- mise a jour initialisation

Revision 1.7  2000/07/12 07:55:14  oaumage
- Correction de la logique de localisation du fichier de configuration

Revision 1.6  2000/07/10 14:25:45  oaumage
- Correction de l'initialisation des objets connection

Revision 1.5  2000/06/18 13:23:33  oaumage
- Correction de l'appel a mad_managers_init

Revision 1.4  2000/06/16 13:47:47  oaumage
- Mise a jour des routines d'initialisation de Madeleine II
- Progression du code de mad_leonie_spawn.c

Revision 1.3  2000/06/15 08:45:03  rnamyst
pm2load/pm2conf/pm2logs are now handled by pm2.

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
#include <sys/wait.h>

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
/* static mad_madeleine_t main_madeleine; */

/*
 * Initialisation des drivers
 * --------------------------
 */
/*
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
*/

/*
#ifdef PM2
p_mad_madeleine_t
mad2_init(int                  *argc,
	  char                **argv,
	  char                 *configuration_file TBX_UNUSED,
	  p_mad_adapter_set_t   adapter_set)
#else
p_mad_madeleine_t
mad_init(
	 int                   *argc,
	 char                 **argv,
	 char                  *configuration_file TBX_UNUSED,
	 p_mad_adapter_set_t    adapter_set
	 )
#endif
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

  mad_managers_init(argc, argv);
  LOG_IN();  
  
  madeleine->nb_channel = 0;
  TBX_INIT_SHARED(madeleine);
  mad_managers_init(argc, argv);
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
  mad_driver_init(madeleine);
  if (spawn_interface->adapter_init)
    spawn_interface->adapter_init(spawn_adapter);
  if (spawn_interface->external_spawn_init)
    spawn_interface->external_spawn_init(spawn_adapter, argc, argv);

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

  if (rank > 0)
    {
      char   output[MAX_ARG_LEN];
      int    f;

      sprintf(output, "/tmp/%s-%s-%d", getenv("USER"), MAD2_LOGNAME, (int)rank);
      f = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0600);
      dup2(f, STDOUT_FILENO);
      dup2(STDOUT_FILENO, STDERR_FILENO);
    }

  mad_connect_hosts(madeleine);
  tbx_list_init(&(madeleine->channel));

  LOG_OUT();
  return madeleine;
}
*/

void
mad_spawn_driver_init(p_mad_madeleine_t   madeleine,
		      int                *argc,
		      char              **argv)
{
  p_mad_configuration_t    configuration   = madeleine->configuration;
  p_mad_driver_t           spawn_driver    = NULL;
  p_mad_driver_interface_t spawn_interface = NULL;
  p_mad_adapter_t          spawn_adapter   = NULL;

  LOG_IN();
  spawn_driver           = &(madeleine->driver[EXTERNAL_SPAWN]);
  spawn_interface        = &(madeleine->driver[EXTERNAL_SPAWN].interface);
  spawn_interface->driver_init(spawn_driver);
  
  spawn_adapter = &(madeleine->adapter[EXTERNAL_SPAWN]);
  
  if (spawn_interface->adapter_init)
    spawn_interface->adapter_init(spawn_adapter);

  if (spawn_interface->external_spawn_init)
    spawn_interface->external_spawn_init(spawn_adapter, argc, argv);

  spawn_interface->configuration_init(spawn_adapter, configuration);
  
  if (spawn_interface->adapter_configuration_init)
    spawn_interface->adapter_configuration_init(spawn_adapter);
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

  /*
   * Toolbox services
   * ----------------
   *
   * Provides:
   * - fast malloc
   * - safe malloc
   * - timing
   * - list objects
   * - slist objects
   * - htable objects
   *
   * Requires:
   * - marcel mutexes
   */
  tbx_init(*argc, argv);

  /*
   * Net-Toolbox services
   * --------------------
   *
   * Provides:
   * - pack/unpack routines for hetereogenous architecture communication
   * - safe non-blocking client-server framework over TCP
   *
   * Requires:
   * - TBX services
   */
  ntbx_init(*argc, argv);
  
  /*
   * Mad2 memory manager 
   * -------------------
   *
   * Provides:
   * - memory management for MadII internal buffer structure
   *
   * Requires:
   * - TBX services
   */
  mad_memory_manager_init(*argc, argv);

  /*
   * Mad2 core initialization 
   * -------------------
   *
   * Provides:
   * - Mad2 core objects initialization
   * - the `madeleine' object
   *
   * Requires:
   * - NTBX services
   * - Mad2 memory manager services
   *
   * Problemes:
   * - argument "configuration file"
   * - argument "adapter_set"
   */
  madeleine = mad_object_init(*argc, argv, configuration_file, adapter_set);

  /*
   * Mad2 spawn driver initialization
   * --------------------------------
   *
   * Provides:
   * - Mad2 initialization from driver info
   *
   * Should provide:
   * - node rank
   * - pm2_self
   * - pm2_conf_size
   *
   * Requires:
   * - the `madeleine' object
   */
  mad_spawn_driver_init(madeleine, argc, argv);

  /*
   * Mad2 command line parsing
   * -------------------------
   *
   * Provides:
   * - Mad2 initialization from command line arguments
   *
   * May provide:
   * - node rank
   * - pm2_self
   *
   * Requires:
   * - the `madeleine' object
   */
  mad_cmd_line_init(madeleine, *argc, argv);

  /*
   * Mad2 output redirection
   * -------------------------
   *
   * Provides:
   * - Output redirection to log files 
   *
   * Requires:
   * - the `madeleine' object
   * - the node rank
   * - high priority
   */
  mad_output_redirection_init(madeleine, *argc, argv);

  /*
   * Mad2 session configuration initialization
   * -----------------------------------------
   *
   * Provides:
   * - session configuration information
   *
   * May provide:
   * - pm2_conf_size
   *
   * Requires:
   * - the `madeleine' object
   */
  mad_configuration_init(madeleine, *argc, argv);

  /*
   * Mad2 network components initialization
   * --------------------------------------
   *
   * Provides:
   * - MadII network components ready to be connected
   * - connection data
   *
   * Requires:
   * - the `madeleine' object
   * - session configuration information
   */
  mad_network_components_init(madeleine, *argc, argv);

  /*
   * Mad2 command line clean-up
   * --------------------------
   *
   * Provides:
   * - command line clean-up
   *
   * Requires:
   * - Mad2 initialization from command line arguments
   */
  mad_purge_command_line(madeleine, argc, argv);

  /*
   * inutile
   * mad_slave_spawn(madeleine, *argc, argv);
   */

  mad_connect(madeleine, *argc, argv);

  LOG_OUT();

  return madeleine;
}

#endif /* EXTERNAL_SPAWN */
