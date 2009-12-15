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
#include <stdint.h>
#include <sys/uio.h>
#include <math.h>
#include <sys/utsname.h>
#include <ctype.h>

#include <nm_private.h>

#define LAT_IDX 2

#ifndef SAMPLING
#define STRAT_ISO_SPLIT
#endif

//#define STRAT_ISO_SPLIT

struct nm_sampling_set_s
{
  double*bandwidth;
  int nb_samples;
};

static struct
{
  struct nm_drv**p_drvs_by_lat;
  struct nm_drv**p_drvs_by_bw;
  double *drv_bws;
  double *drv_lats;
  struct nm_sampling_set_s*sampling_sets;
  int nb_drvs;
} nm_ns = {
  .p_drvs_by_lat = NULL,
  .p_drvs_by_bw  = NULL,
  .drv_bws     = NULL,
  .drv_lats    = NULL,
  .nb_drvs = 0
};

#define LINE_SIZE     100

#ifndef MAXPATHLEN
#  define MAXPATHLEN 1024
#endif

int nm_ns_parse_sampling(struct nm_drv*p_drv)
{
  FILE *sampling_file;
  char sampling_file_path[MAXPATHLEN];
  struct utsname machine_info;
  uname(&machine_info);
  const char *archi = machine_info.machine;
  
  const char*pm2_conf_dir = getenv("PM2_CONF_DIR");
  if(pm2_conf_dir)
    {
      snprintf(sampling_file_path, sizeof(sampling_file_path),
	       "%s/nmad/sampling/%s_%s.nm_ns",
	       pm2_conf_dir, p_drv->driver->name, archi);
    }
  else
    {
      const char*home = getenv("HOME");
      snprintf(sampling_file_path, sizeof(sampling_file_path),
	       "%s/.pm2/nmad/sampling/%s_%s.nm_ns",
	       home, p_drv->driver->name, archi);
    }

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

  struct nm_sampling_set_s*set = &nm_ns.sampling_sets[p_drv->id];

  set->nb_samples = nb_entries;
  set->bandwidth = TBX_MALLOC(nb_entries * sizeof(double));

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
      
      set->bandwidth[cur_entry++] = atof(s);
    }

  fclose(sampling_file);

  return NM_ESUCCESS;
}


#ifdef SAMPLING
static int compare_lat(const void*_drv1, const void*_drv2)
{
  const struct nm_drv**pp_drv1 = (const struct nm_drv**)_drv1;
  const struct nm_drv**pp_drv2 = (const struct nm_drv**)_drv2;
  const struct nm_drv*p_drv1 = *pp_drv1;
  const struct nm_drv*p_drv2 = *pp_drv2;
  if(nm_ns.drv_lats[p_drv2->id] - nm_ns.drv_lats[p_drv1->id] > 0)
    return -1;
  if(nm_ns.drv_lats[p_drv2->id] - nm_ns.drv_lats[p_drv1->id] < 0)
    return 1;
  return 0;
}
#endif

#ifdef SAMPLING
static int compare_bw(const void*_drv1, const void*_drv2)
{
  const struct nm_drv**pp_drv1 = (const struct nm_drv**)_drv1;
  const struct nm_drv**pp_drv2 = (const struct nm_drv**)_drv2;
  const struct nm_drv*p_drv1 = *pp_drv1;
  const struct nm_drv*p_drv2 = *pp_drv2;
  if(nm_ns.drv_lats[p_drv2->id] - nm_ns.drv_lats[p_drv1->id] > 0)
    return 1;
  if(nm_ns.drv_lats[p_drv2->id] - nm_ns.drv_lats[p_drv1->id] < 0)
    return -1;
  return 0;
}
#endif

static void nm_ns_cleanup(void)
{
  if(nm_ns.nb_drvs > 0)
    {
      TBX_FREE(nm_ns.p_drvs_by_lat);
      nm_ns.p_drvs_by_lat = NULL;
      TBX_FREE(nm_ns.p_drvs_by_bw);
      nm_ns.p_drvs_by_bw = NULL;
#ifdef SAMPLING
      TBX_FREE(nm_ns.drv_lats);
      nm_ns.drv_lats = NULL;
      TBX_FREE(nm_ns.drv_bws);
      nm_ns.drv_bws = NULL;
      TBX_FREE(nm_ns.sampling_sets);
      nm_ns.sampling_sets = NULL;
#endif
      nm_ns.nb_drvs = 0;
    }
}

int nm_ns_update(struct nm_core *p_core)
{
  struct nm_drv *p_drv = NULL;
  int i;

  if(p_core->nb_drivers == nm_ns.nb_drvs)
    {
      return NM_ESUCCESS;
    }

  nm_ns_cleanup();
  nm_ns.nb_drvs = p_core->nb_drivers;

#ifdef SAMPLING
  int bw_idx = 0, len;

  nm_ns.sampling_sets = TBX_MALLOC(nm_ns.nb_drvs * sizeof(struct nm_sampling_set_s));

  /* 1 - recenser les bw et les latences */
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      nm_ns_parse_sampling(p_drv);
      bw_idx = (bw_idx > nm_ns.sampling_sets[p_drv->id].nb_samples) ? bw_idx : nm_ns.sampling_sets[p_drv->id].nb_samples;
    }
  bw_idx -= 1;

  len = (1 << LAT_IDX);

  nm_ns.drv_bws = TBX_MALLOC(nm_ns.nb_drvs * sizeof(double));
  nm_ns.drv_lats = TBX_MALLOC(nm_ns.nb_drvs * sizeof(double));
  i = 0;
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      nm_ns.drv_bws[i] = nm_ns.sampling_sets[p_drv->id].bandwidth[bw_idx];
      nm_ns.drv_lats[i] = len / nm_ns.sampling_sets[p_drv->id].bandwidth[LAT_IDX];
      i++;
    }

#if 0
  int k;
  for(k = 0; k < nm_ns.nb_drvs; k++)
    {
      printf("drv_lats[%d] = %lf\n", k, nm_ns.drv_lats[k]);
    }
  for(k = 0; k < nm_ns.nb_drvs; k++)
    {
      printf("drv_bws[%d] = %lf\n", k, nm_ns.drv_bws[k]);
    }
#endif
#endif

  /* 2 - ordonner les bw et les lats */
  nm_ns.p_drvs_by_bw = TBX_MALLOC(nm_ns.nb_drvs * sizeof(struct nm_drv*));
  i = 0;
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      nm_ns.p_drvs_by_bw[i] = p_drv;
      i++;
    }

#ifdef SAMPLING
  qsort(nm_ns.p_drvs_by_bw, nm_ns.nb_drvs, sizeof(struct nm_drv*), compare_bw);
#endif

  nm_ns.p_drvs_by_lat = TBX_MALLOC(nm_ns.nb_drvs * sizeof(struct nm_drv*));
  i = 0;
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      nm_ns.p_drvs_by_lat[i] = p_drv;
      i++;
    }
#ifdef SAMPLING
  qsort(nm_ns.p_drvs_by_lat, nm_ns.nb_drvs, sizeof(struct nm_drv*), compare_lat);
#endif

#if 0
  {
    fprintf(stderr, "# sampling: ordered by bw (%d) \n", nm_ns.nb_drvs);
    int k;
    for(k = 0; k < nm_ns.nb_drvs; k++)
      {
	fprintf(stderr, "#   [%d] -> %p\n", k, nm_ns.drvs_by_bw[k]);
      }
    fprintf(stderr, "# sampling: ordered by lat (%d) \n", nm_ns.nb_drvs);
    for(k = 0; k < nm_ns.nb_drvs; k++)
      {
	fprintf(stderr, "#   [%d] -> %p\n", k, nm_ns.drvs_by_lat[k]);
      }
  }
#endif

  return NM_ESUCCESS;
}

int nm_ns_exit(struct nm_core *p_core)
{
  nm_ns_cleanup();
  return NM_ESUCCESS;
}

double nm_ns_evaluate_bw(struct nm_drv *p_drv, int length)
{
  const double *samplings = nm_ns.sampling_sets[p_drv->id].bandwidth;
  int sampling_id = 0;
  double coef = 0;
  int sampling_start_id = 0;
  
  frexp(2, &sampling_start_id);
  coef = frexp(length, &sampling_id);
  
  sampling_id -= sampling_start_id;
  
  return samplings[sampling_id]
    + coef * (samplings[sampling_id] - samplings[sampling_id - 1]);
}

double nm_ns_evaluate_transfer_time(struct nm_drv *driver, int length)
{
  const double bw = nm_ns_evaluate_bw(driver, length);
  return bw / length;
}

double nm_ns_remaining_transfer_time(struct nm_pkt_wrap *p_pw)
{
  double estimate_transfer_time;
  tbx_tick_t start_transfer_time;
  tbx_tick_t now;

  estimate_transfer_time = nm_ns_evaluate_transfer_time(p_pw->p_drv, p_pw->length);
  start_transfer_time = p_pw->start_transfer_time;

  TBX_GET_TICK(now);
  return estimate_transfer_time - (TBX_TIMING_DELAY(now, start_transfer_time));
}


int nm_ns_dec_bws(struct nm_core *p_core, struct nm_drv*const**p_drvs, int*nb_drivers)
{
  *p_drvs = nm_ns.p_drvs_by_bw;
  *nb_drivers = nm_ns.nb_drvs;
  return NM_ESUCCESS;
}

int nm_ns_inc_lats(struct nm_core *p_core, struct nm_drv*const**p_drvs, int*nb_drivers)
{
  *p_drvs = nm_ns.p_drvs_by_lat;
  *nb_drivers = nm_ns.nb_drvs;
  return NM_ESUCCESS;
}

int nm_ns_multiple_split_ratio(uint32_t len, struct nm_core *p_core,
			       int*nb_chunks, struct nm_rdv_chunk*chunks)
{
#ifdef STRAT_ISO_SPLIT
  {
    int assigned_len = 0;
    int chunk_index = 0;
    int chunk_len = tbx_aligned(len / *nb_chunks, sizeof(uint32_t));

    while(chunk_index < *nb_chunks - 1 && assigned_len + chunk_len < len)
      {
	chunks[chunk_index].len = chunk_len;
	chunk_index++;
	assigned_len += chunk_len;
      }
    if(assigned_len < len)
      {
	chunks[chunk_index].len = len - assigned_len;
	chunk_index++;
      }
    *nb_chunks = chunk_index;
  }
#else
  {
    // warning : on suppose qu'un fragment est obligatoirement envoyé sur chaque driver.
    // à la stratégie de déterminer s'il est judicieux
    // d'employer ou non l'ensemble des drivers disponibles
    int sum_bw = 0;
    int i;
    for(i = 0; i < *nb_chunks; i++)
      {
	sum_bw += nm_ns.drv_bws[chunks[i].drv_id];
      }
    int pending_len = len;
    for(i = 0; i < *nb_chunks - 1; i++)
      {
	const int drv_bw = nm_ns.drv_bws[chunks[i].drv_id];
	chunks[i].len = tbx_aligned((pending_len / sum_bw) * drv_bw, sizeof(uint32_t));
	pending_len -= chunks[i].len;
	sum_bw -= drv_bw;
      }
    chunks[i].len = tbx_aligned(pending_len, sizeof(uint32_t));
  }
#endif

  return NM_ESUCCESS;
}

