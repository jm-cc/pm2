/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#include <pm2.h>
#include <madeleine.h>
#include <sys/netserver.h>
#include <isomalloc.h>

#include <pm2_timing.h>

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
	finished = TRUE;
	break;
      }
      default : {
	fprintf(stderr, "NETSERVER ERROR: %d is not a valid tag.\n", tag);
      }
      }
    }
  }
#ifdef MINIMAL_PREEMPTION
   marcel_setspecialthread(NULL);
#endif

  return NULL;
}

#ifdef MAD2
void netserver_start(p_mad_channel_t channel, unsigned priority)
#else
void netserver_start(unsigned priority)
#endif
{
  marcel_attr_t attr;
  unsigned granted;
#ifndef MAD2
  void *channel = NULL;
#endif

  marcel_attr_init(&attr);
  marcel_attr_setprio(&attr, priority);
  marcel_attr_setstackaddr(&attr,
			   slot_general_alloc(NULL, 0, &granted, NULL));
  marcel_attr_setstacksize(&attr, granted);

  marcel_create(&_recv_pid[nb_netservers], &attr,
		netserver, (any_t)channel);
  nb_netservers++;

#ifdef MINIMAL_PREEMPTION
  marcel_setspecialthread(_recv_pid[0]);
#endif
}

void netserver_wait_end(void)
{
  int i;

  for(i=0; i<nb_netservers; i++)
    marcel_join(_recv_pid[i], NULL);
}

void netserver_stop(void)
{
  int i;

  for(i=0; i<nb_netservers; i++) {
    if(_recv_pid[i] == marcel_self())
      finished = TRUE;
    else
      marcel_cancel(_recv_pid[i]);
  }
}
