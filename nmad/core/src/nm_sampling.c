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


struct nm_sampling_set_s
{
  double*bandwidth_samples; /**< bandiwidth samples in MB/s */
  int nb_samples;           /**< number of samples */
  double bw;                /**< asymptotic bandiwdth in MB/s */
  double lat;               /**< latency in usec. */
};

static struct
{
  struct nm_drv**p_drvs_by_lat;
  struct nm_drv**p_drvs_by_bw;
  puk_hashtable_t sampling_sets;
  int nb_drvs;
} nm_ns = {
  .p_drvs_by_lat = NULL,
  .p_drvs_by_bw  = NULL,
  .sampling_sets = NULL,
  .nb_drvs = 0
};

#define LINE_SIZE     100

#ifndef MAXPATHLEN
#  define MAXPATHLEN 1024
#endif

int nm_ns_parse_sampling(struct nm_sampling_set_s*p_set, struct nm_drv*p_drv)
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
  if(sampling_file)
    {
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
      
      p_set->nb_samples = nb_entries;
      p_set->bandwidth_samples = TBX_MALLOC(nb_entries * sizeof(double));
      
      /* load the sampling file */
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
	  
	  p_set->bandwidth_samples[cur_entry++] = atof(s);
	}
      
      fclose(sampling_file);
      
      /* compute latency and bandwidth */
      p_set->bw = p_set->bandwidth_samples[nb_entries - 1];
      p_set->lat = (1 << LAT_IDX) / p_set->bandwidth_samples[LAT_IDX];
    }
  else
    {
      fprintf(stderr, "# sampling: file <%s> does not exist. Taking default capabilities of driver.\n",
	      sampling_file_path);
      p_set->nb_samples = 0;
      p_set->bandwidth_samples = NULL;
      p_set->bw  = (p_drv->driver->get_capabilities(p_drv))->bandwidth;
      p_set->lat = (p_drv->driver->get_capabilities(p_drv))->latency / 1000.0;
      fprintf(stderr, "# sampling: capabilities for driver %s; lat = %5.2f usec.; bw = %5.2f MB/s\n",
	      p_drv->driver->name, p_set->lat, p_set->bw);
    }

  return NM_ESUCCESS;
}


#ifdef SAMPLING
static int compare_lat(const void*_drv1, const void*_drv2)
{
  const struct nm_drv**pp_drv1 = (const struct nm_drv**)_drv1;
  const struct nm_drv**pp_drv2 = (const struct nm_drv**)_drv2;
  const struct nm_drv*p_drv1 = *pp_drv1;
  const struct nm_drv*p_drv2 = *pp_drv2;
  const struct nm_sampling_set_s*p_set1  = puk_hashtable_lookup(nm_ns.sampling_sets, p_drv1);
  const struct nm_sampling_set_s*p_set2  = puk_hashtable_lookup(nm_ns.sampling_sets, p_drv2);
  if(p_set1->lat < p_set2->lat)
    return -1;
  if(p_set1->lat > p_set2->lat)
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
  const struct nm_sampling_set_s*p_set1  = puk_hashtable_lookup(nm_ns.sampling_sets, p_drv1);
  const struct nm_sampling_set_s*p_set2  = puk_hashtable_lookup(nm_ns.sampling_sets, p_drv2);
  if(p_set2->bw - p_set1->bw > 0)
    return 1;
  if(p_set2->bw - p_set1->bw < 0)
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
    }
}

int nm_ns_update(struct nm_core*p_core, struct nm_drv*p_drv)
{
  nm_ns_cleanup();
  nm_ns.nb_drvs = p_core->nb_drivers;

  /* load samples from disk */
#ifdef SAMPLING
  struct nm_sampling_set_s*p_set = TBX_MALLOC(sizeof(struct nm_sampling_set_s));
  nm_ns_parse_sampling(p_set, p_drv);
  if(nm_ns.sampling_sets == NULL)
    nm_ns.sampling_sets = puk_hashtable_new_ptr();
  puk_hashtable_insert(nm_ns.sampling_sets, p_drv, p_set);
#endif

  /* sort drivers by bandwidth and latency */
  nm_ns.p_drvs_by_bw = TBX_MALLOC(nm_ns.nb_drvs * sizeof(struct nm_drv*));
  nm_ns.p_drvs_by_lat = TBX_MALLOC(nm_ns.nb_drvs * sizeof(struct nm_drv*));
  int i = 0;
  NM_FOR_EACH_DRIVER(p_drv, p_core)
    {
      nm_ns.p_drvs_by_bw[i] = p_drv;
      nm_ns.p_drvs_by_lat[i] = p_drv;
      i++;
    }
#ifdef SAMPLING
  qsort(nm_ns.p_drvs_by_bw, nm_ns.nb_drvs, sizeof(struct nm_drv*), compare_bw);
  qsort(nm_ns.p_drvs_by_lat, nm_ns.nb_drvs, sizeof(struct nm_drv*), compare_lat);
#endif

#if 0
  {
    fprintf(stderr, "# sampling: ordered by dec bw (%d) \n", nm_ns.nb_drvs);
    int k;
    for(k = 0; k < nm_ns.nb_drvs; k++)
      {
	fprintf(stderr, "#   [%d] -> %p (%s)\n", k, nm_ns.p_drvs_by_bw[k], nm_ns.p_drvs_by_bw[k]->driver->name);
      }
    fprintf(stderr, "# sampling: ordered by inc lat (%d) \n", nm_ns.nb_drvs);
    for(k = 0; k < nm_ns.nb_drvs; k++)
      {
	fprintf(stderr, "#   [%d] -> %p (%s)\n", k, nm_ns.p_drvs_by_lat[k], nm_ns.p_drvs_by_lat[k]->driver->name);
      }
  }
#endif

  return NM_ESUCCESS;
}

int nm_ns_exit(struct nm_core *p_core)
{
  nm_ns_cleanup();
  /* TODO- free the sampling_sets hashtable */
  return NM_ESUCCESS;
}

double nm_ns_evaluate_bw(struct nm_drv *p_drv, int length)
{
  const struct nm_sampling_set_s*p_set = puk_hashtable_lookup(nm_ns.sampling_sets, p_drv);
  if(p_set->nb_samples > 0)
    {
      const double *samples = p_set->bandwidth_samples;
      int sample_id = 0;
      double coef = 0;
      int sampling_start_id = 0;
      
      frexp(2, &sampling_start_id);
      coef = frexp(length, &sample_id);
      
      sample_id -= sampling_start_id;
      
      return samples[sample_id] + coef * (samples[sample_id] - samples[sample_id - 1]);
    }
  else
    {
      return p_set->bw;
    }
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
#ifndef SAMPLING
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
#else /* SAMPLING */
  {
    /** @warning we suppose each driver will be used here.
     * It is up to the strategy to decided whether all drivers have to be used.
     */
    int sum_bw = 0;
    int i;
    for(i = 0; i < *nb_chunks; i++)
      {
	const struct nm_sampling_set_s*p_set = puk_hashtable_lookup(nm_ns.sampling_sets, chunks[i].p_drv);
	sum_bw += p_set->bw;
      }
    int pending_len = len;
    for(i = 0; i < *nb_chunks - 1; i++)
      {
	const struct nm_sampling_set_s*p_set = puk_hashtable_lookup(nm_ns.sampling_sets, chunks[i].p_drv);
	const int drv_bw = p_set->bw;
	chunks[i].len = tbx_aligned((pending_len / sum_bw) * drv_bw, sizeof(uint32_t));
	pending_len -= chunks[i].len;
	sum_bw -= drv_bw;
      }
    chunks[i].len = tbx_aligned(pending_len, sizeof(uint32_t));
  }
#endif /* SAMPLING */

  return NM_ESUCCESS;
}

