
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
#define MAX_ARG_STR_LEN  2048
#define MAX_ARG_LEN       512

/*
 * Generation de l'URL de connexion
 * --------------------------------
 */
char *
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
void
mad_parse_url(p_mad_madeleine_t  madeleine)
{
  mad_adapter_id_t  ad = 0;
  char             *tag;
  char             *param;
  char             *c;
  char             *url;
  
  LOG_IN();

  url = madeleine->settings->url;

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
	TBX_FAILURE("too much parameters");

      current_adapter = &madeleine->adapter[ad];

      tag = c;
      c = strchr(tag, '=');
      
      if (!c)
	TBX_FAILURE("invalid cgi string");

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
	TBX_FAILURE("invalid parameter tag");

      current_adapter->master_parameter = TBX_MALLOC(strlen(param) + 1);
      CTRL_ALLOC(current_adapter->master_parameter);
      strcpy(current_adapter->master_parameter, param);
      ad++;      
    }
  while(c);
  
  if (ad != madeleine->nb_adapter)
    TBX_FAILURE("not enough parameters");
  LOG_OUT();
}

#endif /* APPLICATION_SPAWN */

