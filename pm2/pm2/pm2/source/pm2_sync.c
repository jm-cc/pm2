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

_____________________________________________________________________________
*/

#include "pm2.h"
#include "pm2_sync.h"

#//define BARRIER_TRACE

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
}

static void BARRIER_LRPC_func(void)
{
  pm2_thread_create((pm2_func_t)BARRIER_LRPC_threaded_func, NULL);
}

static int _received_from_all_other_nodes (pm2_barrier_t *bar)
{
  int i = 0;
  
  while(( i < _nb_nodes) && (bar->tab_sync[i] >= 1)) i++;
#ifdef BARRIER_TRACE
  if (i >= _nb_nodes)
    fprintf(stderr,"Received all acks for sync\n");
#endif
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
#ifdef BARRIER_TRACE
       fprintf(stderr, "[%s] Sent all sync msg. Local node = %d\n", __FUNCTION__, _local_node_rank);
#endif
     }
 /* 
    wait for messages from all nodes
 */
#ifdef BARRIER_TRACE
 fprintf(stderr,"[%s]Waiting msg...(I am %p)\n", __FUNCTION__, marcel_self());
#endif
 while (!_received_from_all_other_nodes(bar))
   marcel_trueyield();
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
  marcel_sem_init(&bar->mutex, 1);
  marcel_sem_init(&bar->wait, 0);
  bar->local = attr->local;
  bar->nb = 0;
}

/*Thread-level barrier */
void pm2_thread_barrier(pm2_thread_barrier_t *bar)
{
  marcel_sem_P(&bar->mutex);
  if(++bar->nb == bar->local) {
    pm2_barrier(&bar->node_barrier);
    marcel_sem_unlock_all(&bar->wait);
    bar->nb = 0;
    marcel_sem_V(&bar->mutex);
  } else {
    marcel_sem_VP(&bar->mutex, &bar->wait);
  }
}
