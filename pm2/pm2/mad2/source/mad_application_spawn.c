
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
 * Mad_application_spawn.c
 * =======================
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

#ifdef APPLICATION_SPAWN

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
static p_mad_madeleine_t madeleine;


/*
 * Generation de l'URL de connexion
 * --------------------------------
 */
static char *
mad_generate_url(p_mad_madeleine_t madeleine)
{
  char             *url          = NULL;
  char             *cgi_string   = NULL;
  char             *arg          = NULL;
  char             *host_name    = NULL;
  mad_adapter_id_t  ad;
  int               l;
  
  LOG_IN();
  host_name = TBX_MALLOC(MAX_HOSTNAME_LEN);
  CTRL_ALLOC(host_name);  
  url = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(url);
  cgi_string = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(cgi_string);
  arg = TBX_MALLOC(MAX_ARG_LEN);
  CTRL_ALLOC(arg);

  SYSCALL(gethostname(host_name, MAX_HOSTNAME_LEN));

  for (ad = 0;
       ad < madeleine->nb_adapter;
       ad++)
    {
      p_mad_adapter_t adapter = &(madeleine->adapter[ad]);

      sprintf(arg, "device=%s&", adapter->parameter);
      strcat (cgi_string, arg);
    }
  l = strlen(cgi_string);
  cgi_string[l - 1] = 0;
  sprintf(url, "%s?%s", host_name, cgi_string);

  TBX_FREE(host_name);
  TBX_FREE(cgi_string);
  TBX_FREE(arg);

  LOG_OUT();
  return url;
}

/*
 * Interpretation de l'URL de connexion
 * ------------------------------------
 */
static void
mad_parse_url(p_mad_madeleine_t  madeleine,
	      char              *url)
{
  mad_adapter_id_t  ad = 0;
  char             *tag;
  char             *param;
  char             *c;
  
  LOG_IN();
  if (!url)
    {
      LOG_OUT();
      return;
    }

  c = strchr(url, '?');
  if (!c)
    {
      LOG_OUT();
      return;
    }
  c++;

  do
    {
      p_mad_adapter_t current_adapter = NULL;
      
      if (ad >= madeleine->nb_adapter)
	FAILURE("too much parameters");

      current_adapter = &madeleine->adapter[ad];

      tag = c;
      c = strchr(tag, '=');
      
      if (!c)
	FAILURE("invalid cgi string");

      *c = '\0';
      c++;
      param = c;
      c = strchr(param, '&');
      
      if (c)
	{
	  *c='\0';
	  c++;
	}

      if (strcmp(tag, "device"))
	FAILURE("invalid parameter tag");

      current_adapter->master_parameter = TBX_MALLOC(strlen(param) + 1);
      CTRL_ALLOC(current_adapter->master_parameter);
      strcpy(current_adapter->master_parameter, param);
      ad++;      
    }
  while(c);
  
  if (ad != madeleine->nb_adapter)
    FAILURE("not enough parameters");
  LOG_OUT();
}

char *
mad_pre_init(p_mad_adapter_set_t adapter_set)
{
  int   dummy_argc    =       1;
  char *dummy_argv[] = { NULL } ;

  tbx_init(dummy_argc, dummy_argv);
  ntbx_init(dummy_argc, dummy_argv);
  mad_memory_manager_init(dummy_argc, dummy_argv);

  madeleine = mad_object_init(dummy_argc, dummy_argv, NULL, adapter_set);
  mad_network_components_init(madeleine, dummy_argc, dummy_argv);

  return mad_generate_url(madeleine);
}

p_mad_madeleine_t
mad_init(
	 ntbx_host_id_t  rank,
	 char           *configuration_file,
	 char           *url
	 )
{
  int   dummy_argc   =        1;
  char *dummy_argv[] = { NULL };

 
  if (configuration_file)
    {
      if (madeleine->settings->configuration_file)
	{
	  TBX_FREE(madeleine->settings->configuration_file);
	  madeleine->settings->configuration_file = NULL;
	}

      madeleine->settings->configuration_file =
	TBX_MALLOC(strlen(configuration_file) + 1);
      CTRL_ALLOC(madeleine->settings->configuration_file);
      strcpy(madeleine->settings->configuration_file, configuration_file);
    }

  if (url)
    {
      madeleine->settings->url = TBX_MALLOC(strlen(url) + 1);
      CTRL_ALLOC(madeleine->settings->url);
      strcpy(madeleine->settings->url, url);

      if (rank < 0)
	{
	  madeleine->configuration->local_host_id = 1;
	}
      else
	{
	  madeleine->configuration->local_host_id = rank;
	}
    }
  else
    {
      if (rank < 0)
	{
	  madeleine->configuration->local_host_id = 0;
	}
      else
	{
	  madeleine->configuration->local_host_id = rank;
	}
    }
  mad_output_redirection_init(madeleine, dummy_argc, dummy_argv);
  mad_configuration_init(madeleine, dummy_argc, dummy_argv);
  mad_parse_url(madeleine, url);
  mad_connect(madeleine, dummy_argc, dummy_argv);
  
  return madeleine;
}


#endif /* APPLICATION_SPAWN */

