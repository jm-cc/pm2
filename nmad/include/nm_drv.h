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

/** LF queue type for post send/recv lists */
PUK_LFQUEUE_TYPE(nm_pw_post, struct nm_pkt_wrap*, NULL, 1024);

/** Driver.
 */
struct nm_drv
{
  /** link to insert this driver into the driver_list in core */
  struct tbx_fast_list_head _link;

  /** Assembly associated to the driver */
  puk_component_t assembly;

  /** track capabilities */
  struct nm_trk_cap trk_caps[NM_SO_MAX_TRACKS];
  
  /** Driver interface, for use when no instance is needed */
  const struct nm_drv_iface_s*driver;
    
  /** recv request for trk#0 if driver supports recv_any */
  struct nm_pkt_wrap*p_in_rq;

  /** Post-scheduler outgoing lists, to be posted to the driver. */
  struct nm_pw_post_lfqueue_s post_send;

  /** recv requests submited by core, to be posted to the driver. */
  struct nm_pw_post_lfqueue_s post_recv;

  /** Private structure of the driver. */
  void *priv;

  /** Number of tracks opened on this driver. */
  int nb_tracks;
  
  /** Index of the board managed by this instance of driver */
  int index;

  /** Performance information */
  struct nm_drv_profile_s profile;

#ifdef PIOMAN
  /** task used to poll on the driver */
  struct piom_ltask p_ltask;
  /** binding for the pioman ltask */
  piom_topo_obj_t ltask_binding;
#endif	/* PIOMAN */
  /* NM core object. */
  struct nm_core *p_core;
};

#define NM_FOR_EACH_LOCAL_DRIVER(p_drv, p_core) \
  tbx_fast_list_for_each_entry(p_drv, &(p_core)->driver_list, _link)
#define NM_FOR_EACH_DRIVER(p_drv, p_core) \
  tbx_fast_list_for_each_entry(p_drv, &(p_core)->driver_list, _link)



/** Driver for 'NewMad_Driver' component interface.
 */
struct nm_drv_iface_s
{
  const char*name;

  int (*query)     (nm_drv_t p_drv, struct nm_driver_query_param *params, int nparam);
  int (*init)      (nm_drv_t p_drv, struct nm_trk_cap*trk_caps, int nb_trks);
  int (*close)     (nm_drv_t p_drv);

  int (*connect)   (void*_status, struct nm_gate*p_gate, nm_drv_t p_drv, nm_trk_id_t trk_id, const char*remote_url);
  int (*disconnect)(void*_status, struct nm_gate*p_gate, nm_drv_t p_drv, nm_trk_id_t trk_id);

  int (*post_send_iov)(void*_status, struct nm_pkt_wrap *p_pw);
  int (*post_recv_iov)(void*_status, struct nm_pkt_wrap *p_pw);

  int (*poll_send_iov)(void*_status, struct nm_pkt_wrap *p_pw);
  int (*poll_recv_iov)(void*_status, struct nm_pkt_wrap *p_pw);

  int (*wait_recv_iov)(void*_status, struct nm_pkt_wrap*p_pw);
  int (*wait_send_iov)(void*_status, struct nm_pkt_wrap*p_pw);

  int (*prefetch_send)(void*_status, struct nm_pkt_wrap *p_pw);

  int (*cancel_recv_iov)(void*_status, struct nm_pkt_wrap *p_pw);

  int (*poll_send_any_iov)(void*_status, struct nm_pkt_wrap **p_pw);
  int (*poll_recv_any_iov)(void*_status, struct nm_pkt_wrap **p_pw);

  const char* (*get_driver_url)(nm_drv_t p_drv);

  const struct nm_drv_cap_s capabilities; /**< static capabilities */

};

PUK_IFACE_TYPE(NewMad_Driver, struct nm_drv_iface_s);

#endif /* NM_DRV_H */
