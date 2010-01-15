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

#define OVERRUN 10

static void nm_test_iovec_fill(char*sbuf, char*rbuf, int iov_count, int entry_size)
{
  int i;
  for(i = 0; i < iov_count; i++)
    {
      int j;
      for(j = 0; j < entry_size; j++)
	{
	  sbuf[i*entry_size + j] = iov_count - i - 1 + j;
	  rbuf[i*entry_size + j] = -1;
	}
    }
}

static void nm_test_iovec_check(char*rbuf, int iov_count, int entry_size)
{
  int i;
  for(i = 0; i < iov_count; ++i)
    {
      int j;
      for(j = 0; j < entry_size; j++)
	{
	  const char expected = i + j;
	  if(rbuf[i*entry_size + j] != expected)
	    {
	      printf("data corruption detected- index %d; value %d; expected = %d\n", i, rbuf[i*entry_size+j], expected);
	      abort();
	    }
	}
    }
}

/* symmetrical iovec, n x 4 bytes, shuffled entries
 * force client -> server as unexpected, server -> client as expected
 */
static void nm_test_iovec_symmetrical_regular(char*sbuf, char*rbuf, int iov_count, int entry_size, nm_gate_t src, int overrun)
{
  int i;
  struct iovec riov[iov_count], siov[iov_count];
  nm_sr_request_t sreq, rreq;

  printf("iovec test, %dx %d bytes%s\n", iov_count, entry_size, (src == NM_ANY_GATE?", anysrc":""));

  nm_test_iovec_fill(sbuf, rbuf, iov_count, entry_size);

  for(i = 0; i < iov_count; i++)
    {
      siov[i].iov_base = &sbuf[i * entry_size];
      siov[i].iov_len = entry_size;
      
      riov[i].iov_base = &rbuf[(iov_count - i - 1) * entry_size];
      riov[i].iov_len = entry_size;
    }
  
  riov[iov_count-1].iov_len += overrun;

  if(is_server)
    {
      usleep(100*1000);
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
  nm_test_iovec_check(rbuf, iov_count, entry_size);
  printf("  ok.\n");
}

static void nm_test_iovec_full(int iov_count, int entry_size)
{
  char*sbuf = malloc(iov_count * entry_size + OVERRUN);
  char*rbuf = malloc(iov_count * entry_size + OVERRUN);
  nm_test_iovec_symmetrical_regular(sbuf, rbuf, iov_count, entry_size, gate_id, 0); 
  nm_test_iovec_symmetrical_regular(sbuf, rbuf, iov_count, entry_size, NM_ANY_GATE, 0); 
  nm_test_iovec_symmetrical_regular(sbuf, rbuf, iov_count, entry_size, gate_id, OVERRUN); 
  nm_test_iovec_symmetrical_regular(sbuf, rbuf, iov_count, entry_size, NM_ANY_GATE, OVERRUN); 
  free(sbuf);
  free(rbuf);
}

int main(int argc, char ** argv)
{
  
  init(&argc, argv);
  
  nm_test_iovec_full(1, 4);
  nm_test_iovec_full(4, 4);
  nm_test_iovec_full(32, 4);
  nm_test_iovec_full(1024, 4);
  nm_test_iovec_full(32*1024, 4);

  nm_test_iovec_full(1, 65*1024);
  nm_test_iovec_full(8, 65*1024);

  nm_test_iovec_full(1,  1024*1024);
  nm_test_iovec_full(4,  1024*1024);
  nm_test_iovec_full(32, 1024*1024);

  nmad_exit();
  return 0;
}
