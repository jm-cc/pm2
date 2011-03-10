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
#include <values.h>

#include "helper.h"
#include "../sendrecv/clock.h"

#define LOOPS 50000

int main(int argc, char**argv)
{
  nm_pack_cnx_t cnx;
  
  init(&argc, argv);
  clock_init();

  if (is_server)
    {
      /* server
       */
      int i;
      for(i = 0; i < LOOPS; i++)
	{
	  nm_begin_unpacking(p_core, gate_id, 0, &cnx);
	  nm_unpack(&cnx, NULL, 0);
	  nm_end_unpacking(&cnx);

	  nm_begin_packing(p_core, gate_id, 0, &cnx);
	  nm_pack(&cnx, NULL, 0);
	  nm_end_packing(&cnx);
	}
    }
  else
    {
      /* client
       */
      int i;
      double min = DBL_MAX;
      for(i = 0; i < LOOPS; i++) 
	{
	  struct timespec t1, t2;
	  clock_gettime(CLOCK_MONOTONIC, &t1);

	  nm_begin_packing(p_core, gate_id, 0, &cnx);
	  nm_pack(&cnx, NULL, 0);
	  nm_end_packing(&cnx);
	  
	  nm_begin_unpacking(p_core, gate_id, 0, &cnx);
	  nm_unpack(&cnx, NULL, 0);
	  nm_end_unpacking(&cnx);
	  clock_gettime(CLOCK_MONOTONIC, &t2);
	  const double delay = clock_diff(t1, t2);
	  const double t = delay / 2.0;
	  if(t < min)
	    min = t;
	}
      
      printf("pack latency:     %9.3lf usec.\n", min);
      
    }
  
  nmad_exit();
  exit(0);
}
