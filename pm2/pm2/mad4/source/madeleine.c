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
 * Madeleine.c
 * ===========
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
#ifdef PM2
#include <sys/wait.h>
#endif /* PM2 */

/* #define DEBUG */
/* #define TIMING */
#include "madeleine.h"
#include "pm2_common.h"

/*
 * Driver registration
 * -------------------
 */
static void (*mad_driver_registration[])(p_mad_driver_t driver) =
{
#ifdef DRV_TCP
  mad_tcp_register,
#endif /* DRV_TCP */
#ifdef DRV_RAND
  mad_rand_register,
#endif /* DRV_RAND */
#ifdef DRV_VRP
  mad_vrp_register,
#endif /* DRV_VRP */
#ifdef DRV_SISCI
  mad_sisci_register,
#endif /* DRV_SISCI */
#ifdef DRV_BIP
  mad_bip_register,
#endif /* DRV_BIP */
#ifdef DRV_GM
  mad_gm_register,
#endif /* DRV_GM */
  mad_forward_register,
  mad_mux_register,

  NULL
};


/*
 * Constantes
 * ----------
 */
#define MAX_HOSTNAME_LEN  256

/*
 * Objet Madeleine
 * ---------------
 */
static p_mad_madeleine_t main_madeleine = NULL;

/*
 * Functions
 * ---------------
 */

static
void
mad_driver_register(p_mad_madeleine_t madeleine)
{
  p_tbx_htable_t  driver_htable = NULL;
  mad_driver_id_t drv           =   -1;

  LOG_IN();
  driver_htable = madeleine->driver_htable;

  for (drv = 0; drv < mad_driver_number; drv++)
    {
      p_mad_driver_t driver = NULL;

      driver = mad_driver_cons();

      driver->madeleine      = madeleine;
      driver->adapter_htable = tbx_htable_empty_table();
      driver->interface      = mad_driver_interface_cons();

      mad_driver_registration[drv](driver);

      tbx_htable_add(driver_htable, driver->name, driver);
    }
  LOG_OUT();
}

p_mad_madeleine_t
mad_object_init(int    argc TBX_UNUSED,
		char **argv TBX_UNUSED)
{
  p_mad_madeleine_t madeleine = NULL;
  p_ntbx_client_t   client    = NULL;

  LOG_IN();
  madeleine                      = mad_madeleine_cons();
  madeleine->dynamic             = mad_dynamic_cons(); 
  madeleine->dynamic->updated    = tbx_false;
  madeleine->dynamic->merge_done = tbx_false;
  madeleine->dynamic->mergeable  = tbx_false; 
  madeleine->settings            = mad_settings_cons();
  madeleine->session             = mad_session_cons();
  madeleine->dir                 = mad_directory_cons();
  madeleine->old_dir             = NULL;
  madeleine->new_dir             = NULL;

   
  mad_driver_register(madeleine);

  client = ntbx_client_cons();
  ntbx_tcp_client_init(client);
  madeleine->session->leonie_link = client;

  main_madeleine = madeleine;
  LOG_OUT();

  return madeleine;
}

void
mad_cmd_line_init(p_mad_madeleine_t   madeleine,
		  int                 argc,
		  char              **argv)
{
  p_mad_settings_t settings = madeleine->settings;
  tbx_flag_t       host_ok  = tbx_flag_clear;
  tbx_flag_t       port_ok  = tbx_flag_clear;
  tbx_flag_t       dyn_set  = tbx_flag_clear;

  LOG_IN();

  argc--; argv++;

  while (argc)
    {
      if (!strcmp(*argv, "--mad_leonie"))
	{
	  if (!tbx_flag_toggle(&host_ok))
	    FAILURE("Leonie server host name specified twice");

	  argc--; argv++;

	  if (!argc)
	    FAILURE("mad_leonie argument not found");

	  settings->leonie_server_host_name = tbx_strdup(*argv);
	}
      else if (!strcmp(*argv, "--mad_link"))
	{
	  if (!tbx_flag_toggle(&port_ok))
	    FAILURE("Leonie server port specified twice");
	  argc--; argv++;

	  if (!argc)
	    FAILURE("mad_link argument not found");

	  settings->leonie_server_port = tbx_strdup(*argv);
	}
      else if (!strcmp(*argv, "--mad_dyn_mode"))
	{
	  if (!tbx_flag_toggle(&dyn_set))
	    FAILURE("Madeleine's dynamic mode specified twice");

          settings->leonie_dynamic_mode	= tbx_true;
	}

      argc--; argv++;
    }

  if (tbx_flag_is_clear(&host_ok))
    FAILURE("Leonie server host name unspecified");

  if (tbx_flag_is_clear(&port_ok))
    FAILURE("Leonie server port unspecified");

  LOG_OUT();
}

void
mad_leonie_link_init(p_mad_madeleine_t   madeleine,
		     int                 argc TBX_UNUSED,
		     char              **argv TBX_UNUSED)
{
  p_mad_settings_t       settings = NULL;
  p_mad_session_t        session  = NULL;
  p_ntbx_client_t        client   = NULL;
  int                    status   = ntbx_failure;
  ntbx_connection_data_t data;

  LOG_IN();
  settings = madeleine->settings;
  session  = madeleine->session;
  client   = session->leonie_link;

  strcpy(data.data, settings->leonie_server_port);
  status =
    ntbx_tcp_client_connect(client, settings->leonie_server_host_name, &data);
  if (status == ntbx_failure)
    FAILURE("could not setup the Leonie link");

  TRACE("Leonie link is up");

  mad_ntbx_send_string(client, client->local_host);

  session->process_rank = mad_ntbx_receive_int(client);
  TRACE_VAL("process rank", session->process_rank);
   
  session->session_id = mad_ntbx_receive_int(client);
  TRACE_VAL("session is", session->session_id);

  LOG_OUT();
}

void
mad_directory_init(p_mad_madeleine_t   madeleine,
		   int                 argc TBX_UNUSED,
		   char              **argv TBX_UNUSED)
{
  LOG_IN();
  mad_dir_directory_get(madeleine);
  LOG_OUT();
}

void
mad_purge_command_line(p_mad_madeleine_t   madeleine TBX_UNUSED,
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
      else if (!strcmp(*_argv, "--mad_leonie"))
	{
	  _argv++; (*_argc)--; argc--;

	  if (!argc)
	    FAILURE("leonie argument disappeared");

	  _argv++; (*_argc)--;
	}
      else if (!strcmp(*_argv, "--mad_link"))
	{
	  _argv++; (*_argc)--; argc--;

	  if (!argc)
	    FAILURE("link argument disappeared");

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
mad_get_madeleine(void)
{
  p_mad_madeleine_t madeleine = NULL;

  LOG_IN();
  madeleine = main_madeleine;
  LOG_OUT();

  return madeleine;
}

p_mad_madeleine_t
mad_init(int   *argc,
	 char **argv)
{
  p_mad_madeleine_t madeleine = NULL;

  LOG_IN();
  common_pre_init(argc, argv, NULL);
  common_post_init(argc, argv, NULL);
  madeleine = mad_get_madeleine();
  LOG_OUT();

  return madeleine;
}

