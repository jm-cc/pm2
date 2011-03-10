/*
 * NewMadeleine
 * Copyright (C) 2011 (see AUTHORS file)
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
#include <nm_session_interface.h>
#include <nm_launcher.h>

#include "../sendrecv/clock.h"

const int roundtrips = 50000;

int main(int argc, char **argv)
{
  clock_init();
  int rank, peer;
  nm_launcher_init(&argc, argv);
  nm_session_t p_session = NULL;
  nm_launcher_get_session(&p_session);
  nm_launcher_get_rank(&rank);
  const int is_server = !rank;
  peer = 1 - rank;
  nm_gate_t p_gate = NULL;
  nm_launcher_get_gate(peer, &p_gate);
  struct nm_drv*p_drv = nm_drv_default(p_gate);
  assert(p_gate != NULL);
  assert(p_gate->status == NM_GATE_STATUS_CONNECTED);

  /* benchmark */
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  struct puk_receptacle_NewMad_Driver_s*r = &p_gdrv->receptacle;
  static struct nm_pkt_wrap*p_pw = NULL;
  char buffer = 42;
  nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
  nm_so_pw_add_raw(p_pw, &buffer, sizeof(buffer), 0);
  nm_so_pw_assign(p_pw, NM_TRK_SMALL, p_drv, p_gate);
  
  p_pw->v[0].iov_base = &buffer;
  p_pw->v[0].iov_len  = sizeof(buffer);
  p_pw->length = 1;

  if(is_server)
    {
      double min = DBL_MAX;
      int i;
      for(i = 0; i < roundtrips; i++)
	{
	  struct timespec t1, t2;
	  clock_gettime(CLOCK_MONOTONIC, &t1);
	  int err = r->driver->post_send_iov(r->_status, p_pw);
	  while(err == -NM_EAGAIN)
	    {
	      err = r->driver->poll_send_iov(r->_status, p_pw);
	    } 
	  if(err != NM_ESUCCESS)
	    abort();
	  err = r->driver->post_recv_iov(r->_status, p_pw);
	  while(err == -NM_EAGAIN)
	    {
	      err = r->driver->poll_recv_iov(r->_status, p_pw);
	    }
	  if(err != NM_ESUCCESS)
	    abort();
	  clock_gettime(CLOCK_MONOTONIC, &t2);
	  const double delay = clock_diff(t1, t2);
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
	  int err = r->driver->post_recv_iov(r->_status, p_pw);
	  while(err == -NM_EAGAIN)
	    {
	      err = r->driver->poll_recv_iov(r->_status, p_pw);
	    }
	  if(err != NM_ESUCCESS)
	    abort();
	  err = r->driver->post_send_iov(r->_status, p_pw);
	  while(err == -NM_EAGAIN)
	    {
	      err = r->driver->poll_send_iov(r->_status, p_pw);
	    }  
	  if(err != NM_ESUCCESS)
	    abort();
	}
    }

  return 0;
}
