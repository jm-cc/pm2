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

#define NB_PACKS 4
#define SMALL_SIZE 16
#define LARGE_SIZE (64 * 1024)
#define LOOPS   1000

int main(int argc, char**argv)
{
  char*buf1 = NULL;
  char*buf2 = NULL;
  nm_pack_cnx_t cnx;
  
  buf1 = malloc(SMALL_SIZE);
  memset(buf1, 0, SMALL_SIZE);
  
  buf2 = malloc(LARGE_SIZE);
  memset(buf2, 0, LARGE_SIZE);
  
  nm_examples_init(&argc, argv);
  
  if(is_server)
    {
      int n, k;
      
      /* server */
      for(k = 0; k < LOOPS; k++)
	{
	  nm_begin_unpacking(p_session, p_gate, 0, &cnx);
	  for(n = 0; n < NB_PACKS; n++)
	    {
	      nm_unpack(&cnx, buf2, LARGE_SIZE);
              nm_unpack(&cnx, buf1, SMALL_SIZE);
            }
	  nm_end_unpacking(&cnx);
	  
	  nm_begin_packing(p_session, p_gate, 0, &cnx);
	  for(n = 0; n < NB_PACKS; n++)
	    {
              nm_pack(&cnx, buf2, LARGE_SIZE);
              nm_pack(&cnx, buf1, SMALL_SIZE);
            }
	  nm_end_packing(&cnx);
	}
    }
  else
    {
      tbx_tick_t t1, t2;
      int n, k;
      
      /* client */
      TBX_GET_TICK(t1);
      for(k = 0; k < LOOPS; k++)
	{
	  nm_begin_packing(p_session, p_gate, 0, &cnx);
	  for(n = 0; n < NB_PACKS; n++)
	    {
              nm_pack(&cnx, buf2, LARGE_SIZE);
              nm_pack(&cnx, buf1, SMALL_SIZE);
            }
	  nm_end_packing(&cnx);
	  
	  nm_begin_unpacking(p_session, p_gate, 0, &cnx);
	  for(n = 0; n < NB_PACKS; n++)
	    {
              nm_unpack(&cnx, buf2, LARGE_SIZE);
              nm_unpack(&cnx, buf1, SMALL_SIZE);
            }
	  nm_end_unpacking(&cnx);
	}
      TBX_GET_TICK(t2);
      
      printf("%d x (%d + %d) \t%lf usec.\n", NB_PACKS, LARGE_SIZE, SMALL_SIZE, TBX_TIMING_DELAY(t1, t2)/(2*LOOPS));
      
    }
  
  free(buf1);
  free(buf2);
  nm_examples_exit();
  return 0;
}
