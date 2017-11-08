/*
 * NewMadeleine
 * Copyright (C) 2011-2017 (see AUTHORS file)
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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <assert.h>
#include <values.h>

#include <nm_private.h>
#include <nm_launcher_interface.h>

const int roundtrips = 100000;

static inline void minidriver_send(struct puk_receptacle_NewMad_minidriver_s*r, void*buf, nm_len_t len)
{
  int rc = -1;
  struct iovec v = { .iov_base = buf, .iov_len = len };
  struct nm_data_s data;
  if(r->driver->buf_send_get)
    {
      void*p_driver_buf = NULL;
      nm_len_t driver_len = NM_LEN_UNDEFINED;
      (*r->driver->buf_send_get)(r->_status, &p_driver_buf, &driver_len);
      memcpy(p_driver_buf, buf, len);
      (*r->driver->buf_send_post)(r->_status, len);
    }
  else if(r->driver->send_post)
    {
      (*r->driver->send_post)(r->_status, &v, 1);
    }
  else
    {
      nm_data_contiguous_build(&data, buf, len);
      (*r->driver->send_data)(r->_status, &data, 0, len);
    }
  do
    {
      rc = (*r->driver->send_poll)(r->_status);
    }
  while(rc != 0);
}

static inline void minidriver_recv(struct puk_receptacle_NewMad_minidriver_s*r, char*buf, size_t len)
{
  int rc = -1;
  struct iovec v = { .iov_base = buf, .iov_len = len };
  struct nm_data_s data;
  if(r->driver->recv_buf_poll)
    {
      void*p_driver_buf = NULL;
      nm_len_t driver_len = NM_LEN_UNDEFINED;
      do
	{
	  rc = (*r->driver->recv_buf_poll)(r->_status, &p_driver_buf, &driver_len);
	}
      while(rc != 0);
      memcpy(buf, p_driver_buf, len);
      (*r->driver->recv_buf_release)(r->_status);
    }
  else
    {
      if(r->driver->recv_iov_post)
	{
	  (*r->driver->recv_iov_post)(r->_status, &v, 1);
	}
      else
	{
	  nm_data_contiguous_build(&data, buf, len);
	  (*r->driver->recv_data_post)(r->_status, &data, 0, len);
	}
      do
	{
	  rc = (*r->driver->recv_poll_one)(r->_status);
	}
      while(rc != 0);
    }
}


int main(int argc, char **argv)
{
  /* launch nmad session, get gate and driver */
  nm_launcher_init(&argc, argv);
  int rank = -1;
  nm_launcher_get_rank(&rank);
  const int is_server = !rank;
  const int peer = 1 - rank;
  nm_gate_t p_gate = NULL;
  nm_launcher_get_gate(peer, &p_gate);
  assert(p_gate != NULL);
  assert(p_gate->status == NM_GATE_STATUS_CONNECTED);

  /* take over the driver */
  nm_core_schedopt_disable(p_gate->p_core);

  /* hack here-
   * make sure the peer node has flushed its pending recv requests,
   * so that the pw we send are not processed by schedopt.
   */
  usleep(500 * 1000);

  /* benchmark */
  struct nm_trk_s*p_trk_small = nm_trk_get_by_index(p_gate, NM_TRK_SMALL);
  struct puk_receptacle_NewMad_minidriver_s*r = &p_trk_small->receptacle;
  char value = 42;
  void*buffer = &value;
  const nm_len_t len = sizeof(value);

  if(is_server)
    {
      double min = DBL_MAX;
      int i;
      for(i = 0; i < roundtrips; i++)
	{
	  tbx_tick_t t1, t2;
	  TBX_GET_TICK(t1);
	  minidriver_send(r, buffer, len);
	  minidriver_recv(r, buffer, len);
	  TBX_GET_TICK(t2);
	  const double delay = TBX_TIMING_DELAY(t1, t2);
	  const double t = delay / 2;
	  if(t < min)
	    min = t;
	}
      printf("driver latency:   %9.3f usec.\n", min);
    }
  else
    {
      int i;
      for(i = 0; i < roundtrips; i++)
	{
	  minidriver_recv(r, buffer, len);
	  minidriver_send(r, buffer, len);
	}
    }

  return 0;
}
