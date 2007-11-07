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
#include <sys/types.h>
#include <pwd.h>
#ifdef PM2
#include <sys/wait.h>
#endif /* PM2 */

/* #define DEBUG */
/* #define TIMING */
#include "madeleine.h"
#include "pm2_common.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif /* MAXHOSTNAMELEN */

/*
 * Driver registration
 * -------------------
 */
static char * (*mad_driver_registration[])(p_mad_driver_interface_t interface) =
{
  mad_nmad_register,
#ifdef CONFIG_SCTP
  mad_nmad_register,
#endif
#ifdef CONFIG_GM
  mad_nmad_register,
#endif
#ifdef CONFIG_MX
  mad_nmad_register,
#endif
#ifdef CONFIG_QSNET
  mad_nmad_register,
#endif
#ifdef CONFIG_SISCI
  mad_nmad_register,
#endif
#ifdef CONFIG_IBVERBS
  mad_nmad_register,
#endif

  NULL
};

static char *mad_driver_name[] =
{
  "tcp",
#ifdef CONFIG_SCTP
  "sctp",
#endif
#ifdef CONFIG_GM
  "gm",
#endif
#ifdef CONFIG_MX
  "mx",
#endif
#ifdef CONFIG_QSNET
  "quadrics",
#endif
#ifdef CONFIG_SISCI
  "sisci",
#endif
#ifdef CONFIG_IBVERBS
  "ibverbs",
#endif

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
  p_tbx_htable_t  device_htable = NULL;
  mad_driver_id_t drv           =   -1;

  LOG_IN();
  device_htable = madeleine->device_htable;

  for (drv = 0; drv < mad_driver_number; drv++)
    {
      p_mad_driver_interface_t  interface	= NULL;
      char                     *device_name	= NULL;

      interface		= mad_driver_interface_cons();
      mad_driver_registration[drv](interface);
      device_name	= mad_driver_name[drv];
      tbx_htable_add(device_htable, device_name, interface);
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
#ifdef CONFIG_MULTI_RAIL
  madeleine->cnx_darray		 = tbx_darray_init();
  madeleine->master_channel_id   = -1;
#endif


  mad_driver_register(madeleine);

  client = ntbx_client_cons();
  ntbx_tcp_client_init(client);
  madeleine->session->leonie_link = client;
  main_madeleine = madeleine;
  LOG_OUT();

  return madeleine;
}

#ifdef READ_CONFIG_FILES
static void 
mad_cmd_line_init_from_file(p_mad_settings_t settings,
			    tbx_flag_t       *host_ok,
			    tbx_flag_t       *port_ok,
			    tbx_flag_t       *dyn_set) {
  char *host_name = NULL;
  FILE *f = NULL;
  char *filename = NULL;
  struct passwd *s_passwd;
  uid_t uid;

  host_name = TBX_MALLOC(MAXHOSTNAMELEN + 1);
  gethostname(host_name, MAXHOSTNAMELEN);
  uid = geteuid();
  s_passwd = getpwuid(uid);

  filename = TBX_MALLOC(MAXHOSTNAMELEN+100);
  filename = strcpy(filename, "/tmp/");
  filename = strcat(filename, host_name);
  filename = strcat(filename, "_");
  filename = strcat(filename, s_passwd->pw_name);

  f = fopen(filename, "r");
  TBX_FREE(host_name);
  if (f == NULL) {
    TBX_FAILUREF("Cannot read file <%s> with configuration parameters", filename);
  }
  else {
    char *line = NULL;
    int reading=0;

    line = TBX_MALLOC(MAXHOSTNAMELEN + 1);
    while (fgets(line, MAXHOSTNAMELEN, f) != NULL) {
      line[strlen(line)-1] = '\0';
      if (strncmp(line, "--mad_leonie", 12) == 0) {
	reading=1;
      }
      else if (strncmp(line, "--mad_link", 10) == 0) {
	reading=2;
      }
      else {
	if (reading == 1) {
	  if (!tbx_flag_toggle(host_ok))
	    TBX_FAILURE("Leonie server host name specified twice");
	  settings->leonie_server_host_name = tbx_strdup(line);
	  reading=0;
	}
	else if (reading == 2) {
	  if (!tbx_flag_toggle(port_ok))
	    TBX_FAILURE("Leonie server port specified twice");
	  settings->leonie_server_port = tbx_strdup(line);
	  reading=0;
	}
	else {
	  TBX_FAILUREF("Unexpected line <%s> in configuration parameters file <%s>", line, filename);
	}
      }
    }
  }
}
#endif /* READ_CONFIG_FILES */

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
	    TBX_FAILURE("Leonie server host name specified twice");

	  argc--; argv++;

	  if (!argc)
	    TBX_FAILURE("mad_leonie argument not found");

	  settings->leonie_server_host_name = tbx_strdup(*argv);
	}
      else if (!strcmp(*argv, "--mad_link"))
	{
	  if (!tbx_flag_toggle(&port_ok))
	    TBX_FAILURE("Leonie server port specified twice");
	  argc--; argv++;

	  if (!argc)
	    TBX_FAILURE("mad_link argument not found");

	  settings->leonie_server_port = tbx_strdup(*argv);
	}
      else if (!strcmp(*argv, "--mad_dyn_mode"))
	{
	  if (!tbx_flag_toggle(&dyn_set))
	    TBX_FAILURE("Madeleine's dynamic mode specified twice");

          settings->leonie_dynamic_mode	= tbx_true;
#ifndef MARCEL
          TBX_FAILURE("Madeleine's dynamic mode requires Marcel");
#endif /* MARCEL */
	}

      argc--; argv++;
    }

#ifdef READ_CONFIG_FILES
  if (tbx_flag_is_clear(&host_ok) && tbx_flag_is_clear(&port_ok))
    mad_cmd_line_init_from_file(settings, &host_ok, &port_ok, &dyn_set);
#endif // READ_CONFIG_FILES

  if (tbx_flag_is_clear(&host_ok))
    TBX_FAILURE("Leonie server host name unspecified");

  if (tbx_flag_is_clear(&port_ok))
    TBX_FAILURE("Leonie server port unspecified");

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
    TBX_FAILURE("could not setup the Leonie link");

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
mad_drivers_init(p_mad_madeleine_t   madeleine,
                 int                 *p_argc TBX_UNUSED,
                 char              ***p_argv TBX_UNUSED)
{
  mad_dir_driver_init(madeleine, p_argc, p_argv);
}

void
mad_channels_init(p_mad_madeleine_t   madeleine)
{
  mad_dir_channel_init(madeleine);
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
	    TBX_FAILURE("leonie argument disappeared");

	  _argv++; (*_argc)--;
	}
      else if (!strcmp(*_argv, "--mad_link"))
	{
	  _argv++; (*_argc)--; argc--;

	  if (!argc)
	    TBX_FAILURE("link argument disappeared");

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
  mad_memory_manager_init(*argc, argv);
  madeleine = mad_object_init(*argc, argv);
  mad_cmd_line_init(madeleine, *argc, argv);
  mad_leonie_link_init(madeleine, *argc, argv);
  mad_leonie_command_init(madeleine, *argc, argv);
  mad_directory_init(madeleine, *argc, argv);
  mad_drivers_init(madeleine, argc, &argv);
  mad_purge_command_line(madeleine, argc, argv);
  mad_channels_init(madeleine);
  common_post_init(argc, argv, NULL);
  LOG_OUT();

  return madeleine;
}

