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

#ifndef NM_DRV_H
#define NM_DRV_H

struct nm_pkt_wrap;


/** Performance information for driver. This information is determined
 *  at init time and may depend on hardware and board number.
 */
struct nm_drv_profile_s
{
#ifdef PM2_NUIOA
  int numa_node;  /**< NUMA node where the card is attached */
#endif
  /** Approximative performance of the board
   */
  int latency;   /**< in nanoseconds (10^-9 s) */
  int bandwidth; /**< in MB/s */
};

/** Driver.
 */
struct nm_drv
{
  /** link to insert this driver into the driver_list in core */
  struct tbx_fast_list_head _link;

  /** Assembly associated to the driver */
  puk_adapter_t assembly;

  /** Driver interface, for use when no instance is needed */
  const struct nm_drv_iface_s*driver;
  
  /** recv request for trk#0 if driver supports recv_any */
  struct nm_pkt_wrap*p_in_rq;

#ifdef NMAD_POLL
  /* We don't use these lists since PIOMan already manage a 
     list of requests to poll */

  /** Outgoing active pw. */
  struct tbx_fast_list_head pending_send_list;
#ifdef PIOMAN
  /** Lock used to access pending_send_list */
  piom_spinlock_t pending_send_lock;
#endif

  /** recv pw posted to the driver. */
  struct tbx_fast_list_head pending_recv_list;
#ifdef PIOMAN
  /** Lock used to access pending_recv_list */
  piom_spinlock_t pending_recv_lock;
#endif /* PIOMAN */
#endif /* NMAD_POLL */

  /** Post-scheduler outgoing lists, to be posted to thre driver. */
  struct tbx_fast_list_head post_sched_out_list[NM_SO_MAX_TRACKS];

  /** recv requests submited to nmad, to be posted to the driver. */
  struct tbx_fast_list_head post_recv_list[NM_SO_MAX_TRACKS];

#ifdef PIOMAN
  /** Lock used to access post_sched_out_list */
  piom_spinlock_t post_sched_out_lock;

  /** Lock used to access post_recv_list */
  piom_spinlock_t post_recv_lock;

#endif /* PIOMAN */

  /** Private structure of the driver. */
  void *priv;

  /** Number of tracks opened on this driver. */
  int nb_tracks;
  
  /** Index of the board managed by this instance of driver */
  int index;

  /** Performance information */
  struct nm_drv_profile_s profile;

#ifdef PIOMAN
  struct piom_ltask task;
#ifndef PIOM_POLLING_DISABLED
  piom_vpset_t vpset;
#endif
#endif	/* PIOMAN */
  /* NM core object. */
  struct nm_core *p_core;
  
};

#define NM_FOR_EACH_LOCAL_DRIVER(p_drv, p_core) \
  tbx_fast_list_for_each_entry(p_drv, &(p_core)->driver_list, _link)
#define NM_FOR_EACH_DRIVER(p_drv, p_core) \
  tbx_fast_list_for_each_entry(p_drv, &(p_core)->driver_list, _link)

/** Static driver capabilities.
 */
struct nm_drv_cap_s
{
  int has_recv_any;  /**< driver accepts receive from NM_GATE_ANY */
  int rdv_threshold; /**< preferred length for switching to rendez-vous. */
  int min_period;    /**< minimum delay between poll (in microseconds) */
  int is_exportable; /**< blocking calls may be exported by PIOMan */
  int max_unexpected; /**< maximum size of unexpected messages on trk #0 */
};


/** Driver for 'NewMad_Driver' component interface.
 */
struct nm_drv_iface_s
{
  const char*name;

  int (*query)     (struct nm_drv *p_drv, struct nm_driver_query_param *params, int nparam);
  int (*init)      (struct nm_drv *p_drv, struct nm_trk_cap*trk_caps, int nb_trks);
  int (*close)     (struct nm_drv *p_drv);

  int (*connect)   (void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id, const char*remote_url);
  int (*disconnect)(void*_status, struct nm_gate*p_gate, struct nm_drv*p_drv, nm_trk_id_t trk_id);

  int (*post_send_iov)(void*_status, struct nm_pkt_wrap *p_pw);
  int (*post_recv_iov)(void*_status, struct nm_pkt_wrap *p_pw);

  int (*poll_send_iov)(void*_status, struct nm_pkt_wrap *p_pw);
  int (*poll_recv_iov)(void*_status, struct nm_pkt_wrap *p_pw);

  int (*prefetch_send)(void*_status, struct nm_pkt_wrap *p_pw);

  int (*cancel_recv_iov)(void*_status, struct nm_pkt_wrap *p_pw);

  int (*poll_send_any_iov)(void*_status, struct nm_pkt_wrap **p_pw);
  int (*poll_recv_any_iov)(void*_status, struct nm_pkt_wrap **p_pw);

  const char* (*get_driver_url)(struct nm_drv *p_drv);

  const struct nm_drv_cap_s capabilities; /**< static capabilities */

#if( defined(PIOMAN) && defined(MA__LWPS))
  int (*wait_recv_iov)(void*_status, struct nm_pkt_wrap*p_pw);
  int (*wait_send_iov)(void*_status, struct nm_pkt_wrap*p_pw);

  int (*wait_send_any_iov)(void*_status, struct nm_pkt_wrap **p_pw);
  int (*wait_recv_any_iov)(void*_status, struct nm_pkt_wrap **p_pw);
#endif
};

PUK_IFACE_TYPE(NewMad_Driver, struct nm_drv_iface_s);

#endif /* NM_DRV_H */
