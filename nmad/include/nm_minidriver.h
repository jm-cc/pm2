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


struct nm_minidriver_iface_s
{
  /* connection establishment */
  void (*init)(puk_context_t context, const void**drv_url, size_t*url_size);
  void (*connect)(void*_status, const void*remote_url, size_t url_size);
  /* sending primitives */
  void (*send_post)(void*_status, const struct iovec*v, int n);
  int  (*send_poll)(void*_status);
  void (*send_prefetch)(void*_status, const void*ptr, uint64_t size);
  /* receiving primitives */
  void (*recv_init)(void*_status, struct iovec*v, int n);
  int  (*poll_one)(void*_status);
  int  (*cancel_recv)(void*_status); 
};
PUK_IFACE_TYPE(NewMad_minidriver, struct nm_minidriver_iface_s);


#endif /* NM_MINIDRIVER_H */
