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

#include <nm_private.h>
#include "nm_predictions.h"

#define LAT_IDX 2

#ifndef SAMPLING
#define STRAT_ISO_SPLIT
#endif

//#define STRAT_ISO_SPLIT
//#define STRAT_HCW_SPLIT

static struct
{
  nm_drv_id_t*drvs_by_lat;
  nm_drv_id_t*drvs_by_bw;
  double *drv_bws;
  double *drv_lats;
  int nb_drvs;
} nm_ns = {
  .drvs_by_lat = NULL,
  .drvs_by_bw  = NULL,
  .drv_bws     = NULL,
  .drv_lats    = NULL,
  .nb_drvs = 0
};

#ifdef SAMPLING
#endif

#ifdef SAMPLING
static int compare_lat(const void *drv1, const void *drv2)
{
  if(nm_ns.drv_lats[*(nm_drv_id_t *)drv2] - nm_ns.drv_lats[*(nm_drv_id_t *)drv1] > 0)
    return -1;
  if(nm_ns.drv_lats[*(nm_drv_id_t *)drv2] - nm_ns.drv_lats[*(nm_drv_id_t *)drv1] < 0)
    return 1;
  return 0;
}
#endif

#ifdef SAMPLING
static int compare_bw(const void *drv1, const void *drv2)
{
  if(nm_ns.drv_lats[*(nm_drv_id_t *)drv2] - nm_ns.drv_lats[*(nm_drv_id_t *)drv1] > 0)
    return 1;
  if(nm_ns.drv_lats[*(nm_drv_id_t *)drv2] - nm_ns.drv_lats[*(nm_drv_id_t *)drv1] < 0)
    return -1;
  return 0;
}
#endif

static void nm_ns_cleanup(void)
{
  if(nm_ns.nb_drvs > 0)
    {
      TBX_FREE(nm_ns.drvs_by_lat);
      nm_ns.drvs_by_lat = NULL;
      TBX_FREE(nm_ns.drvs_by_bw);
      nm_ns.drvs_by_bw = NULL;
#ifdef SAMPLING
      TBX_FREE(nm_ns.drv_lats);
      nm_ns.drv_lats = NULL;
      TBX_FREE(nm_ns.drv_bws);
      nm_ns.drv_bws = NULL;
#endif
      nm_ns.nb_drvs = 0;
    }
}

int nm_ns_update(struct nm_core *p_core)
{
  nm_ns_cleanup();
  nm_ns.nb_drvs = p_core->nb_drivers;

#ifdef SAMPLING
  struct nm_drv *p_drv = NULL;
  int bw_idx = 0, len;
  int i;

  /* 1 - recenser les bw et les latences */
  for(i = 0; i < nm_ns.nb_drvs; i++)
    {
      p_drv = &p_core->driver_array[i];
      bw_idx = (bw_idx > p_drv->driver->get_capabilities(p_drv)->nb_samplings ? bw_idx : p_drv->driver->get_capabilities(p_drv)->nb_samplings);
    }
  bw_idx -= 1;

  len = (1 << LAT_IDX);

  nm_ns.drv_bws = TBX_MALLOC(nm_ns.nb_drvs * sizeof(double));
  nm_ns.drv_lats = TBX_MALLOC(nm_ns.nb_drvs * sizeof(double));
  for(i = 0; i < nb_drivers; i++)
    {
      p_drv = &p_core->driver_array[i];
      nm_ns.drv_bws[i] = p_drv->driver->get_capabilities(p_drv)->network_sampling_bandwidth[bw_idx];
      nm_ns.drv_lats[i] = len / p_drv->driver->get_capabilities(p_drv)->network_sampling_bandwidth[LAT_IDX];
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
  nm_ns.drvs_by_bw  = TBX_MALLOC(nm_ns.nb_drvs * sizeof(nm_drv_id_t));
  int i;
  for(i = 0; i < nm_ns.nb_drvs; i++)
    {
      nm_ns.drvs_by_bw[i] = i;
    }

#ifdef SAMPLING
  qsort(nm_ns.drvs_by_bw, nm_ns.nb_drvs, sizeof(nm_drv_id_t), compare_bw);
#endif

  nm_ns.drvs_by_lat = TBX_MALLOC(nm_ns.nb_drvs * sizeof(nm_drv_id_t));
  for(i = 0; i < nm_ns.nb_drvs; i++)
    {
      nm_ns.drvs_by_lat[i] = i;
    }
#ifdef SAMPLING
  qsort(nm_ns.drvs_by_lat, nm_ns.nb_drvs, sizeof(nm_drv_id_t), compare_lat);
#endif

#if 0
  {
    fprintf(stderr, "# sampling: ordered by bw (%d) \n", nm_ns.nb_drvs);
    int k;
    for(k = 0; k < nm_ns.nb_drvs; k++)
      {
	fprintf(stderr, "#   [%d] -> %d\n", k, nm_ns.drvs_by_bw[k]);
      }
    fprintf(stderr, "# sampling: ordered by lat (%d) \n", nm_ns.nb_drvs);
    for(k = 0; k < nm_ns.nb_drvs; k++)
      {
	fprintf(stderr, "#   [%d] -> %d\n", k, nm_ns.drvs_by_lat[k]);
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

double nm_ns_evaluate_bw(struct nm_drv *driver, int length)
{
  struct nm_drv_cap* caps = driver->driver->get_capabilities(driver);
  double * samplings = caps->network_sampling_bandwidth;
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


int nm_ns_dec_bws(struct nm_core *p_core, const nm_drv_id_t **drv_ids, int*nb_drivers)
{
  *drv_ids = nm_ns.drvs_by_bw;
  *nb_drivers = nm_ns.nb_drvs;
  return NM_ESUCCESS;
}

int nm_ns_inc_lats(struct nm_core *p_core, const nm_drv_id_t **drv_ids, int*nb_drivers)
{
  *drv_ids = nm_ns.drvs_by_lat;
  *nb_drivers = nm_ns.nb_drvs;
  return NM_ESUCCESS;
}

int nm_ns_split_ratio(uint32_t len_to_send,
		      struct nm_core *p_core, nm_drv_id_t drv1_id, nm_drv_id_t drv2_id,
		      uint32_t *offset)
{
#ifdef STRAT_ISO_SPLIT
  *offset = tbx_aligned(len_to_send / 2, sizeof(uint32_t));
#elif defined STRAT_HCW_SPLIT
  *offset = tbx_aligned(len_to_send/2 + len_to_send/8, sizeof(uint32_t));
#else
 {
   struct nm_drv *drv1 = NULL;
   struct nm_drv *drv2 = NULL;
   double drv1_max_bw, drv2_max_bw;
   int bw_idx, sum_bw;
   uint32_t threshold;

   drv1 = &p_core->driver_array[drv1_id];
   drv2 = &p_core->driver_array[drv2_id];

   bw_idx = (drv1->driver->get_capabilities(drv1)->nb_samplings < drv2->driver->get_capabilities(drv2)->nb_samplings? drv1->driver->get_capabilities(drv1)->nb_samplings:drv2->driver->get_capabilities(drv2)->nb_samplings) - 1;

   drv1_max_bw = drv1->driver->get_capabilities(drv1)->network_sampling_bandwidth[bw_idx];
   drv2_max_bw = drv2->driver->get_capabilities(drv2)->network_sampling_bandwidth[bw_idx];

   sum_bw = drv1_max_bw + drv2_max_bw;

   threshold  = (len_to_send / sum_bw ) * drv1_max_bw;

   *offset = tbx_aligned(threshold, sizeof(uint32_t));
 }
#endif

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
	pending_len -= chunk_lens[i];
	sum_bw -= drv_bw;
      }
    chunks[i].len = tbx_aligned(pending_len, sizeof(uint32_t));
  }
#endif

  return NM_ESUCCESS;
}

