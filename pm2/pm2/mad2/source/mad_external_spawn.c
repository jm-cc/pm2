
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
void
mad_spawn_driver_init(p_mad_madeleine_t   madeleine,
		      int                *argc,
		      char              **argv)
{
  p_mad_configuration_t    configuration   = madeleine->configuration;
  p_mad_settings_t         settings        = madeleine->settings;
  p_mad_driver_t           spawn_driver    = NULL;
  p_mad_driver_interface_t spawn_interface = NULL;
  p_mad_adapter_t          spawn_adapter   = NULL;

  LOG_IN();
  spawn_driver    = &(madeleine->driver[settings->external_spawn_driver]);
  spawn_interface =
    &(madeleine->driver[settings->external_spawn_driver].interface);
  spawn_interface->driver_init(spawn_driver);
  
  spawn_adapter = &(madeleine->adapter[settings->external_spawn_driver]);
  
  if (spawn_interface->adapter_init)
    spawn_interface->adapter_init(spawn_adapter);

  if (spawn_interface->external_spawn_init)
    spawn_interface->external_spawn_init(spawn_adapter, argc, argv);

  spawn_interface->configuration_init(spawn_adapter, configuration);
  
  if (spawn_interface->adapter_configuration_init)
    spawn_interface->adapter_configuration_init(spawn_adapter);
  LOG_OUT();
}

void
mad_exchange_configuration_info(p_mad_madeleine_t madeleine)
{
  p_mad_configuration_t    configuration   = madeleine->configuration;
  p_mad_settings_t         settings        = madeleine->settings;
  ntbx_host_id_t           size            = configuration->size;
  ntbx_host_id_t           rank            = configuration->local_host_id;
  p_mad_driver_interface_t spawn_interface = NULL;
  p_mad_adapter_t          spawn_adapter   = NULL;
	      
  spawn_adapter   = &(madeleine->adapter[0]);
  spawn_interface = &(madeleine->driver[EXTERNAL_SPAWN].interface);

  if (size > 0)
    {
      if (rank)
	{ // Slaves
	  tbx_bool_t     need_conf_info = tbx_false;
	  tbx_bool_t     send_conf_info = tbx_false;
	  char          *msg            = NULL;
	  char           answer[16];
      
	  spawn_interface->receive_adapter_parameter(spawn_adapter, &msg);
	  need_conf_info = atoi(msg);
      
	  TBX_FREE(msg);
	  msg = NULL;

	  if (need_conf_info)
	    {
	      spawn_interface->
		send_adapter_parameter(spawn_adapter, 0, 
				       configuration->host_name[rank]);
	    }

	  sprintf(answer, "%d",
		  (int)(settings->configuration_file?tbx_false:tbx_true));
	  
	  spawn_interface->send_adapter_parameter(spawn_adapter, 0, answer);
	  spawn_interface->receive_adapter_parameter(spawn_adapter, &msg);

	  send_conf_info = atoi(msg);
      
	  TBX_FREE(msg);
	  msg = NULL;

	  if (send_conf_info)
	    {
	      ntbx_host_id_t i;

	      for (i = 0; i < size; i++)
		{
		  if (i != rank)
		    {
		      char *host_name = NULL;
	      
		      spawn_interface->
			receive_adapter_parameter(spawn_adapter, &host_name);

		      if (configuration->host_name[i])
			{
			  TBX_FREE(host_name);
			}
		      else
			{
			  configuration->host_name[i] = host_name;
			}

		      host_name = NULL;
		    }
		}
	    }

	  strcpy(answer, "ok");
	  spawn_interface->send_adapter_parameter(spawn_adapter, 0, answer);
	}
      else
	{ // Master
	  tbx_bool_t     need_conf_info =
	    settings->configuration_file?tbx_false:tbx_true;
	  tbx_bool_t     send_conf_info = tbx_false;
	  char           msg[16];
	  ntbx_host_id_t i;
	  
	  sprintf(msg, "%d", (int)need_conf_info);
	  
	  for (i = 1; i < size; i++)
	    {
	      char *answer = NULL;
	      
	      spawn_interface->send_adapter_parameter(spawn_adapter, i, msg);
	      
	      if (need_conf_info)
		{
		  spawn_interface->
		    receive_adapter_parameter(spawn_adapter,
					      &(configuration->
						host_name[i]));
		}
	      
	      spawn_interface->
		receive_adapter_parameter(spawn_adapter, &answer);

	      send_conf_info |= atoi(answer);
	      TBX_FREE(answer);
	    }

	  sprintf(msg, "%d", (int)send_conf_info);
	  
	  for (i = 1; i < size; i++)
	    {
	      char *answer = NULL;
	      
	      spawn_interface->send_adapter_parameter(spawn_adapter, i, msg);
	      
	      if (send_conf_info)
		{
		  ntbx_host_id_t j;

		  for (j = 0; j < size; j++)
		    {
		      if (j != i)
			{
			  spawn_interface->
			    send_adapter_parameter(spawn_adapter, i,
						   configuration->
						   host_name[j]);
			}
		    }
		}

	      spawn_interface->
		receive_adapter_parameter(spawn_adapter, &answer);

	      TBX_FREE(answer);
	    }
	}
    }
  else
    FAILURE("undefined configuration size");
}

void
mad_exchange_connection_info(p_mad_madeleine_t madeleine)
{
  p_mad_configuration_t    configuration   = madeleine->configuration;
  p_mad_driver_interface_t spawn_interface;
  p_mad_adapter_t          spawn_adapter;
  mad_adapter_id_t         ad;
  ntbx_host_id_t           rank;

  LOG_IN();
  spawn_adapter   = &(madeleine->adapter[0]);
  spawn_interface =
    &(madeleine->driver[madeleine->settings->external_spawn_driver].interface);
  rank            = configuration->local_host_id;

  for (ad = 1;
       ad < madeleine->nb_adapter;
       ad++)
    {
      p_mad_adapter_t adapter;
      
      adapter = &(madeleine->adapter[ad]);

      if (!rank)
	{
	  ntbx_host_id_t host_id;

	  for (host_id = 1;
	       host_id < configuration->size;
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
  
#ifdef PM2DEBUG  
/*
   * Logging services
   * ------------------
   *
   * Provides:
   * - runtime activable logs
   *
   * Requires:
   * - ??? <to be completed>
   */
  pm2debug_init_ext(argc, argv, PM2DEBUG_DO_OPT); 
#endif /* PM2DEBUG */

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

  ntbx_purge_cmd_line(argc, argv);
  tbx_purge_cmd_line(argc, argv);

#ifdef PM2DEBUG  
/*
   * Logging services - args clean-up
   * --------------------------------
   *
   * Provides:
   * - pm2debug command line arguments clean-up
   *
   * Requires:
   * - ??? <to be completed>
   */

  pm2debug_init_ext(argc, argv, PM2DEBUG_CLEAROPT); 
#endif /* PM2DEBUG */

  LOG_OUT();

  return madeleine;
}

#endif /* EXTERNAL_SPAWN */
