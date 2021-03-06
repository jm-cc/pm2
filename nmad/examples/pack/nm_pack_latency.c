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

#include "../common/nm_examples_helper.h"
#include <nm_pack_interface.h>

#define LOOPS 50000

int main(int argc, char**argv)
{
  nm_pack_cnx_t cnx;
  
  nm_examples_init(&argc, argv);

  if (is_server)
    {
      /* server
       */
      int i;
      for(i = 0; i < LOOPS; i++)
	{
	  nm_begin_unpacking(p_session, p_gate, 0, &cnx);
	  nm_unpack(&cnx, NULL, 0);
	  nm_end_unpacking(&cnx);

	  nm_begin_packing(p_session, p_gate, 0, &cnx);
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
	  tbx_tick_t t1, t2;
	  TBX_GET_TICK(t1);

	  nm_begin_packing(p_session, p_gate, 0, &cnx);
	  nm_pack(&cnx, NULL, 0);
	  nm_end_packing(&cnx);
	  
	  nm_begin_unpacking(p_session, p_gate, 0, &cnx);
	  nm_unpack(&cnx, NULL, 0);
	  nm_end_unpacking(&cnx);
	  TBX_GET_TICK(t2);
	  const double delay = TBX_TIMING_DELAY(t1, t2);
	  const double t = delay / 2.0;
	  if(t < min)
	    min = t;
	}
      
      printf("pack latency:     %9.3lf usec.\n", min);
      
    }
  
  nm_examples_exit();
  exit(0);
}
