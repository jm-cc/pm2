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

struct nm_cnx_rq;
struct nm_pkt_wrap;

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
  uint8_t nb_tracks;
  
#ifdef PIOMAN_POLL
  struct piom_server server;
  struct nm_pkt_wrap post_rq;
#endif

#if(!defined(PIOM_POLLING_DISABLED) && defined(MA__LWPS))
  marcel_vpset_t vpset;
#endif

#ifdef PIOM_ENABLE_LTASKS
  struct piom_ltask task;
#endif

  /* NM core object. */
  struct nm_core *p_core;
  
};

#if(!defined(PIOM_POLLING_DISABLED) && defined(MA__LWPS) && !defined(PIOM_ENABLE_LTASKS))
#define NM_FOR_EACH_LOCAL_DRIVER(p_drv, p_core) \
  tbx_fast_list_for_each_entry(p_drv, &(p_core)->driver_list, _link) \
    if(marcel_vpset_isset(&p_drv->vpset, marcel_current_vp()))
#else
#define NM_FOR_EACH_LOCAL_DRIVER(p_drv, p_core) \
  tbx_fast_list_for_each_entry(p_drv, &(p_core)->driver_list, _link)
#endif
#define NM_FOR_EACH_DRIVER(p_drv, p_core) \
  tbx_fast_list_for_each_entry(p_drv, &(p_core)->driver_list, _link)

/** Request for connecting/disconnecting a gate with a driver. */
struct nm_cnx_rq
{
  
  struct nm_gate *p_gate;
  struct nm_drv  *p_drv;
  nm_trk_id_t trk_id;
  
  /** Remote driver url.  */
  const char *remote_drv_url;
};

/** Driver for 'NewMad_Driver' component interface.
 */
struct nm_drv_iface_s
{
  const char*name;

  int (*query)     (struct nm_drv *p_drv, struct nm_driver_query_param *params, int nparam);
  int (*init)      (struct nm_drv *p_drv, struct nm_trk_cap*trk_caps, int nb_trks);
  int (*close)     (struct nm_drv *p_drv);

  int (*connect)   (void*_status, struct nm_cnx_rq *p_crq);
  int (*accept)    (void*_status, struct nm_cnx_rq *p_crq);
  int (*disconnect)(void*_status, struct nm_cnx_rq *p_crq);

  int (*post_send_iov)(void*_status, struct nm_pkt_wrap *p_pw);
  int (*post_recv_iov)(void*_status, struct nm_pkt_wrap *p_pw);

  int (*poll_send_iov)(void*_status, struct nm_pkt_wrap *p_pw);
  int (*poll_recv_iov)(void*_status, struct nm_pkt_wrap *p_pw);

  int (*cancel_recv_iov)(void*_status, struct nm_pkt_wrap *p_pw);

  int (*poll_send_any_iov)(void*_status, struct nm_pkt_wrap **p_pw);
  int (*poll_recv_any_iov)(void*_status, struct nm_pkt_wrap **p_pw);

  const char* (*get_driver_url)(struct nm_drv *p_drv);

  struct nm_drv_cap*(*get_capabilities)(struct nm_drv *p_drv);

  int (*post_recv_iov_all)(struct nm_pkt_wrap *p_pw);
  int (*poll_recv_iov_all)(struct nm_pkt_wrap *p_pw);

#if( defined(PIOMAN) && defined(MA__LWPS))
  int (*wait_recv_iov)(void*_status, struct nm_pkt_wrap*p_pw);
  int (*wait_send_iov)(void*_status, struct nm_pkt_wrap*p_pw);

  int (*wait_send_any_iov)(void*_status, struct nm_pkt_wrap **p_pw);
  int (*wait_recv_any_iov)(void*_status, struct nm_pkt_wrap **p_pw);
#endif
};

PUK_IFACE_TYPE(NewMad_Driver, struct nm_drv_iface_s);

#endif /* NM_DRV_H */
