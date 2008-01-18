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

#include "pm2_common.h"
#include "nm_drv.h"
#include "nm_drv_cap.h"
#include "nm_core.h"
#include "nm_pkt_wrap.h"
#include "nm_predictions.h"
#include "nm_errno.h"

#define LAT_IDX 2

#ifndef SAMPLING
#define STRAT_ISO_SPLIT
#endif

//#define STRAT_ISO_SPLIT
//#define STRAT_HCW_SPLIT

static uint8_t *ordered_drv_id_by_lat;
static uint8_t *ordered_drv_id_by_bw;
#ifdef SAMPLING
static double *drv_bws;
static double *drv_lats;
#endif

#ifdef SAMPLING
static int compare_lat(const void *drv1, const void *drv2){

  if(drv_lats[*(uint8_t *)drv2] - drv_lats[*(uint8_t *)drv1] > 0)
    return -1;
  if(drv_lats[*(uint8_t *)drv2] - drv_lats[*(uint8_t *)drv1] < 0)
    return 1;
  return 0;
}
#endif

static int order_lat(int nb_drivers){
  int i;

  for(i = 0; i < nb_drivers; i++){
    ordered_drv_id_by_lat[i] = i;
  }
#ifdef SAMPLING
  qsort(ordered_drv_id_by_lat, nb_drivers, sizeof(*ordered_drv_id_by_lat), compare_lat);
#endif

#if 0
  int k;
  printf("Latence [qsnet en 0]\n");
  for(k = 0; k < nb_drivers; k++)
    printf("(%d) driver %d\n", k, ordered_drv_id_by_lat[k]);
#endif

  return NM_ESUCCESS;
}

#ifdef SAMPLING
static int compare_bw(const void *drv1, const void *drv2){

  if(drv_lats[*(uint8_t *)drv2] - drv_lats[*(uint8_t *)drv1] > 0)
    return 1;
  if(drv_lats[*(uint8_t *)drv2] - drv_lats[*(uint8_t *)drv1] < 0)
    return -1;
  return 0;
}
#endif

static int order_bw(int nb_drivers){
  int i;

  for(i = 0; i < nb_drivers; i++){
    ordered_drv_id_by_bw[i] = i;
  }

#ifdef SAMPLING
  qsort(ordered_drv_id_by_bw, nb_drivers, sizeof(*ordered_drv_id_by_bw), compare_bw);
#endif

#if 0
  int k;
  printf("BW [mx en 0]\n");
  for(k = 0; k < nb_drivers; k++)
    printf("(%d) driver %d\n", k, ordered_drv_id_by_bw[k]);
#endif

  return NM_ESUCCESS;
}

int
nm_ns_init(struct nm_core *p_core){
  int nb_drivers = p_core->nb_drivers;

#ifdef SAMPLING
  struct nm_drv *p_drv = NULL;
  int bw_idx = 0, len;
  int i;

  /* 1 - recenser les bw et les latences */
  for(i = 0; i < nb_drivers; i++){
    p_drv = &p_core->driver_array[i];

    bw_idx = (bw_idx > p_drv->driver->get_capabilities(p_drv)->nb_samplings ? bw_idx : p_drv->driver->get_capabilities(p_drv)->nb_samplings);
  }
  bw_idx -= 1;

  len = (1 << LAT_IDX);

  drv_bws = TBX_MALLOC(nb_drivers * sizeof(double));
  drv_lats = TBX_MALLOC(nb_drivers * sizeof(double));
  for(i = 0; i < nb_drivers; i++){
    p_drv = &p_core->driver_array[i];

    drv_bws[i] = p_drv->driver->get_capabilities(p_drv)->network_sampling_bandwidth[bw_idx];
    drv_lats[i] = len / p_drv->driver->get_capabilities(p_drv)->network_sampling_bandwidth[LAT_IDX];

    //printf("p_drv->driver->get_capabilities(p_drv)->network_sampling_bandwidth[LAT_IDX] = %g\n", p_drv->driver->get_capabilities(p_drv)->network_sampling_bandwidth[LAT_IDX]);
  }

#if 0
  int k;
  for(k = 0; k < nb_drivers; k++){
    printf("drv_lats[%d] = %lf\n", k, drv_lats[k]);
  }
  for(k = 0; k < nb_drivers; k++){
    printf("drv_bws[%d] = %lf\n", k, drv_bws[k]);
  }
#endif
#endif

  /* 2 - ordonner les bw et les lats */
  ordered_drv_id_by_bw  = TBX_MALLOC(nb_drivers * sizeof(uint8_t));
  order_bw(nb_drivers);

  ordered_drv_id_by_lat = TBX_MALLOC(nb_drivers * sizeof(uint8_t));
  order_lat(nb_drivers);


  return NM_ESUCCESS;
}

int nm_ns_exit(struct nm_core *p_core) {
  TBX_FREE(ordered_drv_id_by_bw);
  TBX_FREE(ordered_drv_id_by_lat);

  return NM_ESUCCESS;
}

double
nm_ns_evaluate_bw(struct nm_drv *driver, int length){

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

double
nm_ns_evaluate_transfer_time(struct nm_drv *driver, int length){
  double bw = nm_ns_evaluate_bw(driver, length);

  return bw / length;
}

double
nm_ns_remaining_transfer_time(struct nm_pkt_wrap *p_pw){
  double estimate_transfer_time;
  tbx_tick_t start_transfer_time;
  tbx_tick_t now;

  estimate_transfer_time = nm_ns_evaluate_transfer_time(p_pw->p_drv, p_pw->length);
  start_transfer_time = p_pw->start_transfer_time;

  TBX_GET_TICK(now);
  return estimate_transfer_time - (TBX_TIMING_DELAY(now, start_transfer_time));
}


int
nm_ns_dec_bws(struct nm_core *p_core,
             uint8_t **drv_ids){
  *drv_ids = ordered_drv_id_by_bw;

  return NM_ESUCCESS;
}

int
nm_ns_inc_lats(struct nm_core *p_core,
               uint8_t **drv_ids){
  *drv_ids = ordered_drv_id_by_lat;

  return NM_ESUCCESS;
}

int
nm_ns_split_ratio(uint32_t len_to_send,
                  struct nm_core *p_core, int drv1_id, int drv2_id,
                  uint32_t *offset){

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

int
nm_ns_multiple_split_ratio(uint32_t len_to_send,
                           struct nm_core *p_core,
                           int nb_drv, uint8_t *drv_ids,
                           uint32_t *chunk_lens,
                           int *final_nb_drv){

#ifdef STRAT_ISO_SPLIT
  {
    int assigned_len = 0;
    int cur_nb_drv = 0;
    int chunk_len = tbx_aligned(len_to_send / nb_drv, sizeof(uint32_t));

    while(cur_nb_drv < nb_drv-1 && assigned_len + chunk_len < len_to_send){
      chunk_lens[cur_nb_drv] = chunk_len;
      cur_nb_drv ++;
      assigned_len+=chunk_len;
    }

    if(assigned_len < len_to_send){
      chunk_lens[cur_nb_drv] = len_to_send - assigned_len;
      cur_nb_drv++;
    }

    *final_nb_drv = cur_nb_drv;
  }
#else
  {
    // warning : on suppose qu'un fragment est obligatoirement envoyé sur chaque driver.
    // à la stratégie de déterminer s'il est judicieux
    // d'employer ou non l'ensemble des drivers disponibles
    int sum_bw = 0, drv_bw, pending_len = 0;
    int i;

    for(i = 0; i < nb_drv; i++){
      sum_bw += drv_bws[drv_ids[i]];
    }
    pending_len = len_to_send;

    for(i = 0; i < nb_drv - 1; i++){
      drv_bw = drv_bws[drv_ids[i]];

      chunk_lens[i] = tbx_aligned((pending_len / sum_bw) * drv_bw, sizeof(uint32_t));

      pending_len -= chunk_lens[i];
      sum_bw -= drv_bw;
    }
    chunk_lens[i] = tbx_aligned(pending_len, sizeof(uint32_t));

    *final_nb_drv = nb_drv;
  }
#endif

  return NM_ESUCCESS;
}

