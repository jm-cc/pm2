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

/* Here, it's possible to define USE_DYN_ARRAYS */
#define USE_DYN_ARRAYS

#define MAX_NETSERVERS   128

static marcel_t _recv_pid[MAX_NETSERVERS];
static unsigned nb_netservers = 0;

static boolean finished = FALSE;

#define SYSTEM_SERVICE	-1L

extern marcel_key_t _pm2_lrpc_mode_key, _pm2_lrpc_num_key, _pm2_cloning_key;
extern marcel_sem_t _pm2_critical[];

extern rpc_func_t _pm2_rpc_funcs[];
extern int _pm2_req_sizes[];
extern int _pm2_res_sizes[];
extern marcel_attr_t _pm2_lrpc_attr[];

extern pm2_post_migration_hook _pm2_post_migr_func;
extern pm2_post_post_migration_hook _pm2_post_post_migr_func;

extern volatile unsigned _pm2_imported_threads;

extern marcel_key_t mad2_send_key, mad2_recv_key;

static void netserver_printf()
{ 
  static char buf[2048] __MAD_ALIGNED__;
  unsigned len;

  mad_unpack_int(MAD_IN_HEADER, &len, 1);
  mad_unpack_byte(MAD_IN_HEADER, buf, len);

  mad_recvbuf_receive();

  tfprintf(stderr, "%s", buf);
}

#ifdef MIGRATE_IN_HEADER
#define MIGR_MODE  MAD_IN_HEADER
#else
#define MIGR_MODE  MAD_IN_PLACE
#endif

#ifdef MAD2
#define MAD2_TRANSMIT_RECV_CONNECTION(pid) \
  *(marcel_specificdatalocation((pid), mad2_recv_key)) = marcel_getspecific(mad2_recv_key)
#else
#define MAD2_TRANSMIT_RECV_CONNECTION(pid) (void)0
#endif


static void netserver_migration()
{
  int nb, i;
  pm2_migr_ctl mctl;

  mad_unpack_int(MAD_IN_HEADER, &nb, 1);

  {
#ifdef USE_DYN_ARRAYS
    marcel_t pids[nb];
    any_t keys[nb];
#else
    marcel_t *pids = tmalloc(nb*sizeof(marcel_t));
    any_t *keys = tmalloc(nb*sizeof(any_t));
#endif

    for(i=0; i<nb; i++) {

      mad_unpack_byte(MAD_IN_HEADER, (char *)&mctl, sizeof(mctl));
      pids[i] = mctl.task;

      slot_general_alloc(NULL, 0, NULL, mctl.stack_base);

      mad_unpack_byte(MIGR_MODE,
		      (char *)mctl.stack_base + mctl.depl,
		      mctl.blk);

      block_unpack_all();

      if(_pm2_post_migr_func != NULL)
	keys[i] = (*_pm2_post_migr_func)((marcel_t)mctl.task);
    }

    mad_recvbuf_receive();

    for(i=0; i<nb; i++)
      marcel_end_hibernation(pids[i],
			     (post_migration_func_t)_pm2_post_post_migr_func,
			     keys[i]);

#ifndef USE_DYN_ARRAYS
    tfree(pids);
    tfree(keys);
#endif
  }
  _pm2_imported_threads += nb;
}

static void netserver_cloning()
{
  marcel_t pid;
  int rank;
  unsigned long depl, taille;
  void **old, **stack_base;
  any_t key = NULL;

  mad_unpack_int(MAD_IN_HEADER, &rank, 1);
  mad_unpack_byte(MAD_IN_HEADER, (char *)&old, sizeof(void *));

  stack_base = slot_general_alloc(NULL, 0, NULL, old);

  mad_unpack_byte(MAD_IN_HEADER, (char *)&pid, sizeof(marcel_t));

  mad_unpack_long(MAD_IN_HEADER, &depl, 1);
  mad_unpack_long(MAD_IN_HEADER, &taille, 1);

  mad_unpack_byte(MIGR_MODE, (char *)stack_base + depl, taille);

  block_unpack_all();

  if(_pm2_post_migr_func != NULL)
    key = (*_pm2_post_migr_func)((marcel_t)pid);

  mad_recvbuf_receive();

  *((int *)(marcel_specificdatalocation((marcel_t)pid,
					_pm2_cloning_key))) = rank;
  marcel_end_hibernation(pid,
			 (post_migration_func_t)_pm2_post_post_migr_func,
			 key);
  _pm2_imported_threads++;
}

static void netserver_lrpc()
{
  int num, tid, priority, sched_policy;
  rpc_args *args;
  marcel_attr_t attr;
  marcel_t pid;
#ifndef MINIMAL_PREEMPTION
  marcel_sem_t sem;
#endif
  unsigned granted;

  mad_unpack_int(MAD_IN_HEADER, &tid, 1);
  mad_unpack_int(MAD_IN_HEADER, &num, 1);
  mad_unpack_int(MAD_IN_HEADER, &priority, 1);
  mad_unpack_int(MAD_IN_HEADER, &sched_policy, 1);

  attr = _pm2_lrpc_attr[num];
  marcel_attr_setstackaddr(&attr,
			   slot_general_alloc(NULL, 0, &granted, NULL));
  marcel_attr_setstacksize(&attr, granted);
  marcel_attr_setprio(&attr, priority);
  marcel_attr_setschedpolicy(&attr, sched_policy);

  marcel_create(&pid, &attr, (marcel_func_t)_pm2_rpc_funcs[num], NULL);

  marcel_getuserspace(pid, (void **)&args);
  mad_unpack_pointer(MAD_IN_HEADER, &args->ptr_att, 1);

  args->num = num;
  args->tid = tid;
  args->quick = FALSE;
  args->sync = TRUE;

#ifndef MINIMAL_PREEMPTION
  args->sem = &sem;
  marcel_sem_init(&sem, 0);
#else
  args->initiator = marcel_self();
#endif

  MAD2_TRANSMIT_RECV_CONNECTION(pid);

  marcel_run(pid, args);

#ifdef MINIMAL_PREEMPTION
  marcel_explicityield(pid);
#else
  marcel_sem_P(&sem);
#endif
}

static void netserver_lrpc_done()
{
  pm2_rpc_wait_t *ptr_att;
  pointer p;

  mad_unpack_pointer(MAD_IN_HEADER, &p, 1);
  ptr_att = (pm2_rpc_wait_t *)to_any_t(&p);
  (*ptr_att->unpack)(ptr_att->result);
  mad_recvbuf_receive();
  marcel_sem_V(&ptr_att->sem);
}

static void netserver_async_lrpc()
{
  int num, tid, priority, sched_policy;
  rpc_args *args;
  marcel_attr_t attr;
  marcel_t pid;
#ifndef MINIMAL_PREEMPTION
  marcel_sem_t sem;
#endif
  unsigned granted;

  mad_unpack_int(MAD_IN_HEADER, &tid, 1);
  mad_unpack_int(MAD_IN_HEADER, &num, 1);
  mad_unpack_int(MAD_IN_HEADER, &priority, 1);
  mad_unpack_int(MAD_IN_HEADER, &sched_policy, 1);

  attr = _pm2_lrpc_attr[num];
  marcel_attr_setstackaddr(&attr,
			   slot_general_alloc(NULL, 0, &granted, NULL));
  marcel_attr_setstacksize(&attr, granted);
  marcel_attr_setprio(&attr, priority);
  marcel_attr_setschedpolicy(&attr, sched_policy);

  marcel_create(&pid, &attr, (marcel_func_t)_pm2_rpc_funcs[num], NULL);

  marcel_getuserspace(pid, (void **)&args);
  args->num = num;
  args->tid = tid;
  args->quick = FALSE;
  args->sync = FALSE;

#ifndef MINIMAL_PREEMPTION
  args->sem = &sem;
  marcel_sem_init(&sem, 0);
#else
  args->initiator = marcel_self();
#endif

  MAD2_TRANSMIT_RECV_CONNECTION(pid);

  marcel_run(pid, args);

#ifdef MINIMAL_PREEMPTION
  marcel_explicityield(pid);
#else
  marcel_sem_P(&sem);
#endif
}

static void netserver_raw_rpc()
{
  int num;

  mad_unpack_int(MAD_IN_HEADER, &num, 1);

  (*_pm2_rpc_funcs[num])(NULL);
}

static void netserver_quick_lrpc()
{
  rpc_args args;
  marcel_sem_t sem;

  mad_unpack_int(MAD_IN_HEADER, &args.tid, 1);
  mad_unpack_int(MAD_IN_HEADER, &args.num, 1);
  mad_unpack_pointer(MAD_IN_HEADER, &args.ptr_att, 1);

  args.sem = &sem;
  args.sync = TRUE;
  args.quick = TRUE;
  marcel_sem_init(&sem, 0);

  (*_pm2_rpc_funcs[args.num])(&args);
}

static void netserver_quick_async_lrpc()
{
  rpc_args args;
  marcel_sem_t sem;

  mad_unpack_int(MAD_IN_HEADER, &args.tid, 1);
  mad_unpack_int(MAD_IN_HEADER, &args.num, 1);

  args.sem = &sem;
  args.sync = FALSE;
  args.quick = TRUE;
  marcel_sem_init(&sem, 0);

  (*_pm2_rpc_funcs[args.num])(&args);
}

static void netserver_merge()
{
  unsigned size;
  marcel_sem_t *sem;
  char *dest;

  mad_unpack_byte(MAD_IN_HEADER, (char *)&sem, sizeof(marcel_sem_t *));
  mad_unpack_byte(MAD_IN_HEADER, (char *)&dest, sizeof(char *));
  mad_unpack_int(MAD_IN_HEADER, &size, 1);
  mad_unpack_byte(MAD_IN_HEADER, dest, size);

  mad_recvbuf_receive();

  marcel_sem_V(sem);
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

  marcel_setspecific(_pm2_lrpc_mode_key, (any_t)1);
  marcel_setspecific(_pm2_lrpc_num_key, (any_t)((long)SYSTEM_SERVICE));

  marcel_cleanup_push(_netserver_term_func, marcel_self());

  while(!finished) {
#ifdef MAD2
    mad_receive((p_mad_channel_t)arg);
#else
    mad_receive();
#endif
    mad_unpack_int(MAD_IN_HEADER, &tag, 1);
    switch(tag) {
    case NETSERVER_RAW_RPC : {
      netserver_raw_rpc();
      break;
    }
    case NETSERVER_LRPC : {
      netserver_lrpc();
      break;
    }
    case NETSERVER_LRPC_DONE : {
      netserver_lrpc_done();
      break;
    }
    case NETSERVER_ASYNC_LRPC : {
      netserver_async_lrpc();
      break;
    }
    case NETSERVER_QUICK_LRPC : {
      netserver_quick_lrpc();
      break;
    }
    case NETSERVER_QUICK_ASYNC_LRPC : {
      netserver_quick_async_lrpc();
      break;
    }
    case NETSERVER_MIGRATION : {
      netserver_migration();
      break;
    }
    case NETSERVER_END : {
      finished = TRUE;
      break;
    }
    case NETSERVER_CRITICAL_SECTION : {
      marcel_sem_V(&_pm2_critical[0]);
      marcel_sem_P(&_pm2_critical[1]);
      break;
    }
    case NETSERVER_CLONING : {
      netserver_cloning();
      break;
    }
    case NETSERVER_PRINTF : {
      netserver_printf();
      break;
    }
    case NETSERVER_MERGE : {
      netserver_merge();
      break;
    }
    default : {
      fprintf(stderr, "NETSERVER ERROR: %d is not a valid tag.\n", tag);
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

  for(i=0; i<nb_netservers; i++)
    marcel_cancel(_recv_pid[i]);
}
