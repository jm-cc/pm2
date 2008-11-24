
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
#include "pm2_sync.h"

//#define BARRIER_TRACE

//#define TRACE_SYNC

#ifdef TRACE_SYNC
#define ENTER() fprintf(stderr, "[%p] >>> %s\n", marcel_self(), __FUNCTION__)
#define EXIT() fprintf(stderr, "[%p] <<< %s\n", marcel_self(), __FUNCTION__)
#else
#define ENTER()
#define EXIT()
#endif

static int BARRIER_LRPC, _local_node_rank, _nb_nodes;

#define barrier_lock(bar) marcel_mutex_lock(&bar->sync_mutex)
#define barrier_unlock(bar) marcel_mutex_unlock(&bar->sync_mutex)


static void BARRIER_LRPC_threaded_func(void)
{
  int sender;
  pm2_barrier_t *bar;
#ifdef BARRIER_TRACE
  int i;
#endif
  ENTER();

  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&bar, sizeof(bar));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&sender, sizeof(int));
  pm2_rawrpc_waitdata();
#ifdef BARRIER_TRACE
  fprintf(stderr,"[%s] on module %d...\n", __FUNCTION__, _local_node_rank);
#endif
  barrier_lock(bar); /*Useless: normally there are no concurrent writes...*/
  bar->tab_sync[sender]++;
#ifdef BARRIER_TRACE
  fprintf(stderr,"[%s] t[%d] = %d\n", __FUNCTION__, sender, bar->tab_sync[sender]);
  for (i=0 ; i < _nb_nodes ; i++)
    fprintf(stderr,"[%s] --->t[%d] = %d\n", __FUNCTION__, i, bar->tab_sync[i]);
#endif
  barrier_unlock(bar);

  EXIT();
}

static void BARRIER_LRPC_func(void)
{
  pm2_thread_create((pm2_func_t)BARRIER_LRPC_threaded_func, NULL);
}

static int _received_from_all_other_nodes (pm2_barrier_t *bar)
{
  int i = 0;

  barrier_lock(bar);
  while(( i < _nb_nodes) && (bar->tab_sync[i] >= 1)) 
      i++;

#ifdef BARRIER_TRACE
  if (i >= _nb_nodes)
    fprintf(stderr,"Received all acks for sync\n");
  /*  else
    {
      int j;

      for (j=0 ; j < _nb_nodes ; j++)
	fprintf(stderr,"Sync status: t[%d] = %d\n", j, bar->tab_sync[j]);
    }
    */
#endif
     barrier_unlock(bar);
  return (i == _nb_nodes);
}

/* The following primitive needs to be called by PM2 in a startup function,
to allow for proper barrier initialization. */
void pm2_barrier_init(pm2_barrier_t *bar)
{
  int i, size = pm2_config_size();
  
  bar->tab_sync = (int *)tmalloc(size * sizeof(int));
  marcel_mutex_init(&bar->sync_mutex, NULL);
  for (i = 0; i < size; i++)
    bar->tab_sync[i] = 0;
}


/* Node-level barrier */
void pm2_barrier(pm2_barrier_t *bar)
{
 int i;
 ENTER();

 barrier_lock(bar);
 bar->tab_sync[_local_node_rank]++;
 barrier_unlock(bar);

 /* 
    send messages to all nodes
 */
 for (i=0 ; i < _nb_nodes ; i++)
   if (i!=_local_node_rank)
     {
       pm2_rawrpc_begin(i, BARRIER_LRPC, NULL);
       pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&bar, sizeof(pm2_barrier_t *));
       pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&_local_node_rank, sizeof(int));
       pm2_rawrpc_end();
     }
#ifdef BARRIER_TRACE
       fprintf(stderr, "[%s] Sent all sync msg. Local node = %d\n", __FUNCTION__, _local_node_rank);
#endif

 /* 
    wait for messages from all nodes
 */
#ifdef BARRIER_TRACE
 fprintf(stderr,"[%s]Waiting msg...(I am %p)\n", __FUNCTION__, marcel_self());
#endif
 while (!_received_from_all_other_nodes(bar))
   marcel_yield();
#ifdef BARRIER_TRACE
 for (i=0 ; i < _nb_nodes ; i++)
   fprintf(stderr,"Sync ok: t[%d] = %d\n", i, bar->tab_sync[i]);
#endif

 barrier_lock(bar);
 for (i=0 ; i < _nb_nodes ; i++)
   bar->tab_sync[i]--;
#ifdef BARRIER_TRACE
 for (i=0 ; i < _nb_nodes ; i++)
   fprintf(stderr,"Sync ended: t[%d] = %d\n", i, bar->tab_sync[i]);
#endif
 barrier_unlock(bar);
 
 EXIT();
}


void pm2_barrier_init_rpc()
{
  pm2_rawrpc_register(&BARRIER_LRPC, BARRIER_LRPC_func);
}


void pm2_sync_init(int myself, int confsize)
{
  _nb_nodes = confsize;
  _local_node_rank = myself;
}

/*************************** thread barrier *************************************/

/* The following primitive needs to be called by PM2 in a startup function,
to allow for proper barrier initialization. */
void pm2_thread_barrier_init(pm2_thread_barrier_t *bar, pm2_thread_barrier_attr_t *attr)
{
  pm2_barrier_init(&bar->node_barrier);
  marcel_mutex_init(&bar->mutex, NULL);
  marcel_cond_init(&bar->cond, NULL);
  bar->nb = 0;
  if (attr)
    {
      int i;
      bar->nb_prot = attr->nb_prot;
      bar->local = attr->local;     
      for(i = 0; i < attr->nb_prot; i++)
	(bar->prot)[i] = (attr->prot)[i];
    }
  else
    bar->nb_prot =0;
}

/*Thread-level barrier */
void pm2_thread_barrier(pm2_thread_barrier_t *bar)
{
  ENTER();

  marcel_mutex_lock(&bar->mutex);
  if(++bar->nb == bar->local) {
    pm2_barrier(&bar->node_barrier);
    marcel_cond_broadcast(&bar->cond);
    bar->nb = 0;
  } else {
    marcel_cond_wait(&bar->cond, &bar->mutex);
  }
  marcel_mutex_unlock(&bar->mutex);

  EXIT();
}
