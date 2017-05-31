/*
 * NewMadeleine
 * Copyright (C) 2013 (see AUTHORS file)
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
#include <tbx.h>
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
struct nm_drv_cap_s
{
  int has_recv_any;   /**< driver accepts receive from NM_GATE_ANY */
  int rdv_threshold;  /**< preferred length for switching to rendez-vous. */
  int min_period;     /**< minimum delay between poll (in microseconds) */
  int is_exportable;  /**< blocking calls may be exported by PIOMan */
  int max_unexpected; /**< maximum size of unexpected messages on trk #0 */
  int supports_data;  /**< driver can send/recv direct nm_data_s */
};

struct nm_minidriver_properties_s
{
  struct nm_drv_profile_s profile;
  struct nm_drv_cap_s capabilities;
};

/** Interface driver for the 'NewMad_minidriver' component interface.
 * A driver interface simpler than NewMad_Driver, that do not rely 
 * on nmad core structures, making driver fully encapsulated.
 */
struct nm_minidriver_iface_s
{
  void (*getprops)(int index, struct nm_minidriver_properties_s*props);
  /* connection establishment */
  void (*init)(puk_context_t context, const void**drv_url, size_t*url_size);
  void (*connect)(void*_status, const void*remote_url, size_t url_size);
  void (*close)(puk_context_t context);
  /* sending primitives */
  void (*send_post)(void*_status, const struct iovec*v, int n);
  void (*send_data)(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
  int  (*send_poll)(void*_status);
  void (*send_prefetch)(void*_status, const void*ptr, nm_len_t size);
  /* receiving primitives */
  void (*recv_init)(void*_status, struct iovec*v, int n);
  void (*recv_data)(void*_status, const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len);
  int  (*poll_one)(void*_status);
  int  (*cancel_recv)(void*_status);
  /* buffer-based sending */
  void (*buf_send_get)(void*_status, void**p_buffer, nm_len_t*p_len);
  void (*buf_send_post)(void*_status, nm_len_t len);
  int  (*buf_send_poll)(void*_status);
};
PUK_IFACE_TYPE(NewMad_minidriver, struct nm_minidriver_iface_s);


#endif /* NM_MINIDRIVER_H */
