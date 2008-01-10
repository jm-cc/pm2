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
#include <sys/uio.h>
#include <assert.h>

#include <pm2_common.h>

#include <nm_public.h>
#include <nm_so_public.h>
#include "nm_so_pkt_wrap.h"
#include "nm_core_inline.h"

#ifdef CONFIG_PROTO_MAD3
#include <madeleine.h>

static const int param_min_size = 2;
static const int param_max_size = (8*1024*1024);
static const int param_nb_samples = 1000;

static void
nm_ns_print_errno(char *msg,
                  int err){
  err = (err > 0 ? err : -err);

//  switch(err){
//  case NM_ESUCCESS :
//    DISP("%s - Successful operation", msg);
//    break;
//
//  case NM_EUNKNOWN :
//    DISP("%s - Unknown error", msg);
//    break;
//
//  case NM_ENOTIMPL :
//    DISP("%s - Not implemented", msg);
//    break;
//
//  case NM_ESCFAILD :
//    DISP("%s - Syscall failed, see errno", msg);
//    break;
//
//  case NM_EAGAIN :
//    DISP("%s - Poll again", msg);
//    break;
//
//  case NM_ECLOSED :
//    DISP("%s - Connection closed", msg);
//    break;
//
//  case NM_EBROKEN :
//    DISP("%s - Error condition on connection", msg);
//    break;
//
//  case NM_EINVAL :
//    DISP("%s - Invalid parameter", msg);
//    break;
//
//  case NM_ENOTFOUND :
//    DISP("%s - Not found", msg);
//    break;
//
//  case NM_ENOMEM :
//    DISP("%s - Out of memory", msg);
//    break;
//
//  case NM_EALREADY :
//    DISP("%s - Already in progress or done", msg);
//    break;
//
//  case NM_ETIMEDOUT :
//    DISP("%s - Operation timeout", msg);
//    break;
//
//  case NM_EINPROGRESS :
//    DISP("%s - Operation in progress", msg);
//    break;
//
//  case NM_EUNREACH :
//    DISP("%s - Destination unreachable", msg);
//    break;
//
//  case NM_ECANCELED :
//    DISP("%s - Operation canceled", msg);
//    break;
//
//  case NM_EABORTED :
//    DISP("%s - Operation aborted", msg);
//    break;
//
//  default:
//    TBX_FAILURE("Sortie d'erreur incorrecte");
//  }
}

static int
nm_ns_initialize_pw(struct nm_core *p_core,
                    struct nm_drv  *p_drv,
                    struct nm_gate *p_gate,
                    unsigned char *ptr,
                    struct nm_pkt_wrap **pp_pw){
  struct nm_pkt_wrap *p_pw = NULL;
  int err;

  err = nm_pkt_wrap_alloc(p_core, &p_pw, 0, 0);
  if(err != NM_ESUCCESS){
    nm_ns_print_errno("nm_ns_initialize_pw - nm_pkt_wrap_alloc",
                      err);
    goto out;
  }


  err = nm_iov_alloc(p_core, p_pw, 1);
  if (err != NM_ESUCCESS) {
    nm_ns_print_errno("nm_ns_initialize_pw - nm_iov_alloc",
                      err);
    nm_pkt_wrap_free(p_core, p_pw);
    goto out;
  }

  err = nm_iov_append_buf(p_core, p_pw, ptr, param_min_size);
  if (err != NM_ESUCCESS) {
    nm_ns_print_errno("nm_ns_initialize_pw - nm_iov_append_buf",
                      err);
    nm_iov_free(p_core, p_pw);
    nm_pkt_wrap_free(p_core, p_pw);
    goto out;
  }

  p_pw->p_gate = p_gate;
  p_pw->p_drv  = p_drv;
  p_pw->p_trk  = p_drv->p_track_array[0];

  *pp_pw	= p_pw;

 out:
  return err;
}

static void
nm_ns_fill_pw_data(struct nm_pkt_wrap *p_pw){
  unsigned char * data = p_pw->v[0].iov_base;
  unsigned char c = 0;
  int i = 0, n = 0;

  for (i = 0; i < param_max_size; i++){
    n += 7;
    c = (unsigned char)(n % 256);
    data[i] = c;
  }
}

static void
nm_ns_update_pw(struct nm_pkt_wrap *p_pw, unsigned char *ptr, int len){
  p_pw->v[0].iov_base	= ptr;
  p_pw->v[0].iov_len	= len;

  p_pw->drv_priv   = NULL;
  p_pw->gate_priv  = NULL;
}

static void
nm_ns_free(struct nm_core *p_core, struct nm_pkt_wrap *p_pw){

  /* free iovec */
  nm_iov_free(p_core, p_pw);

  /* free pkt_wrap */
  nm_pkt_wrap_free(p_core, p_pw);
}


#if 0
static void
control_buffer(struct nm_pkt_wrap *p_pw, int len) {
  unsigned char *main_buffer = p_pw->v[0].iov_base;
  int ok = 1;
  unsigned int n  = 0;
  int i  = 0;

  for (i = 0; i < len; i++) {
    unsigned char c = 0;

    n += 7;
    c = (unsigned char)(n % 256);

    if (main_buffer[i] != c) {
      int v1 = 0;
      int v2 = 0;

      v1 = c;
      v2 = main_buffer[i];
      printf("Bad data at byte %X: expected %X, received %X\n", i, v1, v2);
      ok = 0;
    }
  }

  if (!ok) {
    printf("%d bytes reception failed", len);
  } else {
    printf("ok");
  }
}
#endif

static int
nm_ns_ping(struct nm_drv *driver, struct nm_gate *p_gate, void *status){
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
  err = nm_ns_initialize_pw(driver->p_core, driver, p_gate, data_recv,
                            &receiving_pw);
  if(err != NM_ESUCCESS){
    nm_ns_print_errno("nm_ns_ping - nm_ns_initialize_receiving_pw",
                      err);
    goto out;
  }


  //printf("ping-PWs initialisés\n");

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
      //control_buffer(receiving_pw, size);

    }
    TBX_GET_TICK(t2);

    sum = TBX_TIMING_DELAY(t1, t2);
    lat	            = sum / (2 * param_nb_samples);
    bw_million_byte = size * (param_nb_samples / (sum / 2));
    bw_mbyte        = bw_million_byte / 1.048576;

    printf("%d\t%lf\n", size, bw_mbyte);

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

static int
nm_ns_pong(struct nm_drv *driver, struct nm_gate *p_gate, void *status){
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
  err = nm_ns_initialize_pw(driver->p_core, driver, p_gate, data_send,
                            &sending_pw);
  if(err != NM_ESUCCESS){
    nm_ns_print_errno("nm_ns_pong - nm_ns_initialize_sending_pw",
                      err);
    goto out;
  }

  nm_ns_fill_pw_data(sending_pw);

  data_recv = TBX_MALLOC(param_max_size);
  err = nm_ns_initialize_pw(driver->p_core, driver, p_gate, data_recv,
                            &receiving_pw);
  if(err != NM_ESUCCESS){
    nm_ns_print_errno("nm_ns_pong - nm_ns_initialize_receiving_pw",
                      err);
    goto out;
  }


  //printf("pong-PWs initialisés\n");

  for (i = 0, size = param_min_size; size <= param_max_size; i++, size*= 2) {

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
    //control_buffer(receiving_pw, size);

    while (nb_samples++ < param_nb_samples - 1) {
      err = drv_ops->post_send_iov(status, sending_pw);
      if(err != NM_ESUCCESS && err != -NM_EAGAIN){
        nm_ns_print_errno("nm_ns_pong - post_send_iov",
                          err);
        goto out;
      }
      while(err != NM_ESUCCESS){
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
      while(err != NM_ESUCCESS){
        err = drv_ops->poll_recv_iov(status, receiving_pw);
        if(err != NM_ESUCCESS && err != -NM_EAGAIN){
          nm_ns_print_errno("nm_ns_pong - poll_recv_iov",
                            err);
          goto out;
        }
      }
      nm_ns_update_pw(receiving_pw, data_recv, size);
      //control_buffer(receiving_pw, size);
    }

    err = drv_ops->post_send_iov(status, sending_pw);
    if(err != NM_ESUCCESS && err != -NM_EAGAIN){
      nm_ns_print_errno("nm_ns_pong - post_send_iov",
                        err);
      goto out;
    }
    while(err != NM_ESUCCESS){
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

int main(int argc, char **argv){
  p_mad_madeleine_t       madeleine      = NULL;
  p_mad_session_t         session          = NULL;
  int                     is_server	= -1;

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

  p_drv  = &p_core->driver_array[0]; // always one activated network
  p_gate = &p_core->gate_array[0];
  p_gdrv = p_gate->p_gate_drv_array[0];

  if(!is_server){
    //printf("Je suis le client\n");
    nm_ns_ping(p_drv, p_gate, p_gdrv->receptacle._status);

  } else {
    //printf("Je suis le serveur\n");
    nm_ns_pong(p_drv, p_gate, p_gdrv->receptacle._status);
  }

  mad_exit(madeleine);
  return 0;
}

#else
int main(int argc, char **argv) {
  fprintf(stderr, "Require mad3_emu\n");
  exit(EXIT_FAILURE);
}
#endif
