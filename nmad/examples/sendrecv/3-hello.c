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

#include <nm_public.h>
#include <nm_sendrecv_interface.h>
#include "helper.h"

const char *msg	= "hello, world";

#ifdef CONFIG_PROTO_MAD3
int main(int argc, char **argv) {
        printf("This program does not work with the protocol mad3\n");
	exit(0);
}
#else
int main(int argc, char	**argv)
{
  nm_core_t		p_core		= NULL;
  char			*r_url1		= NULL;
  char			*r_url2		= NULL;
  char			*l_url		= NULL;
  nm_drv_id_t		 drv_id		=    0;
  nm_gate_t		 gate1   	=    0;
  nm_gate_t		 gate2   	=    0;
  int err;
  
  err = nm_core_init(&argc, argv, &p_core);
  if (err != NM_ESUCCESS) {
    printf("nm_core_init returned err = %d\n", err);
    goto out;
  }
  
  argc--;
  argv++;
  
  if (argc) {
    /* client */
    
    r_url1	= *argv;
    argc--;
    argv++;
    
    
    if(argc){
      r_url2	= *argv;
      argc--;
      argv++;
      printf("running as client using remote url: %s %s\n", r_url1, r_url2);
    } else {
      printf("running as client using remote url: %s\n", r_url1);
    }
    
  } else {
    /* no args: server */
    printf("running as server\n");
  }
  
#if defined CONFIG_IBVERBS
  err = nm_core_driver_load_init(p_core, nm_core_component_load("driver", "ibverbs"), &drv_id, &l_url);
#elif defined CONFIG_MX
  err = nm_core_driver_load_init(p_core, nm_core_component_load("driver", "mx"), &drv_id, &l_url);
#elif defined CONFIG_GM
  err = nm_core_driver_load_init(p_core, nm_core_component_load("driver", "gm"), &drv_id, &l_url);
#elif defined CONFIG_QSNET
  err = nm_core_driver_load_init(p_core, nm_core_component_load("driver", "qsnet"), &drv_id, &l_url);
#elif defined CONFIG_SISCI
  err = nm_core_driver_load_init(p_core, nm_core_component_load("driver", "sisci"), &drv_id, &l_url);
#elif defined CONFIG_TCP
  err = nm_core_driver_load_init(p_core, nm_core_component_load("driver", "tcp"), &drv_id, &l_url);
#endif
  if (err != NM_ESUCCESS) {
    printf("nm_core_driver_load_init returned err = %d\n", err);
    goto out;
  }

  err = nm_sr_init(p_core);
  if (err != NM_ESUCCESS) {
    printf("nm_sr_init returned err = %d\n", err);
    goto out;
  }  

  err = nm_core_gate_init(p_core, &gate1);
  if (err != NM_ESUCCESS) {
    printf("nm_core_gate_init returned err = %d\n", err);
    goto out;
  }
  
  err = nm_core_gate_init(p_core, &gate2);
  if (err != NM_ESUCCESS) {
    printf("nm_core_gate_init returned err = %d\n", err);
    goto out;
  }
  
  printf("local url: [%s]\n", l_url);

  if(r_url1)
    {
      fprintf(stderr, "connecting gate1 to url: %s\n", r_url1);
      err = nm_core_gate_connect(p_core, gate1, drv_id, r_url1);
      if (err != NM_ESUCCESS) 
	{
	  printf("nm_core_gate_connect returned err = %d\n", err);
	  goto out;
	}
    }
  else
    {
      fprintf(stderr, "accepting connections on gate1\n");
      err = nm_core_gate_accept(p_core, gate1, drv_id, NULL);
      if (err != NM_ESUCCESS) 
	{
	  printf("nm_core_gate_accept returned err = %d\n", err);
	  goto out;
	}
    }
  fprintf(stderr, "gate1 is connected.\n");
  if(r_url2)
    {
      fprintf(stderr, "connecting gate2 to url: %s\n", r_url2);
      err = nm_core_gate_connect(p_core, gate2, drv_id, r_url2);
      if (err != NM_ESUCCESS) 
	{
	  printf("nm_core_gate_connect returned err = %d\n", err);
	  goto out;
	}
    }
  else
    {
      fprintf(stderr, "accepting connections on gate2\n");
      err = nm_core_gate_accept(p_core, gate2, drv_id, NULL);
      if (err != NM_ESUCCESS) 
	{
	  printf("nm_core_gate_accept returned err = %d\n", err);
	  goto out;
	}
    }
  fprintf(stderr, "gate2 is connected.\n");

  size_t len = 1 + strlen(msg);
  char*s_buf = malloc(len);
  char*r_buf = malloc(len);
  strcpy(s_buf, msg);
  memset(r_buf, 0, len);
  
  if(!r_url1 && !r_url2)
    {
      nm_sr_request_t request;
      
      fprintf(stderr, "sending message on gate1...\n");
      err = nm_sr_isend(p_core, gate1, 0, s_buf, len, &request);
      nm_sr_swait(p_core, &request);
    
      nm_sr_irecv(p_core, NM_ANY_GATE, 0, r_buf, len, &request);
      nm_sr_rwait(p_core, &request);
    }
  else 
    {
      nm_sr_request_t request;
      nm_gate_t gate = NM_ANY_GATE;
      
      nm_sr_irecv(p_core, NM_ANY_GATE, 0, r_buf, len, &request);
      nm_sr_rwait(p_core, &request);
      
      nm_sr_recv_source(p_core, &request, &gate);
    
      if(gate == gate1)
	{
	  fprintf(stderr, "got message from gate1- sending message on gate2...\n");
	  nm_sr_isend(p_core, gate2, 0, s_buf, len, &request);
	  nm_sr_swait(p_core, &request);
	} 
      else 
	{
	  fprintf(stderr, "got message from gate2- sending message on gate1...\n");
	  nm_sr_isend(p_core, gate1, 0, s_buf, len, &request);
	  nm_sr_swait(p_core, &request);
	}
    }
  
  printf("buffer contents: %s\n", r_buf);
  
 out:
  return 0;
}

// sur le noeud 1 : pm2-load 3-hello
// sur le noeud 2 : pm2-load 3-hello "url_1"
// sur le neoud 3 : pm2-load 3-hello "url_1" "url_2"

#endif

