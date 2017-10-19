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


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "../common/nm_examples_helper.h"
#include <nm_pack_interface.h>

#define MAX     (1024 * 1024)
#define LOOPS   100

int main(int argc, char**argv)
{
  nm_pack_cnx_t cnx;
  char*buf = NULL;
  nm_len_t len;
  
  nm_examples_init(&argc, argv);
  
  buf = malloc(MAX);
  memset(buf, 0, MAX);
  
  if(is_server)
    {
      int k;
      /* server
       */
      for(len = 0; len <= MAX; len = _next(len, 2, 0))
	{
	  for(k = 0; k < LOOPS; k++)
	    {
	      nm_begin_unpacking(p_session, p_gate, 0, &cnx);
	      nm_unpack(&cnx, buf, len);
	      nm_end_unpacking(&cnx);
	      
	      nm_begin_packing(p_session, p_gate, 0, &cnx);
	      nm_pack(&cnx, buf, len);
	      nm_end_packing(&cnx);
	    }
	}
    }
  else
    {
      tbx_tick_t t1, t2;
      double sum, lat, bw_million_byte, bw_mbyte;
      int k;
      /* client
       */
      printf(" size     |  latency     |   10^6 B/s   |   MB/s    |\n");
      
      for(len = 0; len <= MAX; len = _next(len, 2, 0))
	{
	  TBX_GET_TICK(t1);
	  for(k = 0; k < LOOPS; k++)
	    {
	      nm_begin_packing(p_session, p_gate, 0, &cnx);
	      nm_pack(&cnx, buf, len);
	      nm_end_packing(&cnx);
	      
	      nm_begin_unpacking(p_session, p_gate, 0, &cnx);
	      nm_unpack(&cnx, buf, len);
	      nm_end_unpacking(&cnx);
	    }
	  
	  TBX_GET_TICK(t2);
	  
	  sum = TBX_TIMING_DELAY(t1, t2);
	  
	  lat	      = sum / (2 * LOOPS);
	  bw_million_byte = len * (LOOPS / (sum / 2));
	  bw_mbyte        = bw_million_byte / 1.048576;
	  
	  printf("%lld\t%lf\t%8.3f\t%8.3f\n",
		 (long long)len, lat, bw_million_byte, bw_mbyte);
	}
    }
  
  free(buf);
  nm_examples_exit();
  return 0;
}
