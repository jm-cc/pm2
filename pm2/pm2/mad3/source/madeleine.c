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
#ifdef DRV_UDP
  mad_udp_register,
#endif /* DRV_UDP */
#ifdef DRV_VIA
  mad_via_register,
#endif /* DRV_VIA */
#ifdef DRV_SISCI
  mad_sisci_register,
#endif /* DRV_SISCI */
#ifdef DRV_SBP
  mad_sbp_register,
#endif /* DRV_SBP */
#ifdef DRV_MPI
  mad_mpi_register,
#endif /* DRV_MPI */
#ifdef DRV_BIP
  mad_bip_register,
#endif /* DRV_BIP */
#ifdef MARCEL /* Forwarding Transmission Module */
  mad_forward_register,
#endif /* MARCEL */

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
  madeleine = mad_madeleine_cons();
 
  madeleine->settings = mad_settings_cons();
  madeleine->session  = mad_session_cons();
  madeleine->dir      = mad_directory_cons();

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

  LOG_IN();

  argc--; argv++;

  while (argc)
    {
      if (!strcmp(*argv, "--mad_leonie"))
	{
	  argc--; argv++;

	  if (!argc)
	    FAILURE("mad_leonie argument not found");

	  settings->leonie_server_host_name = tbx_strdup(*argv);
	} 
      else if (!strcmp(*argv, "--mad_link"))
	{
	  argc--; argv++;

	  if (!argc)
	    FAILURE("mad_link argument not found");

	  settings->leonie_server_port = tbx_strdup(*argv);
	}

      argc--; argv++;
    }
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
  status = ntbx_tcp_client_connect(client,
				   settings->leonie_server_host_name,
				   &data);
  if (status == ntbx_failure)
    FAILURE("could not setup the Leonie link");

  TRACE("Leonie link is up");  
  mad_ntbx_send_string(client, client->local_host);
  session->process_rank = mad_ntbx_receive_int(client);
  TRACE_VAL("process rank", session->process_rank);
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

#if 0
static
void
mad_output_redirection_init(p_mad_madeleine_t   madeleine,
			    int                 argc TBX_UNUSED,
			    char              **argv TBX_UNUSED)
{
  p_mad_session_t  session  = madeleine->session;
  p_mad_settings_t settings = madeleine->settings;

  LOG_IN();
  if (!settings->debug_mode)
    {
      char *fmt  = "/tmp/%s-%s-%d";
      char *user = NULL;
      int   len  = 0;

      user = getenv("USER");
      if (!user)
	FAILURE("USER environment variable not defined");

      len = strlen(fmt) + strlen(user) + strlen(MAD3_LOGNAME);
      
      {
	char output[len];
	int  f;

	sprintf(output, "/tmp/%s-%s-%d",
		user, MAD3_LOGNAME, (int)session->process_rank);

	f = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0600);
	if (f < 0)
	  ERROR("open");
      
	if (dup2(f, STDOUT_FILENO) < 0)
	  ERROR("dup2");

	if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0)
	  ERROR("dup2");
      }
    }
  LOG_OUT();
}
#endif // 0

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

#if 0
  pm2debug_init_ext(argc, argv, PM2DEBUG_DO_OPT);  
  tbx_init(*argc, argv);
  ntbx_init(*argc, argv);
  mad_memory_manager_init(*argc, argv);
  madeleine = mad_object_init(*argc, argv);

  mad_cmd_line_init(madeleine, *argc, argv);
  mad_leonie_link_init(madeleine, *argc, argv);
  mad_directory_init(madeleine, *argc, argv);
  
  mad_purge_command_line(madeleine, argc, argv);
  ntbx_purge_cmd_line(argc, argv);
  tbx_purge_cmd_line(argc, argv);
  pm2debug_init_ext(argc, argv, PM2DEBUG_CLEAROPT);  
#endif // 0
  LOG_OUT();

  return madeleine;
}

