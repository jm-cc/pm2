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

#include <err.h>
#include <stdio.h>

#include "helper.h"

/* symmetrical iovec, n x 4 bytes, shuffled entries
 * force client -> server as unexpected, server -> client as expected
 */
static void nm_test_iovec_symmetrical_regular(nm_gate_t src, int iov_count)
{
  int i;
  int sbuf[iov_count];
  int rbuf[iov_count];
  struct iovec riov[iov_count], siov[iov_count];
  nm_sr_request_t sreq, rreq;

  for(i = 0; i < iov_count; i++)
    {
      sbuf[i] = iov_count - i - 1;
      siov[i].iov_base = &sbuf[i];
      siov[i].iov_len = sizeof(int);
      
      rbuf[i] = -1;
      riov[i].iov_base = &rbuf[iov_count - i - 1];
      riov[i].iov_len = sizeof(int);
    }

  if(is_server)
    {
      sleep(1);
      nm_sr_isend_iov(p_core, gate_id, 42, siov, iov_count, &sreq);
      nm_sr_swait(p_core, &sreq);
      nm_sr_irecv_iov(p_core, src, 42, riov, iov_count, &rreq);
    }
  else
    {
      nm_sr_irecv_iov(p_core, src, 42, riov, iov_count, &rreq);
      nm_sr_isend_iov(p_core, gate_id, 42, siov, iov_count, &sreq);
      nm_sr_swait(p_core, &sreq);
    }
  nm_sr_rwait(p_core, &rreq);
  for(i = 0; i < iov_count; ++i)
    {
      if(rbuf[i] != i)
	{
	  printf("data corruption detected- index %d value %d\n", i, rbuf[i]);
	  abort();
	}
    }
}

int main(int argc, char ** argv)
{
  
  init(&argc, argv);
  
  printf("iovec test, 1x 4 bytes\n");
  nm_test_iovec_symmetrical_regular(gate_id, 1);
  printf("  ok.\n");

  printf("iovec test, 1x 4 bytes, anysrc\n");
  nm_test_iovec_symmetrical_regular(NM_ANY_GATE, 1);
  printf("  ok.\n");
  
  printf("iovec test, 4x 4 bytes\n");
  nm_test_iovec_symmetrical_regular(gate_id, 4);
  printf("  ok.\n");

  printf("iovec test, 4x 4 bytes, anysrc\n");
  nm_test_iovec_symmetrical_regular(NM_ANY_GATE, 4);
  printf("  ok.\n");

  printf("iovec test, 32x 4 bytes\n");
  nm_test_iovec_symmetrical_regular(gate_id, 32);
  printf("  ok.\n");

  printf("iovec test, 32x 4 bytes, anysrc\n");
  nm_test_iovec_symmetrical_regular(NM_ANY_GATE, 32);
  printf("  ok.\n");

  printf("iovec test, 1024x 4 bytes\n");
  nm_test_iovec_symmetrical_regular(gate_id, 1024);
  printf("  ok.\n");

  printf("iovec test, 1024x 4 bytes, anysrc\n");
  nm_test_iovec_symmetrical_regular(NM_ANY_GATE, 1024);
  printf("  ok.\n");

  nmad_exit();
  return 0;
}
