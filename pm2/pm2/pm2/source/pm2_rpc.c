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
#include <sys/netserver.h>

#define MAX_LRPC_FUNCS	50

#define MAIN_SERVICE	-2L
#define SYSTEM_SERVICE	-1L

static unsigned PM2_LRPC, PM2_LRPC_DONE, PM2_ASYNC_LRPC,
  PM2_QUICK_LRPC, PM2_QUICK_ASYNC_LRPC;

rpc_func_t _pm2_rpc_funcs[MAX_LRPC_FUNCS];
unpack_func_t _pm2_unpack_req_funcs[MAX_LRPC_FUNCS];
unpack_func_t _pm2_unpack_res_funcs[MAX_LRPC_FUNCS];
pack_func_t _pm2_pack_req_funcs[MAX_LRPC_FUNCS];
pack_func_t _pm2_pack_res_funcs[MAX_LRPC_FUNCS];
int _pm2_req_sizes[MAX_LRPC_FUNCS];
int _pm2_res_sizes[MAX_LRPC_FUNCS];
marcel_attr_t _pm2_lrpc_attr[MAX_LRPC_FUNCS];
static char *rpc_names[MAX_LRPC_FUNCS];
int _pm2_optimize[MAX_LRPC_FUNCS];  /* accessed by other modules */


#define MAX_CHANNELS    16

struct pm2_rpc_channel_struct_t {
  pm2_channel_t channel[2];
};

static struct pm2_rpc_channel_struct_t pm2_rpc_channel[MAX_CHANNELS];
static unsigned nb_of_channels = 1;

#define TRANSMIT_NETSERVER_SEM(pid) \
  *(marcel_specificdatalocation((pid), _pm2_mad_key)) = marcel_getspecific(_pm2_mad_key)

/* *** Mad II *** */

#ifdef MAD2
extern marcel_key_t mad2_recv_key;
#define MAD2_TRANSMIT_RECV_CONNECTION(pid) \
  *(marcel_specificdatalocation((pid), mad2_recv_key)) = marcel_getspecific(mad2_recv_key)
#else
#define MAD2_TRANSMIT_RECV_CONNECTION(pid) (void)0
#endif


#ifdef MAD2

static pm2_rpc_channel_t pm2_rpc_main_channel = 0;

typedef enum {
  REQUEST,
  RESULT
} comm_direction_t;

static __inline__ pm2_channel_t channel(pm2_rpc_channel_t c,
					unsigned to,
					comm_direction_t dir)
{
  extern int __pm2_self;

  if(dir == REQUEST) {
    return (to > __pm2_self) ? pm2_rpc_channel[c].channel[0] : pm2_rpc_channel[c].channel[1];
  } else {
    return (to > __pm2_self) ? pm2_rpc_channel[c].channel[1] : pm2_rpc_channel[c].channel[0];
  }
}

#endif

void pm2_rpc_channel_alloc(pm2_rpc_channel_t *channel)
{
  *channel = nb_of_channels++;
}

char *pm2_lrpc_name(int num)
{
  return "NotAvailable";
}

int pm2_lrpc_runned_by(marcel_t pid)
{
   return (int)(long)(*marcel_specificdatalocation(pid, _pm2_lrpc_num_key));
}

void _pm2_term_func(void *arg)
{
  block_descr_t *block_descr_ptr;

#ifdef DEBUG
  fprintf(stderr, "Thread %p seems to have finished\n", arg);
#endif

  block_descr_ptr = (block_descr_t *)(*marcel_specificdatalocation((marcel_t)arg, _pm2_block_key));
  block_flush_list(block_descr_ptr);
  slot_free(NULL, marcel_stackbase((marcel_t)arg));
}

/**** lrpc completion ****/

void _end_service(rpc_args *args, any_t res, int local)
{
  if(local) {
    memcpy(args->true_ptr_att->result, res, _pm2_res_sizes[args->num]);
  } else if(args->tid == __pm2_self && _pm2_optimize[args->num]) {
    pm2_rpc_wait_t *ptr_att;
    ptr_att = (pm2_rpc_wait_t *)to_any_t(&args->ptr_att);
    memcpy(ptr_att->result, res, _pm2_res_sizes[args->num]);
    marcel_sem_V(&ptr_att->sem);
  } else {
#ifdef MAD2
    pm2_attr_t attr;

    pm2_attr_init(&attr);
    /* FIX THIS: use the same "pm2_rpc_channel" as the one of the request!
       Currently, pm2_rpc_main_channel is always used for replies... */
    pm2_attr_setchannel(&attr, channel(pm2_rpc_main_channel, args->tid, RESULT));

    pm2_rawrpc_begin(args->tid, PM2_LRPC_DONE, &attr);
#else
    pm2_rawrpc_begin(args->tid, PM2_LRPC_DONE, NULL);
#endif

    old_mad_pack_pointer(MAD_IN_HEADER, &args->ptr_att, 1);
    (*_pm2_pack_res_funcs[args->num])(res);

    pm2_rawrpc_end();
  }
}

static void netserver_lrpc_done(void)
{
  pm2_rpc_wait_t *ptr_att;
  pointer p;

  old_mad_unpack_pointer(MAD_IN_HEADER, &p, 1);
  ptr_att = (pm2_rpc_wait_t *)to_any_t(&p);
  (*ptr_att->unpack)(ptr_att->result);
  pm2_rawrpc_waitdata();
  marcel_sem_V(&ptr_att->sem);
}

void pm2_register_service(int *num, rpc_func_t func,
			  int req_size,
			  pack_func_t pack_req, unpack_func_t unpack_req,
			  int res_size,
			  pack_func_t pack_res, unpack_func_t unpack_res,
			  char *name, int optimize_if_local)
{
  static unsigned service_number = 0;

  _pm2_rpc_funcs[service_number] = func;
  _pm2_req_sizes[service_number] = req_size;
  _pm2_pack_req_funcs[service_number] = pack_req;
  _pm2_unpack_req_funcs[service_number] = unpack_req;
  _pm2_res_sizes[service_number] = res_size;
  _pm2_pack_res_funcs[service_number] = pack_res;
  _pm2_unpack_res_funcs[service_number] = unpack_res;
  rpc_names[service_number] = name;
  _pm2_optimize[service_number] = optimize_if_local;

  marcel_attr_init(&_pm2_lrpc_attr[service_number]);
  marcel_attr_setdetachstate(&_pm2_lrpc_attr[service_number], TRUE);
  marcel_attr_setuserspace(&_pm2_lrpc_attr[service_number], sizeof(rpc_args));

  *num = service_number++;
}


/**** lrpc call ****/

void pm2_rpc_call(int module, int num, pm2_attr_t *pm2_attr,
		  any_t args, any_t results, pm2_rpc_wait_t *att)
{
  pm2_attr_t attr;

  if(pm2_attr == NULL) {
    pm2_attr_init(&attr);
    pm2_attr = &attr;
  }

  pm2_disable_migration();

  marcel_sem_init(&att->sem, 0);
  att->result = results;
  att->unpack = _pm2_unpack_res_funcs[num];
  if(module == __pm2_self && _pm2_optimize[num]) {
    rpc_args *arg;
    marcel_t pid;
    marcel_attr_t attr;
#ifndef MINIMAL_PREEMPTION
    marcel_sem_t sem; /* pour attendre l'initialisation du processus */
#endif
    unsigned granted;

    attr = _pm2_lrpc_attr[num];
    marcel_attr_setstackaddr(&attr,
			     slot_general_alloc(NULL, 0,
						&granted, NULL));
    marcel_attr_setstacksize(&attr, granted);
    marcel_attr_setprio(&attr, pm2_attr->priority);
    marcel_attr_setschedpolicy(&attr, pm2_attr->sched_policy);

    marcel_create(&pid, &attr, (marcel_func_t)_pm2_rpc_funcs[num], NULL);
    marcel_getuserspace(pid, (void **)&arg);

    arg->num = num;
    arg->tid = __pm2_self;
    arg->req = args;
    arg->quick = FALSE;
    arg->sync = TRUE;

    to_pointer(att, &arg->ptr_att);

#ifndef MINIMAL_PREEMPTION
    arg->sem = &sem;
    marcel_sem_init(&sem, 0);
#else
    arg->initiator = marcel_self();
#endif
    marcel_run(pid, arg);

#ifdef MINIMAL_PREEMPTION
    marcel_explicityield(pid);
#else
    marcel_sem_P(&sem);
#endif

  } else {
    pointer p;

#ifdef DEBUG
    if(module == __pm2_self && !mad_can_send_to_self())
      RAISE(NOT_IMPLEMENTED);
#endif

    to_pointer((any_t)att, &p);

#ifdef MAD2
    {
      unsigned c;

      pm2_attr_getchannel(pm2_attr, &c);
      pm2_attr_setchannel(pm2_attr, channel(c, module, REQUEST));
    }
#endif
    pm2_rawrpc_begin(module, PM2_LRPC, pm2_attr);

    old_mad_pack_int(MAD_IN_HEADER, &__pm2_self, 1);
    old_mad_pack_int(MAD_IN_HEADER, &num, 1);
    old_mad_pack_int(MAD_IN_HEADER, &pm2_attr->priority, 1);
    old_mad_pack_int(MAD_IN_HEADER, &pm2_attr->sched_policy, 1);

    old_mad_pack_pointer(MAD_IN_HEADER, &p, 1);

    (*_pm2_pack_req_funcs[num])(args);

    pm2_rawrpc_end();
  }
}

static void netserver_lrpc(void)
{
  int num, tid, priority, sched_policy;
  rpc_args *args;
  marcel_attr_t attr;
  marcel_t pid;
#ifndef MINIMAL_PREEMPTION
  marcel_sem_t sem;
#endif
  unsigned granted;

  old_mad_unpack_int(MAD_IN_HEADER, &tid, 1);
  old_mad_unpack_int(MAD_IN_HEADER, &num, 1);
  old_mad_unpack_int(MAD_IN_HEADER, &priority, 1);
  old_mad_unpack_int(MAD_IN_HEADER, &sched_policy, 1);

  attr = _pm2_lrpc_attr[num];
  marcel_attr_setstackaddr(&attr,
			   slot_general_alloc(NULL, 0, &granted, NULL));
  marcel_attr_setstacksize(&attr, granted);
  marcel_attr_setprio(&attr, priority);
  marcel_attr_setschedpolicy(&attr, sched_policy);

  marcel_create(&pid, &attr, (marcel_func_t)_pm2_rpc_funcs[num], NULL);

  marcel_getuserspace(pid, (void **)&args);
  old_mad_unpack_pointer(MAD_IN_HEADER, &args->ptr_att, 1);

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

  TRANSMIT_NETSERVER_SEM(pid);
  MAD2_TRANSMIT_RECV_CONNECTION(pid);

  marcel_run(pid, args);

#ifdef MINIMAL_PREEMPTION
  marcel_explicityield(pid);
#else
  marcel_sem_P(&sem);
#endif
}

void pm2_quick_rpc_call(int module, int num, pm2_attr_t *pm2_attr,
			any_t args, any_t results, pm2_rpc_wait_t *att)
{
  pm2_attr_t attr;

  if(pm2_attr == NULL) {
    pm2_attr_init(&attr);
    pm2_attr = &attr;
  }

  pm2_disable_migration();

  marcel_sem_init(&att->sem, 0);
  att->result = results;
  if(module == __pm2_self && _pm2_optimize[num]) {
    rpc_args arg;
    marcel_sem_t sem;

    arg.num = num;
    arg.tid = __pm2_self;
    arg.sync = TRUE;
    arg.quick = TRUE;
    arg.req = args;
    arg.sem = &sem;
    arg.true_ptr_att = att;
    marcel_sem_init(&sem, 0);

    (*_pm2_rpc_funcs[num])(&arg);

    marcel_sem_V(&att->sem);

  } else {
    pointer p;

#ifdef DEBUG
    if(module == __pm2_self && !mad_can_send_to_self())
      RAISE(NOT_IMPLEMENTED);
#endif

    to_pointer((any_t)att, &p);
    att->unpack = _pm2_unpack_res_funcs[num];

#ifdef MAD2
    {
      unsigned c;

      pm2_attr_getchannel(pm2_attr, &c);
      pm2_attr_setchannel(pm2_attr, channel(c, module, REQUEST));
    }
#endif
    pm2_rawrpc_begin(module, PM2_QUICK_LRPC, pm2_attr);

    old_mad_pack_int(MAD_IN_HEADER, &__pm2_self, 1);
    old_mad_pack_int(MAD_IN_HEADER, &num, 1);
    old_mad_pack_pointer(MAD_IN_HEADER, &p, 1);

    (*_pm2_pack_req_funcs[num])(args);

    pm2_rawrpc_end();
  }
}

static void netserver_quick_lrpc(void)
{
  rpc_args args;
  marcel_sem_t sem;

  old_mad_unpack_int(MAD_IN_HEADER, &args.tid, 1);
  old_mad_unpack_int(MAD_IN_HEADER, &args.num, 1);
  old_mad_unpack_pointer(MAD_IN_HEADER, &args.ptr_att, 1);

  args.sem = &sem;
  args.sync = TRUE;
  args.quick = TRUE;
  marcel_sem_init(&sem, 0);

  (*_pm2_rpc_funcs[args.num])(&args);
}

void pm2_rpc_wait(pm2_rpc_wait_t *att)
{
  marcel_sem_P(&att->sem);
  pm2_enable_migration();
}

void pm2_async_rpc(int module, int num, pm2_attr_t *pm2_attr, any_t args)
{
  pm2_attr_t attr;

  if(pm2_attr == NULL) {
    pm2_attr_init(&attr);
    pm2_attr = &attr;
  }

  pm2_disable_migration();

  if(module == __pm2_self && _pm2_optimize[num]) {
    rpc_args *arg;
    marcel_t pid;
    marcel_attr_t attr;
#ifndef MINIMAL_PREEMPTION
    marcel_sem_t sem;
#endif
    unsigned granted;

    attr = _pm2_lrpc_attr[num];
    marcel_attr_setstackaddr(&attr,
			     slot_general_alloc(NULL, 0,
						&granted, NULL));
    marcel_attr_setstacksize(&attr, granted);
    marcel_attr_setprio(&attr, pm2_attr->priority);
    marcel_attr_setschedpolicy(&attr, pm2_attr->sched_policy);

    marcel_create(&pid, &attr, (marcel_func_t)_pm2_rpc_funcs[num], NULL);
    marcel_getuserspace(pid, (void **)&arg);

    arg->num = num;
    arg->tid = __pm2_self;
    arg->req = args;
    arg->sync = FALSE;
    arg->quick = FALSE;

#ifndef MINIMAL_PREEMPTION
    arg->sem = &sem;
    marcel_sem_init(&sem, 0);
#else
    arg->initiator = marcel_self();
#endif

    marcel_run(pid, arg);

#ifdef MINIMAL_PREEMPTION
    marcel_explicityield(pid);
#else
    marcel_sem_P(&sem);
#endif

  } else {

#ifdef DEBUG
    if(module == __pm2_self && !mad_can_send_to_self())
      RAISE(NOT_IMPLEMENTED);
#endif

#ifdef MAD2
    {
      unsigned c;

      pm2_attr_getchannel(pm2_attr, &c);
      pm2_attr_setchannel(pm2_attr, channel(c, module, REQUEST));
    }
#endif
    pm2_rawrpc_begin(module, PM2_ASYNC_LRPC, pm2_attr);

    old_mad_pack_int(MAD_IN_HEADER, &__pm2_self, 1);
    old_mad_pack_int(MAD_IN_HEADER, &num, 1);
    old_mad_pack_int(MAD_IN_HEADER, &pm2_attr->priority, 1);
    old_mad_pack_int(MAD_IN_HEADER, &pm2_attr->sched_policy, 1);

    (*_pm2_pack_req_funcs[num])(args);

    pm2_rawrpc_end();
  }

  pm2_enable_migration();
}

static void netserver_async_lrpc(void)
{
  int num, tid, priority, sched_policy;
  rpc_args *args;
  marcel_attr_t attr;
  marcel_t pid;
#ifndef MINIMAL_PREEMPTION
  marcel_sem_t sem;
#endif
  unsigned granted;

  old_mad_unpack_int(MAD_IN_HEADER, &tid, 1);
  old_mad_unpack_int(MAD_IN_HEADER, &num, 1);
  old_mad_unpack_int(MAD_IN_HEADER, &priority, 1);
  old_mad_unpack_int(MAD_IN_HEADER, &sched_policy, 1);

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

  TRANSMIT_NETSERVER_SEM(pid);
  MAD2_TRANSMIT_RECV_CONNECTION(pid);

  marcel_run(pid, args);

#ifdef MINIMAL_PREEMPTION
  marcel_explicityield(pid);
#else
  marcel_sem_P(&sem);
#endif
}

void pm2_multi_async_rpc(int *modules, int nb, int num, pm2_attr_t *pm2_attr,
			 any_t args)
{
  int i;


  if(nb) {
    pm2_attr_t attr;

    if(pm2_attr == NULL) {
      pm2_attr_init(&attr);
      pm2_attr = &attr;
    }

    pm2_disable_migration();

    for(i=0; i<nb; i++) {

      if(modules[i] == __pm2_self && _pm2_optimize[num]) {
	rpc_args *arg;
	marcel_t pid;
	marcel_attr_t attr;
#ifndef MINIMAL_PREEMPTION
	marcel_sem_t sem;
#endif
	unsigned granted;

	attr = _pm2_lrpc_attr[num];
	marcel_attr_setstackaddr(&attr,
				 slot_general_alloc(NULL, 0,
						    &granted, NULL));
	marcel_attr_setstacksize(&attr, granted);
	marcel_attr_setprio(&attr, pm2_attr->priority);
	marcel_attr_setschedpolicy(&attr, pm2_attr->sched_policy);

	marcel_create(&pid, &attr, (marcel_func_t)_pm2_rpc_funcs[num], NULL);
	marcel_getuserspace(pid, (void **)&arg);

	arg->num = num;
	arg->req = args;
	arg->tid = __pm2_self;
	arg->quick = FALSE;
	arg->sync = FALSE;

#ifndef MINIMAL_PREEMPTION
	arg->sem = &sem;
	marcel_sem_init(&sem, 0);
#else
	arg->initiator = marcel_self();
#endif
	marcel_run(pid, arg);

#ifdef MINIMAL_PREEMPTION
	marcel_explicityield(pid);
#else
	marcel_sem_P(&sem);
#endif
      } else {

#ifdef DEBUG
	if(modules[i] == __pm2_self && !mad_can_send_to_self())
	  RAISE(NOT_IMPLEMENTED);
#endif

#ifdef MAD2
	{
	  unsigned c;

	  pm2_attr_getchannel(pm2_attr, &c);
	  pm2_attr_setchannel(pm2_attr, channel(c, modules[i], REQUEST));
	}
#endif
	pm2_rawrpc_begin(modules[i], PM2_ASYNC_LRPC, pm2_attr);

	old_mad_pack_int(MAD_IN_HEADER, &__pm2_self, 1);
	old_mad_pack_int(MAD_IN_HEADER, &num, 1);
	old_mad_pack_int(MAD_IN_HEADER, &pm2_attr->priority, 1);
	old_mad_pack_int(MAD_IN_HEADER, &pm2_attr->sched_policy, 1);

	(*_pm2_pack_req_funcs[num])(args);

	pm2_rawrpc_end();
      }
    }
    pm2_enable_migration();
  }
}

void pm2_quick_async_rpc(int module, int num, pm2_attr_t *pm2_attr,
			 any_t args)
{
  pm2_attr_t attr;

  if(pm2_attr == NULL) {
    pm2_attr_init(&attr);
    pm2_attr = &attr;
  }

  pm2_disable_migration();

  if(module == __pm2_self && _pm2_optimize[num]) {
    rpc_args arg;
    marcel_sem_t sem;

    arg.num = num;
    arg.tid = __pm2_self;
    arg.req = args;
    arg.sync = FALSE;
    arg.quick = TRUE;
    arg.sem = &sem;
    marcel_sem_init(&sem, 0);

    (*_pm2_rpc_funcs[num])(&arg);

  } else {

#ifdef DEBUG
    if(module == __pm2_self && !mad_can_send_to_self())
      RAISE(NOT_IMPLEMENTED);
#endif

#ifdef MAD2
    {
      unsigned c;

      pm2_attr_getchannel(pm2_attr, &c);
      pm2_attr_setchannel(pm2_attr, channel(c, module, REQUEST));
    }
#endif
    pm2_rawrpc_begin(module, PM2_QUICK_ASYNC_LRPC, pm2_attr);

    old_mad_pack_int(MAD_IN_HEADER, &__pm2_self, 1);
    old_mad_pack_int(MAD_IN_HEADER, &num, 1);

    (*_pm2_pack_req_funcs[num])(args);

    pm2_rawrpc_end();
  }

  pm2_enable_migration();
}

static void netserver_quick_async_lrpc(void)
{
  rpc_args args;
  marcel_sem_t sem;

  old_mad_unpack_int(MAD_IN_HEADER, &args.tid, 1);
  old_mad_unpack_int(MAD_IN_HEADER, &args.num, 1);

  args.sem = &sem;
  args.sync = FALSE;
  args.quick = TRUE;
  marcel_sem_init(&sem, 0);

  (*_pm2_rpc_funcs[args.num])(&args);
}

void pm2_multi_quick_async_rpc(int *modules, int nb, int num, pm2_attr_t *pm2_attr,
			       any_t args)
{
  int i;

  if(nb) {
    pm2_attr_t attr;

    if(pm2_attr == NULL) {
      pm2_attr_init(&attr);
      pm2_attr = &attr;
    }

    pm2_disable_migration();

    for(i=0; i<nb; i++) {

      if(modules[i] == __pm2_self && _pm2_optimize[num]) {
	rpc_args arg;
	marcel_sem_t sem;

	arg.num = num;
	arg.tid = __pm2_self;
	arg.req = args;
	arg.sync = FALSE;
	arg.quick = TRUE;
	arg.sem = &sem;
	marcel_sem_init(&sem, 0);

	(*_pm2_rpc_funcs[num])(&arg);

      } else {

#ifdef DEBUG
	if(modules[i] == __pm2_self && !mad_can_send_to_self())
	  RAISE(NOT_IMPLEMENTED);
#endif

#ifdef MAD2
	{
	  unsigned c;

	  pm2_attr_getchannel(pm2_attr, &c);
	  pm2_attr_setchannel(pm2_attr, channel(c, modules[i], REQUEST));
	}
#endif
	pm2_rawrpc_begin(modules[i], PM2_QUICK_ASYNC_LRPC, pm2_attr);

	old_mad_pack_int(MAD_IN_HEADER, &__pm2_self, 1);
	old_mad_pack_int(MAD_IN_HEADER, &num, 1);

	(*_pm2_pack_req_funcs[num])(args);

	pm2_rawrpc_end();
      }
    }
    pm2_enable_migration();
  }
}

unsigned __pm2_rpc_init_called = 0;

void pm2_rpc_init(void)
{
  int i;

  pm2_console_init_rpc();
  pm2_isomalloc_init_rpc();
#ifdef DSM
  dsm_pm2_init_rpc();
#endif

  for(i=0; i<nb_of_channels; i++) {
    pm2_channel_alloc(&pm2_rpc_channel[i].channel[0]);
    pm2_channel_alloc(&pm2_rpc_channel[i].channel[1]);
  }

  pm2_rawrpc_register(&PM2_LRPC, netserver_lrpc);
  pm2_rawrpc_register(&PM2_LRPC_DONE, netserver_lrpc_done);
  pm2_rawrpc_register(&PM2_ASYNC_LRPC, netserver_async_lrpc);
  pm2_rawrpc_register(&PM2_QUICK_LRPC, netserver_quick_lrpc);
  pm2_rawrpc_register(&PM2_QUICK_ASYNC_LRPC, netserver_quick_async_lrpc);

  __pm2_rpc_init_called = 1;
}
