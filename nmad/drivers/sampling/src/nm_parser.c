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

#include <tbx.h>
#include <nm_public.h>
#include "nm_drv_ops.h"
#include "nm_drv_cap.h"
#include "nm_drv.h"
#include "nm_parser.h"

#define DRV_NAME_SIZE 20
#define LINE_SIZE     100

int
nm_parse_sampling(struct nm_drv *drv, char *drv_name){

  FILE *sampling_file;
  int sampling_file_path_len = 0;
  char *str = TBX_MALLOC(LINE_SIZE *sizeof(char));
  char *s = NULL;
  int nb_entries = 0;
  int cur_entry = 0;

  char *starting_path = getenv("PM2_ROOT");
  sampling_file_path_len += strlen(starting_path);

  char *next_part = "/nmad/sampling/";
  sampling_file_path_len += strlen(next_part);

  //char *drv_name = NULL;
//#if defined CONFIG_IBVERBS
//  drv_name = "ibverbs";
//#elif defined CONFIG_MX
//  drv_name = "mx";
//#elif defined CONFIG_GM
//  drv_name = "gm";
//#elif defined CONFIG_QSNET
//  drv_name = "qsnet";
//#elif defined CONFIG_TCP
//  drv_name = "tcp";
//#else
//  drv_name = "tcpdg";
//#endif

  sampling_file_path_len += strlen(drv_name);

  char *separator = "_";
  sampling_file_path_len += strlen(separator);

  struct utsname machine_info;
  uname(&machine_info);
  char *archi = machine_info.machine;
  sampling_file_path_len += strlen(archi);

  separator = "_samplings.nm_ns";
  sampling_file_path_len += strlen(separator);

  char *sampling_file_path = TBX_MALLOC(sampling_file_path_len * sizeof(char));
  memset(sampling_file_path, 0, sampling_file_path_len);
  strcat(sampling_file_path, starting_path);
  strcat(sampling_file_path, next_part);
  strcat(sampling_file_path, drv_name);
  strcat(sampling_file_path, "_");
  strcat(sampling_file_path, archi);
  strcat(sampling_file_path, "_samplings.nm_ns");

  sampling_file = fopen(sampling_file_path, "r");
  if(!sampling_file){
    TBX_FAILUREF("Sampling file <%s> does not exist. Check if the sampling has been done.", sampling_file_path);
  }

  /* count sampling entries */
  while(1){
    s = fgets(str, LINE_SIZE, sampling_file);
    if(!s)
      break;

    if(isdigit(str[0]))
      nb_entries++;
  }
  drv->cap.nb_samplings = nb_entries;

  drv->cap.network_sampling_bandwidth = TBX_MALLOC(nb_entries * sizeof(double));

  /* process the sampling file */
  fseek(sampling_file, 0L, SEEK_SET);
  while(1){
    s = fgets(str, LINE_SIZE, sampling_file);
    if(!s)
      break;

    if(!isdigit(str[0]))
      continue;

    s = strchr(str, '\t') + 1;

    drv->cap.network_sampling_bandwidth[cur_entry++] = atof(s);
  }

  fclose(sampling_file);
  TBX_FREE(sampling_file_path);
  TBX_FREE(str);

  return NM_ESUCCESS;
}

