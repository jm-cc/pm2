/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <stdint.h>
#include <ctype.h>

#include <nm_private.h>

#define LINE_SIZE     100

#ifndef MAXPATHLEN
#  define MAXPATHLEN 1024
#endif

int nm_parse_sampling(struct nm_drv *drv, char *drv_name)
{
  FILE *sampling_file;
  char sampling_file_path[MAXPATHLEN];
  struct utsname machine_info;
  uname(&machine_info);
  const char *archi = machine_info.machine;

  snprintf(sampling_file_path, sizeof(sampling_file_path),
	   "%s/nmad/sampling/%s_%s.nm_ns",
	   getenv("PM2_ROOT"), drv_name, archi);

  sampling_file = fopen(sampling_file_path, "r");
  if(!sampling_file)
    {
      TBX_FAILUREF("Sampling file <%s> does not exist. Check if the sampling has been done.", sampling_file_path);
    }

  fprintf(stderr, "# sampling: reading sampling file %s...\n", sampling_file_path);
  /* count sampling entries */
  int nb_entries = 0;
  char *s = NULL;
  do
    {
      char str[LINE_SIZE];
      s = fgets(str, LINE_SIZE, sampling_file);
      if(s && isdigit(str[0]))
	nb_entries++;
    }
  while(s);

  struct nm_drv_cap*caps = drv->driver->get_capabilities(drv);

  caps->nb_samplings = nb_entries;

  caps->network_sampling_bandwidth = TBX_MALLOC(nb_entries * sizeof(double));

  /* process the sampling file */
  fseek(sampling_file, 0L, SEEK_SET);
  int cur_entry = 0;
  while(1)
    {
      char str[LINE_SIZE];
      s = fgets(str, LINE_SIZE, sampling_file);
      if(!s)
	break;
      
      if(!isdigit(str[0]))
	continue;
      
      s = strchr(str, '\t') + 1;
      
      caps->network_sampling_bandwidth[cur_entry++] = atof(s);
    }

  fclose(sampling_file);

  return NM_ESUCCESS;
}

