/*
 * NewMadeleine
 * Copyright (C) 2013-2017 (see AUTHORS file)
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

#ifndef NM_MINIDRIVER_H
#define NM_MINIDRIVER_H

#include <nm_public.h>
#include <Padico/Puk.h>
#ifdef PM2_TOPOLOGY
#include <hwloc.h>
#endif
#include <sys/uio.h>

/** Performance information for driver. This information is determined
 *  at init time and may depend on hardware and board number.
 */
struct nm_drv_profile_s
{
#ifdef PM2_TOPOLOGY
  hwloc_cpuset_t cpuset;  /**< cpu set close to the card (allocated by the caller, filled by driver) */
#endif /* PM2_TOPOLOGY */
  /** Approximative performance of the board
   */
  int latency;   /**< in nanoseconds (10^-9 s) */
  int bandwidth; /**< in MB/s */
};

/** Static driver capabilities.
 */
struct nm_minidriver_capabilities_s
{
  nm_len_t max_msg_size; /**< maximum message size for the track */
  int supports_data;     /**< driver can send/recv direct nm_data_s */
  int supports_buf_send; /**< driver supported buffer-based send */
  int supports_buf_recv; /**< driver supported buffer-based recv */
  int has_recv_any;      /**< driver accepts receive from NM_GATE_ANY */
  int min_period;        /**< minimum delay between poll (in microseconds) */
  int is_exportable;     /**< blocking calls may be exported by PIOMan */
  int trk_rdv;           /**< trk needs a rdv for matched send/recv size; if 0, trk preserves boundaries */
};

struct nm_minidriver_properties_s
{
  struct nm_drv_profile_s profile;
  struct nm_minidriver_capabilities_s capabilities;
};

/** Interface driver for the 'NewMad_minidriver' component interface.
 * A driver interface simpler than NewMad_Driver, that do not rely 
 * on nmad core structures, making driver fully encapsulated.
 */
struct nm_minidriver_iface_s
{
  /* ** driver init; applied to context before/after instantiation */

  /** get driver properties (profiling and capabilities) */
  void (*getprops)(puk_context_t context, struct nm_minidriver_properties_s*props);
  /** init the driver and get its url */
  void (*init)(puk_context_t context, const void**drv_url, size_t*url_size);
  /** close the driver */
  void (*close)(puk_context_t context);
  
  /* ** connection establishment */

  /** connect this instance to the remote url  */
  void (*connect)(void*_status, const void*remote_url, size_t url_size);
  /* TODO- disconnect */
  /* TODO- register error handler for fault-tolerance */
  
  /* ** sending primitives */

  /** post a send for data described by iovec */
  void (*send_post)(void*_status, const struct iovec*v, int n);
  /** post a send for data described by nm_data iterator */
  void (*send_data)(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
  /** poll a send operation already posted */
  int  (*send_poll)(void*_status);
  /** prefetch data in driver buffers before send_post 
   * @note currently broken; obsoleted by buffer-based send (see below)
   */
  void (*send_prefetch)(void*_status, const void*ptr, nm_len_t size);

  /* ** buffer-based sending */

  /** get a buffer to prepare data for send */
  void (*buf_send_get)(void*_status, void**p_buffer, nm_len_t*p_len);
  /** post a send after upper layers have filled-in the buffer */
  void (*buf_send_post)(void*_status, nm_len_t len);
  /* note: polling for buff_send is done through send_poll() */

  /* ** receiving primitives */

  /** post a recv for data described by iovec */
  void (*recv_init)(void*_status, struct iovec*v, int n);
  /** post a recv for data described by nm_data iterator */
  void (*recv_data)(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
  /** poll a recv operation already posted */
  int  (*poll_one)(void*_status);
  /** cancel a posted recv */
  int  (*cancel_recv)(void*_status);
  /** poll the driver for a pending recv from any source; returns status of instance */
  int  (*recv_poll_any)(puk_context_t p_context, void**_status);
  /** passively wait for a pending recv from any source; returns status of instance */
  int  (*recv_wait_any)(puk_context_t p_context, void**_status);

  /* ** buffer-based recv */

  /** poll the driver for a pending recv; returns buffer */
  int  (*buf_recv_poll)(void*_status, void**p_buffer, nm_len_t*p_len);
  /** release a buffer returned by buf_recv_poll() or buf_recv_wait_any() */
  void (*buf_recv_release)(void*_status);
};
PUK_IFACE_TYPE(NewMad_minidriver, struct nm_minidriver_iface_s);


#endif /* NM_MINIDRIVER_H */
