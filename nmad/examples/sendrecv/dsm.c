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

#include "helper.h"

#define LOOPS      2000
#define PAGE_SIZE (65 * 1024)
#define NB_PAGES   3

int main(int argc, char **argv)
{
  nm_examples_init(&argc, argv);
  
  if (is_server)
    {
      int k;
      /* server */

      int n, i = 0, j = 0;
      
      nm_sr_request_t r_request[NB_PAGES * 4];
      nm_sr_request_t s_request[NB_PAGES * 3];
      
      /*reception */
      int source[NB_PAGES];
      int numero_page[NB_PAGES];
      tbx_bool_t envoi_diff[NB_PAGES];
      int type_acces[NB_PAGES];
      
      /* envoi */
      int n_page = 4;
      tbx_bool_t trouve = tbx_true;
      char *page = malloc(PAGE_SIZE);
      memset(page, 'a', PAGE_SIZE);
      
      for(k = 0; k < LOOPS; k++)
	{
	  i = 0;
	  j = 0;
	  for(n = 0; n < NB_PAGES; n++)
	    {
	      nm_sr_irecv(p_session, p_gate, 0, &source[n],      sizeof(int), &r_request[i++]);
	      nm_sr_irecv(p_session, p_gate, 0, &numero_page[n], sizeof(int), &r_request[i++]);
	      nm_sr_irecv(p_session, p_gate, 0, &envoi_diff[n],  sizeof(tbx_bool_t), &r_request[i++]);
	      nm_sr_irecv(p_session, p_gate, 0, &type_acces[n],  sizeof(int), &r_request[i++]);
	      
	      nm_sr_isend(p_session, p_gate, 0, &n_page, sizeof(int), &s_request[j++]);
	      nm_sr_isend(p_session, p_gate, 0, &trouve, sizeof(tbx_bool_t), &s_request[j++]);
	      nm_sr_isend(p_session, p_gate, 0, page, PAGE_SIZE, &s_request[j++]);
	    }
	  for(n = 0; n < i; n++)
	    nm_sr_rwait(p_session, &r_request[n]);
	  for(n = 0; n < j; n++)
	    nm_sr_swait(p_session, &s_request[n]);
	}
    }
  else 
    {
      tbx_tick_t t1, t2;
      double sum, tps_par_page, tps;
      int k;
      /* client */
      
      int n, i = 0, j = 0;
      
      nm_sr_request_t r_request[NB_PAGES * 3];
      nm_sr_request_t s_request[NB_PAGES * 4];
      
      /* reception */
      int numero_page[NB_PAGES];
      tbx_bool_t trouve[NB_PAGES];
      char *page = malloc(PAGE_SIZE);
      memset(page, '0', PAGE_SIZE);
      
      /* envoi */
      int mon_id = 56;
      int n_page = 4;
      tbx_bool_t envoi_diff = tbx_true;
      int type_acces = 1;
      
      printf("page_size | nb pages | lat par page | lat\n");
      
      TBX_GET_TICK(t1);
      
      for(k = 0; k < LOOPS; k++)
	{
	  i = 0;
	  j = 0;
	  for(n = 0; n < NB_PAGES; n++)
	    {
	      nm_sr_isend(p_session, p_gate, 0, &mon_id, sizeof(int), &s_request[j++]);
	      nm_sr_isend(p_session, p_gate, 0, &n_page, sizeof(int), &s_request[j++]);
	      nm_sr_isend(p_session, p_gate, 0, &envoi_diff, sizeof(tbx_bool_t), &s_request[j++]);
	      nm_sr_isend(p_session, p_gate, 0, &type_acces, sizeof(int), &s_request[j++]);
	      
	      nm_sr_irecv(p_session, p_gate, 0, &numero_page[n], sizeof(int), &r_request[i++]);
	      nm_sr_irecv(p_session, p_gate, 0, &trouve[n],  sizeof(tbx_bool_t), &r_request[i++]);
	      nm_sr_irecv(p_session, p_gate, 0, page,  sizeof(int), &r_request[i++]);
	    }
	  
	  for(n = 0; n < j; n++)
	    nm_sr_swait(p_session, &s_request[n]);
	  for(n = 0; n < i; n++)
	    nm_sr_rwait(p_session, &r_request[n]);
	}
      
      TBX_GET_TICK(t2);
      
      sum = TBX_TIMING_DELAY(t1, t2);
      tps = sum / LOOPS;
      tps_par_page = sum / NB_PAGES / LOOPS;
      
      printf("%d\t%d\t%lf\t%lf\n", PAGE_SIZE, NB_PAGES, tps_par_page, tps);
    }
  nm_examples_exit();
  exit(0);
}
