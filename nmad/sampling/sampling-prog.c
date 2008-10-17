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

#ifndef CONFIG_PROTO_MAD3
#  error "sampling-prog requires option 'mad3_emu'"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <assert.h>

#include <nm_private.h>
#include <madeleine.h>

static const int param_min_size = 2;
static const int param_max_size = (8*1024*1024);
static const int param_nb_samples = 1000;

static void nm_ns_print_errno(char *msg, int err)
{
  err = (err > 0 ? err : -err);

}

static int nm_ns_initialize_pw(struct nm_core *p_core,
			       struct nm_drv  *p_drv,
			       struct nm_gate *p_gate,
			       unsigned char *ptr,
			       struct nm_pkt_wrap **pp_pw)
{
  struct nm_pkt_wrap *p_pw = NULL;
  const uint8_t flags = NM_SO_DATA_DONT_USE_HEADER;
  const nm_tag_t tag = 0;
  const uint8_t seq = 0;
  int err = nm_so_pw_alloc(flags, &p_pw);
  if(err != NM_ESUCCESS){
    nm_ns_print_errno("nm_ns_initialize_pw - nm_so_pw_alloc", err);
    goto out;
  }
  err = nm_so_pw_add_data(p_pw, tag, seq, ptr, param_min_size, 0, 1, flags);
  if(err != NM_ESUCCESS){
    nm_ns_print_errno("nm_ns_initialize_pw - nm_so_pw_add_data", err);
    goto out;
  }

  p_pw->p_gate = p_gate;
  p_pw->p_drv  = p_drv;
  p_pw->trk_id = NM_TRK_SMALL;

  *pp_pw	= p_pw;

 out:
  return err;
}

static void nm_ns_fill_pw_data(struct nm_pkt_wrap *p_pw)
{
  unsigned char * data = p_pw->v[0].iov_base;
  unsigned char c = 0;
  int i = 0, n = 0;

  for (i = 0; i < param_max_size; i++){
    n += 7;
    c = (unsigned char)(n % 256);
    data[i] = c;
  }
}

static void nm_ns_update_pw(struct nm_pkt_wrap *p_pw, unsigned char *ptr, int len)
{
  p_pw->v[0].iov_base	= ptr;
  p_pw->v[0].iov_len	= len;

  p_pw->drv_priv   = NULL;
  p_pw->gate_priv  = NULL;
}

static void nm_ns_free(struct nm_core *p_core, struct nm_pkt_wrap *p_pw)
{
  nm_so_pw_free(p_pw);
}


static int nm_ns_ping(struct nm_drv *driver, struct nm_gate *p_gate, void *status, FILE*sampling_file)
{
  const struct nm_drv_iface_s *drv_ops = driver->driver;
  struct nm_pkt_wrap * sending_pw = NULL;
  struct nm_pkt_wrap * receiving_pw = NULL;
  unsigned char *data_send	= NULL;
  unsigned char *data_recv 	= NULL;
  int nb_samples = 0;
  int size = 0;
  int err;
  int i = 0;

  data_send = TBX_MALLOC(param_max_size);
  err = nm_ns_initialize_pw(driver->p_core, driver, p_gate, data_send, &sending_pw);
  if(err != NM_ESUCCESS){
    nm_ns_print_errno("nm_ns_ping - nm_ns_initialize_sending_pw",
                      err);
    goto out;
  }

  nm_ns_fill_pw_data(sending_pw);

  data_recv = TBX_MALLOC(param_max_size);
  err = nm_ns_initialize_pw(driver->p_core, driver, p_gate, data_recv, &receiving_pw);
  if(err != NM_ESUCCESS){
    nm_ns_print_errno("nm_ns_ping - nm_ns_initialize_receiving_pw",
                      err);
    goto out;
  }

  double sum, lat, bw_million_byte, bw_mbyte;
  tbx_tick_t t1, t2;

  for (i = 0, size = param_min_size; size <= param_max_size; i++, size*= 2) {

    nm_ns_update_pw(sending_pw, data_send, size);
    nm_ns_update_pw(receiving_pw, data_recv, size);

    TBX_GET_TICK(t1);
    while (nb_samples++ < param_nb_samples) {

      err = drv_ops->post_send_iov(status, sending_pw);
      if(err != NM_ESUCCESS && err != -NM_EAGAIN){
        nm_ns_print_errno("nm_ns_ping - post_send_iov",
                          err);
        goto out;
      }

      while(err != NM_ESUCCESS){
        err = drv_ops->poll_send_iov(status, sending_pw);

        if(err != NM_ESUCCESS && err != -NM_EAGAIN){
          nm_ns_print_errno("nm_ns_ping - poll_send_iov",
                            err);

          goto out;
        }

      }
      nm_ns_update_pw(sending_pw,  data_send, size);


      err = drv_ops->post_recv_iov(status, receiving_pw);
      if(err != NM_ESUCCESS && err != -NM_EAGAIN){
        nm_ns_print_errno("nm_ns_ping - post_recv_iov",
                          err);
        goto out;
      }

      while(err != NM_ESUCCESS){
        err = drv_ops->poll_recv_iov(status, receiving_pw);
        if(err != NM_ESUCCESS && err != -NM_EAGAIN){
          nm_ns_print_errno("nm_ns_ping - poll_recv_iov",
                            err);
          goto out;
        }
      }
      nm_ns_update_pw(receiving_pw, data_recv, size);

    }
    TBX_GET_TICK(t2);

    sum = TBX_TIMING_DELAY(t1, t2);
    lat	            = sum / (2 * param_nb_samples);
    bw_million_byte = size * (param_nb_samples / (sum / 2));
    bw_mbyte        = bw_million_byte / 1.048576;

    fprintf(stderr, "%d\t%lf\t%lf\n", size, lat, bw_mbyte);
    fprintf(sampling_file, "%d\t%lf\n", size, bw_mbyte);

    nb_samples = 0;
    sum = 0.0;
    size = (size==0? 1:size);
  }
  nm_ns_free(driver->p_core, sending_pw);
  nm_ns_free(driver->p_core, receiving_pw);

  err = NM_ESUCCESS;

 out:
  return err;
}

static int nm_ns_pong(struct nm_drv *driver, struct nm_gate *p_gate, void *status)
{
  const struct nm_drv_iface_s *drv_ops = driver->driver;
  struct nm_pkt_wrap * sending_pw = NULL;
  struct nm_pkt_wrap * receiving_pw = NULL;
  unsigned char *data_send          = NULL;
  unsigned char *data_recv          = NULL;
  int nb_samples = 0;
  int size = 0;
  int err;
  int i = 0;

  data_send = TBX_MALLOC(param_max_size);
  err = nm_ns_initialize_pw(driver->p_core, driver, p_gate, data_send, &sending_pw);
  if(err != NM_ESUCCESS){
    nm_ns_print_errno("nm_ns_pong - nm_ns_initialize_sending_pw",
                      err);
    goto out;
  }

  nm_ns_fill_pw_data(sending_pw);

  data_recv = TBX_MALLOC(param_max_size);
  err = nm_ns_initialize_pw(driver->p_core, driver, p_gate, data_recv, &receiving_pw);
  if(err != NM_ESUCCESS){
    nm_ns_print_errno("nm_ns_pong - nm_ns_initialize_receiving_pw",
                      err);
    goto out;
  }

  for (i = 0, size = param_min_size; size <= param_max_size; i++, size*= 2)
    {
      nm_ns_update_pw(sending_pw,    data_send, size);
      nm_ns_update_pw(receiving_pw, data_recv, size);
      
      err = drv_ops->post_recv_iov(status, receiving_pw);
      if(err != NM_ESUCCESS && err != -NM_EAGAIN){
	nm_ns_print_errno("nm_ns_pong - post_recv_iov",
			  err);
	goto out;
      }
      while(err != NM_ESUCCESS){
	err = drv_ops->poll_recv_iov(status, receiving_pw);
	if(err != NM_ESUCCESS && err != -NM_EAGAIN){
	  nm_ns_print_errno("nm_ns_pong - poll_recv_iov",
			    err);
	}
      }
      nm_ns_update_pw(receiving_pw, data_recv, size);
      
      while (nb_samples++ < param_nb_samples - 1) 
	{
	  err = drv_ops->post_send_iov(status, sending_pw);
	  if(err != NM_ESUCCESS && err != -NM_EAGAIN){
	    nm_ns_print_errno("nm_ns_pong - post_send_iov",
			      err);
	    goto out;
	  }
	  while(err != NM_ESUCCESS)
	    {
	      err = drv_ops->poll_send_iov(status, sending_pw);
	      if(err != NM_ESUCCESS && err != -NM_EAGAIN){
		nm_ns_print_errno("nm_ns_pong - poll_send_iov",
				  err);
		goto out;
	      }
	    }
	  nm_ns_update_pw(sending_pw,  data_send, size);
	  
	  err = drv_ops->post_recv_iov(status, receiving_pw);
	  if(err != NM_ESUCCESS && err != -NM_EAGAIN){
	    nm_ns_print_errno("nm_ns_pong - post_recv_iov",
			      err);
	    goto out;
	  }
	  while(err != NM_ESUCCESS)
	    {
	      err = drv_ops->poll_recv_iov(status, receiving_pw);
	      if(err != NM_ESUCCESS && err != -NM_EAGAIN){
		nm_ns_print_errno("nm_ns_pong - poll_recv_iov",
				  err);
		goto out;
	      }
	    }
	  nm_ns_update_pw(receiving_pw, data_recv, size);
	}

      err = drv_ops->post_send_iov(status, sending_pw);
      if(err != NM_ESUCCESS && err != -NM_EAGAIN){
	nm_ns_print_errno("nm_ns_pong - post_send_iov",
			  err);
	goto out;
      }
      while(err != NM_ESUCCESS)
	{
	  err = drv_ops->poll_send_iov(status, sending_pw);
	  if(err != NM_ESUCCESS && err != -NM_EAGAIN){
	    nm_ns_print_errno("nm_ns_pong - poll_send_iov",
			      err);
	    goto out;
	  }
	}
      nm_ns_update_pw(sending_pw,  data_send, size);
      
      nb_samples = 0;
      size = (size==0? 1:size);
    }
  
  nm_ns_free(driver->p_core, sending_pw);
  nm_ns_free(driver->p_core, receiving_pw);
  TBX_FREE(data_send);
  TBX_FREE(data_recv);

  err = NM_ESUCCESS;

 out:

  return err;
}

int main(int argc, char **argv)
{
  p_mad_madeleine_t       madeleine = NULL;
  p_mad_session_t         session   = NULL;
  int                     is_server = -1;

  struct nm_core          *p_core     = NULL;
  struct nm_drv           *p_drv = NULL;
  struct nm_gate          *p_gate = NULL;
  struct nm_gate_drv      *p_gdrv = NULL;

  /* Initialisation de l'émulation */
  madeleine    = mad_init(&argc, argv);
  session      = madeleine->session;
  is_server    = session->process_rank;

  /* Initialisation de Nmad */
  p_core = mad_nmad_get_core();

  p_drv  = &p_core->driver_array[0];
  p_gate = &p_core->gate_array[0];
  p_gdrv = p_gate->p_gate_drv_array[0];

  if(!is_server)
    {
      const char*filename = argv[1];
      fprintf(stderr, "# sampling:  writing file %s\n", filename);
      FILE*sampling_file = fopen(filename, "w");
      if(sampling_file == NULL)
	{
	  fprintf(stderr, "# ## sampling: cannot write file %s.\n", filename);
	  abort();
	}
      nm_ns_ping(p_drv, p_gate, p_gdrv->receptacle._status, sampling_file);
      fclose(sampling_file);
    }
  else
    {
      nm_ns_pong(p_drv, p_gate, p_gdrv->receptacle._status);
    }

  mad_exit(madeleine);
  return 0;
}

