/* #define DEBUG */
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#include "pm2.h"
#include "madeleine.h"
#include "sys/netserver.h"
#include "isomalloc.h"
#include "pm2_timing.h"

#define MAX_NETSERVERS   128

static marcel_t _recv_pid[MAX_NETSERVERS];
static unsigned nb_netservers = 0;

static volatile boolean finished = FALSE;

extern pm2_rawrpc_func_t pm2_rawrpc_func[];

static void netserver_raw_rpc(int num)
{
  marcel_sem_t sem;

  marcel_sem_init(&sem, 0);
  marcel_setspecific(_pm2_mad_key, &sem);

  (*pm2_rawrpc_func[num])();

  marcel_sem_P(&sem);
}

void _netserver_term_func(void *arg)
{
#ifdef DEBUG
  tfprintf(stderr, "netserver is ending.\n");
#endif

  slot_free(NULL, marcel_stackbase((marcel_t)arg));
}

#ifdef MAD2
void pm2_send_stop_server(int i);
extern volatile int pm2_zero_halt;
#endif

static any_t netserver(any_t arg)
{
  unsigned tag;

  marcel_cleanup_push(_netserver_term_func, marcel_self());

  while(!finished) {
#ifdef MAD2
    mad_receive((p_mad_channel_t)arg);
#else
    mad_receive();
#endif
    pm2_unpack_int(SEND_SAFER, RECV_EXPRESS, &tag, 1);
    if(tag >= NETSERVER_RAW_RPC)
      netserver_raw_rpc(tag - NETSERVER_RAW_RPC);
    else {
      switch(tag) {
      case NETSERVER_END : {
#ifdef MAD2
	mad_recvbuf_receive();

	if (__pm2_self == 0) {
	  if(!pm2_zero_halt) {
	    pm2_zero_halt=TRUE;
	    pm2_send_stop_server(1);
	  } else {
	    finished = TRUE;
	  }
	} else {
	  /* On fait suivre et on termine... */
	  pm2_send_stop_server((__pm2_self+1) % __pm2_conf_size);
	  finished = TRUE;
	}
#else
	finished = TRUE;
#endif
	break;
      }
      default : {
	fprintf(stderr, "NETSERVER ERROR: %d is not a valid tag.\n", tag);
      }
      }
    }
  }

  return NULL;
}

#ifdef MAD2
void netserver_start(p_mad_channel_t channel)
#else
void netserver_start(void)
#endif
{
  marcel_attr_t attr;
  unsigned granted;
#ifndef MAD2
  void *channel = NULL;
#endif

  marcel_attr_init(&attr);

#ifdef ONE_VP_PER_NET_THREAD
  {
    unsigned vp = marcel_sched_add_vp();
    extern marcel_vpmask_t __pm2_global_vpmask; // declared in netserver.c

    marcel_vpmask_add_vp(&__pm2_global_vpmask, vp);

    marcel_change_vpmask(__pm2_global_vpmask);

    marcel_attr_setvpmask(&attr, MARCEL_VPMASK_ALL_BUT_VP(vp));

    mdebug("Extra vp (%d) allocated for netserver thread\n", vp);
  }
#endif

#ifdef REALTIME_NET_THREADS
  marcel_attr_setrealtime(&attr, MARCEL_CLASS_REALTIME);
#endif

  marcel_attr_setstackaddr(&attr,
			   slot_general_alloc(NULL, 0, &granted, NULL, NULL));
  marcel_attr_setstacksize(&attr, granted);

  marcel_create(&_recv_pid[nb_netservers], &attr,
		netserver, (any_t)channel);
  nb_netservers++;
}

void netserver_wait_end(void)
{
  int i;

  LOG_IN();

  for(i=0; i<nb_netservers; i++) {
    marcel_join(_recv_pid[i], NULL);
  }

  LOG_OUT();
}

void netserver_stop(void)
{
#ifndef MAD2
  int i;

  for(i=0; i<nb_netservers; i++) {
    if(_recv_pid[i] == marcel_self())
      finished = TRUE;
    else {
      mdebug("netserveur killing %p\n", _recv_pid[i]);
      marcel_cancel(_recv_pid[i]);
    }
  }
#endif
}
