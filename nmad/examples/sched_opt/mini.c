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
#include <nm_private.h>

const char *msg	= "hello, world";

int main(int	  argc, char**argv)
{
  nm_core_t p_core = NULL;
  struct nm_pkt_wrap*p_pw = NULL;
  char			*r_url		= NULL;
  char			*l_url		= NULL;
  nm_drv_t		 p_drv		= NULL;
  nm_gate_t                gate   	= NULL;
  int err;
  nm_tag_t tag = 128;
  int seq = 0;
  
  err = nm_core_init(&argc, argv, &p_core);
  if (err != NM_ESUCCESS) {
    printf("nm_core_init returned err = %d\n", err);
    goto out;
  }
  
  argc--;
  argv++;
  
  if (argc) {
    r_url	= *argv;
    printf("running as client using remote url: [%s]\n", r_url);
    
    argc--;
    argv++;
  } else {
    printf("running as server\n");
  }
  const char*default_driver = NULL;
#if defined CONFIG_MX
  default_driver = "mx";
#elif defined CONFIG_IBVERBS
  default_driver = "ibverbs";
#elif defined CONFIG_GM
  default_driver = "gm";
#elif defined CONFIG_TCP
  default_driver = "tcp";
#endif
  err = nm_core_driver_load_init(p_core, nm_core_component_load("driver", default_driver), &p_drv, &l_url);
  if (err != NM_ESUCCESS) {
    printf("nm_core_driver_load_init returned err = %d\n", err);
    goto out;
  }
  
  err = nm_core_gate_init(p_core, &gate);
  if (err != NM_ESUCCESS) {
    printf("nm_core_gate_init returned err = %d\n", err);
    goto out;
  }
  
  size_t len = 1 + strlen(msg);
  char*buf = malloc((size_t)len);
  
  if (!r_url) 
    {
      /* server */
      printf("local url: [%s]\n", l_url);
      
      err = nm_core_gate_accept(p_core, gate, p_drv, NULL);
      if (err != NM_ESUCCESS) {
	printf("nm_core_gate_accept returned err = %d\n", err);
	goto out;
      }
      
      memset(buf, 0, len);
      
      err = nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
      if (err != NM_ESUCCESS) {
	printf("nm_so_pw_alloc returned err = %d\n", err);
	goto out;
      }

      err = nm_so_pw_add_data(p_pw, tag, seq, buf, len,
			      0, 1, NM_PW_NOHEADER);

      nm_core_post_recv(p_pw, gate, NM_TRK_SMALL, NM_DRV_DEFAULT);

      struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
      err = (*r->driver->post_recv_iov)(r->_status, p_pw);
      while(err == -NM_EAGAIN)
	{
	  err = (*r->driver->poll_recv_iov)(r->_status, p_pw);
	} 
      if (err != NM_ESUCCESS)
	{
	  printf("poll/post_recv returned err = %d\n", err);
	}
      else
	{
	  printf("received message: %s\n", buf);
	}
    } 
  else
    {
      /* client */
      err = nm_core_gate_connect(p_core, gate, p_drv, r_url);
      if (err != NM_ESUCCESS) {
	printf("nm_core_gate_connect returned err = %d\n", err);
	goto out;
      }
      
      strcpy(buf, msg);
      
      err = nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
      if (err != NM_ESUCCESS) {
	printf("nm_so_pw_alloc returned err = %d\n", err);
	goto out;
      }

      err = nm_so_pw_add_data(p_pw, tag, seq, buf, len,
			      0, 1, NM_PW_NOHEADER);

      nm_core_post_send(gate, p_pw, NM_TRK_SMALL, NM_DRV_DEFAULT);
      
      struct puk_receptacle_NewMad_Driver_s*r = &p_pw->p_gdrv->receptacle;
      err = (*r->driver->post_send_iov)(r->_status, p_pw);
      while(err == -NM_EAGAIN)
	{
	  err = (*r->driver->poll_send_iov)(r->_status, p_pw);
	}
      if (err != NM_ESUCCESS) 
	{
	  printf("poll/post_send returned err = %d\n", err);
	}
    }
  sleep(2);
  
 out:
  return 0;
}

