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
  /* Assembly associated to the driver */
  puk_adapter_t assembly;
  
  const struct nm_drv_iface_s*driver;
  
  /* NM core object. */
  struct nm_core *p_core;
  
  /* Driver id. */
  nm_drv_id_t id;

  /** Number of tracks opened on this driver. */
  uint8_t nb_tracks;
 
  /** Cumulated number of pending out requests on this driver. */
  uint8_t out_req_nb;
  
  /** Private structure of the driver. */
  void *priv;
  
#ifdef PIOMAN
  struct piom_server server;
#endif
};

/* Driver for 'NewMad_Driver' component interface.
 */
struct nm_drv_iface_s
{
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
  const char* (*get_track_url)(struct nm_drv *p_drv, nm_trk_id_t trk_id);

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
