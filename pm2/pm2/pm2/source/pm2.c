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
#include <isomalloc.h>
#include <block_alloc.h>

#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>

#include <pm2_timing.h>

#ifdef DSM
#include "dsm_pm2.h"
#endif

#undef min
#define min(a, b)	((a) < (b) ? (a) : (b))

#undef max
#define max(a, b)	((a) > (b) ? (a) : (b))

exception LRPC_ERROR = "LRPC_ERROR";

static pm2_startup_func_t startup_func = NULL;

static size_t stack_by_default;

#define MAX_LRPC_FUNCS	50

static int spmd_conf[MAX_MODULES], spmd_conf_size;

volatile unsigned _pm2_imported_threads = 0;

marcel_sem_t _pm2_critical[2];

int __pm2_self;

marcel_key_t _pm2_lrpc_mode_key,
  _pm2_lrpc_num_key,
  _pm2_cloning_key,
  _pm2_block_key,
  _pm2_isomalloc_nego_key;

static block_descr_t _pm2_main_block_descr;

#define MAIN_SERVICE	-2L
#define SYSTEM_SERVICE	-1L

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

volatile pm2_pre_migration_hook	_pm2_pre_migr_func = NULL;
volatile pm2_post_migration_hook _pm2_post_migr_func = NULL;
volatile pm2_post_post_migration_hook _pm2_post_post_migr_func = NULL;

char _pm2_print_buf[1024] __MAD_ALIGNED__;

long _pm2_rpc_failed, _pm2_rpc_incorrect;

#define pm2_main_module() (spmd_conf[0])

static marcel_sem_t print_mutex;

#define lock_print()   marcel_sem_P(&print_mutex)
#define unlock_print() marcel_sem_V(&print_mutex)

#define executed_by_main_thread() ((boolean)(long)marcel_getspecific(_pm2_lrpc_mode_key))

void pm2_enter_critical_section(void)
{
  if(!executed_by_main_thread()) {
    unsigned tag = NETSERVER_CRITICAL_SECTION;

    mad_sendbuf_init(__pm2_self);
    mad_pack_int(MAD_IN_HEADER, &tag, 1);
    mad_sendbuf_send();

    marcel_sem_P(&_pm2_critical[0]);
  }
}

void pm2_leave_critical_section(void)
{
  if(!executed_by_main_thread())
    marcel_sem_V(&_pm2_critical[1]);
}

void pm2_init_rpc(void)
{
  int i;

  for(i=0; i<MAX_LRPC_FUNCS; i++)
    _pm2_rpc_funcs[i] = NULL;

  pm2_console_init_rpc();
  pm2_isomalloc_init_rpc();
#ifdef DSM
  dsm_pm2_init_rpc();
#endif
}

char *pm2_lrpc_name(int num)
{
  switch(num) {
  case SYSTEM_SERVICE : return "_syst_";
  case MAIN_SERVICE : return "_main_";
  default : return rpc_names[num];
  }
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
    unsigned tag = NETSERVER_LRPC_DONE;

    mad_sendbuf_init(args->tid);
    mad_pack_int(MAD_IN_HEADER, &tag, 1);
    mad_pack_pointer(MAD_IN_HEADER, &args->ptr_att, 1);
    (*_pm2_pack_res_funcs[args->num])(res);

    mad_sendbuf_send();
  }
}

void pm2_set_startup_func(pm2_startup_func_t f)
{
  startup_func = f;
}

static void precalc_marcel_attr(void)
{
  int i;

  for(i=0; i<MAX_LRPC_FUNCS; i++) {
    if(_pm2_rpc_funcs[i] != NULL) {
      marcel_attr_init(&_pm2_lrpc_attr[i]);
      marcel_attr_setdetachstate(&_pm2_lrpc_attr[i], TRUE);
      marcel_attr_setuserspace(&_pm2_lrpc_attr[i], sizeof(rpc_args));
      /* marcel_attr_setschedpolicy(&_pm2_lrpc_attr[i], MARCEL_SCHED_AFFINITY); */
    }
  }
}

void pm2_printf(char *format, ...)
{ 
  va_list args;
  char *ptr;

  if(pm2_main_module() == __pm2_self) {
    lock_task();
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    unlock_task();
  } else {
    lock_print();
    sprintf(_pm2_print_buf, "[t%x] ", __pm2_self);
    ptr = _pm2_print_buf + strlen(_pm2_print_buf);
    va_start(args, format);
    vsprintf(ptr, format, args);
    va_end(args);
    {
      unsigned tag = NETSERVER_PRINTF;
      unsigned len = strlen(_pm2_print_buf)+1;

      mad_sendbuf_init(pm2_main_module());
      mad_pack_int(MAD_IN_HEADER, &tag, 1);
      mad_pack_int(MAD_IN_HEADER, &len, 1);
      mad_pack_byte(MAD_IN_HEADER, _pm2_print_buf, len);

      mad_sendbuf_send();
    }
    unlock_print();
  }
}

static int module_rank(int tid)
{
  int rank;

  for(rank=0; spmd_conf[rank] != tid; rank++) ;
  return rank;
}

/* Initialise spmd_conf, spmd_conf_size ET __pm2_self */
void pm2_init(int *argc, char **argv, int nb_proc, int **tids, int *nb)
{
  timing_init();

#ifdef USE_SAFE_MALLOC
  safe_malloc_init();
#endif   

  marcel_init(argc, argv);

  mad_init(argc, argv, nb_proc, spmd_conf, &spmd_conf_size, &__pm2_self);

  marcel_strip_cmdline(argc, argv);

  if(nb)
    *nb = spmd_conf_size;

  if(tids)
    *tids = spmd_conf;

  mad_buffers_init();

  block_init(module_rank(__pm2_self), spmd_conf_size);

  marcel_key_create(&_pm2_lrpc_mode_key, NULL);
  marcel_key_create(&_pm2_lrpc_num_key, NULL);
  marcel_key_create(&_pm2_cloning_key, NULL);
  marcel_key_create(&_pm2_block_key, NULL);
  marcel_key_create(&_pm2_isomalloc_nego_key, NULL);
  marcel_setspecific(_pm2_lrpc_num_key, (any_t)((long)MAIN_SERVICE));

  marcel_setspecific(_pm2_block_key, (any_t)(&_pm2_main_block_descr));
  block_init_list(&_pm2_main_block_descr);

  marcel_sem_init(&print_mutex, 1);
  marcel_sem_init(&_pm2_critical[0], 0);
  marcel_sem_init(&_pm2_critical[1], 0);

#ifdef DSM
  dsm_pm2_init(module_rank(__pm2_self), spmd_conf_size);
#endif

  stack_by_default = SLOT_SIZE - 1024;

  precalc_marcel_attr();

  if(startup_func != NULL)
    (*startup_func)(spmd_conf, spmd_conf_size);

  netserver_start(MAX_PRIO);
}

static void pm2_wait_end(void)
{
  char mess[128];
  static boolean already_called = FALSE;

   if(!already_called) {

     netserver_wait_end();

      marcel_end();

      sprintf(mess, "[Threads : %ld created, %d imported (%ld cached)]\n", marcel_createdthreads(), _pm2_imported_threads, marcel_cachedthreads());

      fprintf(stderr, mess);

      already_called = TRUE;
   }
}

void pm2_exit(void)
{
   pm2_wait_end();

   mad_exit();

   block_exit();

#ifdef DSM
  dsm_pm2_exit();
#endif
}

void pm2_kill_modules(int *modules, int nb)
{
  int i;
  unsigned tag = NETSERVER_END;

  for(i=0; i<nb; i++) {
    if(modules[i] == __pm2_self && !mad_can_send_to_self()) {
      netserver_stop();
    } else {
      mad_sendbuf_init(modules[i]);
      mad_pack_int(MAD_IN_HEADER, &tag, 1);
      mad_sendbuf_send();
    }
  }
}

void pm2_register_service(int *num, rpc_func_t func,
			  int req_size,
			  pack_func_t pack_req, unpack_func_t unpack_req,
			  int res_size,
			  pack_func_t pack_res, unpack_func_t unpack_res,
			  char *name, int optimize_if_local)
{
  static unsigned service_number = 0;

  *num = service_number++;
  _pm2_rpc_funcs[*num] = func;
  _pm2_req_sizes[*num] = req_size;
  _pm2_pack_req_funcs[*num] = pack_req;
  _pm2_unpack_req_funcs[*num] = unpack_req;
  _pm2_res_sizes[*num] = res_size;
  _pm2_pack_res_funcs[*num] = pack_res;
  _pm2_unpack_res_funcs[*num] = unpack_res;
  rpc_names[*num] = name;
  _pm2_optimize[*num] = optimize_if_local;
}

void pm2_rpc_call(int module, int num, pm2_attr_t *pm2_attr,
		  any_t args, any_t results, pm2_rpc_wait_t *att)
{
  if(pm2_attr == NULL)
    pm2_attr = &pm2_attr_default;

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
    unsigned tag = NETSERVER_LRPC;

#ifdef DEBUG
    if(module == __pm2_self && !mad_can_send_to_self())
      RAISE(NOT_IMPLEMENTED);
#endif

    to_pointer((any_t)att, &p);

    mad_sendbuf_init(module);
    mad_pack_int(MAD_IN_HEADER, &tag, 1);
    mad_pack_int(MAD_IN_HEADER, &__pm2_self, 1);
    mad_pack_int(MAD_IN_HEADER, &num, 1);
    mad_pack_int(MAD_IN_HEADER, &pm2_attr->priority, 1);
    mad_pack_int(MAD_IN_HEADER, &pm2_attr->sched_policy, 1);

    mad_pack_pointer(MAD_IN_HEADER, &p, 1);

    (*_pm2_pack_req_funcs[num])(args);

    mad_sendbuf_send();
  }
}

void pm2_rpc_call_begin(int module, int num,
			pm2_attr_t *pm2_attr,
			any_t args, any_t results,
			pm2_rpc_wait_t *att)
{
  pointer p;
  unsigned tag = NETSERVER_LRPC;

#ifdef DEBUG
  if(module == __pm2_self && !mad_can_send_to_self())
    RAISE(NOT_IMPLEMENTED);
#endif

  if(pm2_attr == NULL)
    pm2_attr = &pm2_attr_default;

  pm2_disable_migration();

  marcel_sem_init(&att->sem, 0);
  att->result = results;
  att->unpack = _pm2_unpack_res_funcs[num];

  to_pointer((any_t)att, &p);

  mad_sendbuf_init(module);
  mad_pack_int(MAD_IN_HEADER, &tag, 1);
  mad_pack_int(MAD_IN_HEADER, &__pm2_self, 1);
  mad_pack_int(MAD_IN_HEADER, &num, 1);
  mad_pack_int(MAD_IN_HEADER, &pm2_attr->priority, 1);
  mad_pack_int(MAD_IN_HEADER, &pm2_attr->sched_policy, 1);

  mad_pack_pointer(MAD_IN_HEADER, &p, 1);
}

void pm2_rpc_call_end(void)
{
  mad_sendbuf_send();
}

void pm2_quick_rpc_call(int module, int num, pm2_attr_t *pm2_attr,
			any_t args, any_t results, pm2_rpc_wait_t *att)
{
  if(pm2_attr == NULL)
    pm2_attr = &pm2_attr_default;

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
    unsigned tag = NETSERVER_QUICK_LRPC;

#ifdef DEBUG
    if(module == __pm2_self && !mad_can_send_to_self())
      RAISE(NOT_IMPLEMENTED);
#endif

    to_pointer((any_t)att, &p);
    att->unpack = _pm2_unpack_res_funcs[num];

    mad_sendbuf_init(module);
    mad_pack_int(MAD_IN_HEADER, &tag, 1);
    mad_pack_int(MAD_IN_HEADER, &__pm2_self, 1);
    mad_pack_int(MAD_IN_HEADER, &num, 1);
    mad_pack_pointer(MAD_IN_HEADER, &p, 1);

    (*_pm2_pack_req_funcs[num])(args);

    mad_sendbuf_send();
  }
}

void pm2_rpc_wait(pm2_rpc_wait_t *att)
{
  marcel_sem_P(&att->sem);

  if(att->result == (any_t)&_pm2_rpc_failed) {
    pm2_enable_migration();
    RAISE(LRPC_ERROR);
  }

  pm2_enable_migration();
}

void pm2_cancel_wait(pm2_rpc_wait_t *att)
{
  att->result = (any_t)&_pm2_rpc_failed;
  marcel_sem_V(&att->sem);
}

void pm2_async_rpc(int module, int num, pm2_attr_t *pm2_attr, any_t args)
{
  if(pm2_attr == NULL)
    pm2_attr = &pm2_attr_default;

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
    unsigned tag = NETSERVER_ASYNC_LRPC;

#ifdef DEBUG
    if(module == __pm2_self && !mad_can_send_to_self())
      RAISE(NOT_IMPLEMENTED);
#endif

    mad_sendbuf_init(module);
    mad_pack_int(MAD_IN_HEADER, &tag, 1);
    mad_pack_int(MAD_IN_HEADER, &__pm2_self, 1);
    mad_pack_int(MAD_IN_HEADER, &num, 1);
    mad_pack_int(MAD_IN_HEADER, &pm2_attr->priority, 1);
    mad_pack_int(MAD_IN_HEADER, &pm2_attr->sched_policy, 1);

    (*_pm2_pack_req_funcs[num])(args);

    mad_sendbuf_send();
  }

  pm2_enable_migration();
}

_PRIVATE_ void pm2_async_rpc_begin(int module, int num,
				   pm2_attr_t *pm2_attr,
				   any_t args)
{
  unsigned tag = NETSERVER_ASYNC_LRPC;

#ifdef DEBUG
  if(module == __pm2_self && !mad_can_send_to_self())
    RAISE(NOT_IMPLEMENTED);
#endif

  if(pm2_attr == NULL)
    pm2_attr = &pm2_attr_default;

  pm2_disable_migration();

  mad_sendbuf_init(module);
  mad_pack_int(MAD_IN_HEADER, &tag, 1);
  mad_pack_int(MAD_IN_HEADER, &__pm2_self, 1);
  mad_pack_int(MAD_IN_HEADER, &num, 1);
  mad_pack_int(MAD_IN_HEADER, &pm2_attr->priority, 1);
  mad_pack_int(MAD_IN_HEADER, &pm2_attr->sched_policy, 1);

}

_PRIVATE_ void pm2_async_rpc_end(void)
{
  mad_sendbuf_send();
  pm2_enable_migration();
}

void pm2_multi_async_rpc(int *modules, int nb, int num, pm2_attr_t *pm2_attr,
			 any_t args)
{
  int i;
  unsigned tag = NETSERVER_ASYNC_LRPC;


  if(nb) {

    if(pm2_attr == NULL)
      pm2_attr = &pm2_attr_default;

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

	mad_sendbuf_init(modules[i]);
	mad_pack_int(MAD_IN_HEADER, &tag, 1);
	mad_pack_int(MAD_IN_HEADER, &__pm2_self, 1);
	mad_pack_int(MAD_IN_HEADER, &num, 1);
	mad_pack_int(MAD_IN_HEADER, &pm2_attr->priority, 1);
	mad_pack_int(MAD_IN_HEADER, &pm2_attr->sched_policy, 1);

	(*_pm2_pack_req_funcs[num])(args);

	mad_sendbuf_send();
      }
    }
    pm2_enable_migration();
  }
}

void pm2_quick_async_rpc(int module, int num, pm2_attr_t *pm2_attr,
			 any_t args)
{
  if(pm2_attr == NULL)
    pm2_attr = &pm2_attr_default;

  pm2_disable_migration();

  TIMING_EVENT("quick_async_rpc'begin");

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
    unsigned tag = NETSERVER_QUICK_ASYNC_LRPC;

#ifdef DEBUG
    if(module == __pm2_self && !mad_can_send_to_self())
      RAISE(NOT_IMPLEMENTED);
#endif

    TIMING_EVENT("sendbuf_init'begin");

    mad_sendbuf_init(module);
    mad_pack_int(MAD_IN_HEADER, &tag, 1);
    mad_pack_int(MAD_IN_HEADER, &__pm2_self, 1);
    mad_pack_int(MAD_IN_HEADER, &num, 1);

    TIMING_EVENT("pack_req_stub'begin");

    (*_pm2_pack_req_funcs[num])(args);

    TIMING_EVENT("sendbuf_send'begin");

    mad_sendbuf_send();

    TIMING_EVENT("sendbuf_send'end");
  }

  pm2_enable_migration();
}

void pm2_multi_quick_async_rpc(int *modules, int nb, int num, pm2_attr_t *pm2_attr,
			       any_t args)
{
  int i;
  unsigned tag = NETSERVER_QUICK_ASYNC_LRPC;

  if(nb) {

    if(pm2_attr == NULL)
      pm2_attr = &pm2_attr_default;

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

	mad_sendbuf_init(modules[i]);
	mad_pack_int(MAD_IN_HEADER, &tag, 1);
	mad_pack_int(MAD_IN_HEADER, &__pm2_self, 1);
	mad_pack_int(MAD_IN_HEADER, &num, 1);

	(*_pm2_pack_req_funcs[num])(args);

	mad_sendbuf_send();
      }
    }
    pm2_enable_migration();
  }
}

typedef struct {
  int nb;
  marcel_t *pids;
  boolean do_send;
} migration_arg;

#ifdef MIGRATE_IN_HEADER
#define MIGR_MODE  MAD_IN_HEADER
#else
#define MIGR_MODE  MAD_IN_PLACE
#endif

static void migrate_func(marcel_t task, unsigned long depl, unsigned long blk, void *arg)
{ 
  migration_arg *args = (migration_arg *)arg;
  pm2_migr_ctl mctl;
  block_descr_t *block_descr_ptr;

  mctl.stack_base = marcel_stackbase(task);
  mctl.task = task;
  mctl.depl = depl;
  mctl.blk = blk;

  mad_pack_byte(MAD_IN_HEADER, (char *)&mctl, sizeof(mctl));

  mad_pack_byte(MIGR_MODE, (char *)mctl.stack_base + depl, blk);

  block_descr_ptr = (block_descr_t *)(*marcel_specificdatalocation(task, _pm2_block_key));
  block_pack_all(block_descr_ptr);

  if(_pm2_pre_migr_func != NULL)
    (*_pm2_pre_migr_func)(task);

  slot_slot_busy(mctl.stack_base);
  slot_list_busy(&block_descr_ptr->slot_descr);

  if(args->do_send) {
    mad_sendbuf_send();

#ifndef MIGRATE_IN_HEADER
    {
      int i;

      for(i=0; i<args->nb; i++)
	if(args->pids[i] != marcel_self())
	  marcel_cancel(args->pids[i]);
    }
#endif
  }
}

void pm2_migrate_group(marcel_t *pids, int nb, int module)
{ 
  migration_arg arg;
  int i;
  boolean me_too = FALSE;

  if(module == __pm2_self) {
    for(i=0; i<nb; i++) {
      pids[i]->depl = 0;
      marcel_unfreeze(pids, nb);
    }
    pm2_unfreeze();
  } else {
    unsigned tag = NETSERVER_MIGRATION;

    for(i=0; i<nb; i++)
      if(pids[i]->not_migratable || !pids[i]->detached) {
	RAISE(CONSTRAINT_ERROR);
      }

    marcel_freeze(pids, nb);
    pm2_unfreeze();

    arg.do_send = FALSE;
    arg.pids = pids;
    arg.nb = nb;

    mad_sendbuf_init(module);
    mad_pack_int(MAD_IN_HEADER, &tag, 1);
    mad_pack_int(MAD_IN_HEADER, &nb, 1);

    for(i=0; i<nb; i++) {
      if(pids[i] == marcel_self())
	me_too = TRUE;
      else
#ifdef MIGRATE_IN_HEADER
	marcel_begin_hibernation(pids[i], migrate_func, &arg, FALSE);
#else
      /* 
       * On met le flag "fork" à VRAI parce qu'il faut maintenir ces threads
       * en vie tant que le message n'est pas envoyé (IN_PLACE).
       */
      marcel_begin_hibernation(pids[i], migrate_func, &arg, TRUE);
#endif
    }

    if(me_too) {
      arg.do_send = TRUE;
      marcel_begin_hibernation(marcel_self(), migrate_func, &arg, FALSE);
    } else {
      mad_sendbuf_send();

#ifndef MIGRATE_IN_HEADER
      for(i=0; i<nb; i++)
	marcel_cancel(pids[i]);
#endif
    }
  }
}

void pm2_migrate(marcel_t pid, int module)
{
  pm2_migrate_group(&pid, 1, module);
}

typedef struct {
  int *modules, nb;
} cloning_arg;

static void cloning_func(marcel_t task, unsigned long depl, unsigned long blk, void *arg)
{
  cloning_arg *args = (cloning_arg *)arg;
  void *stack_base = marcel_stackbase(task);
  block_descr_t *block_descr_ptr;
  unsigned tag = NETSERVER_CLONING;
  unsigned rank;

   slot_slot_busy(stack_base);
   block_descr_ptr = (block_descr_t *)(*marcel_specificdatalocation(task,_pm2_block_key));
   slot_list_busy(&block_descr_ptr->slot_descr);

   for(rank=1; rank<=args->nb; rank++) {

     mad_sendbuf_init(args->modules[rank-1]);

     mad_pack_int(MAD_IN_HEADER, &tag, 1);
     mad_pack_int(MAD_IN_HEADER, &rank, 1);

     mad_pack_byte(MAD_IN_HEADER, (char *)&stack_base, sizeof(void *));

     mad_pack_byte(MAD_IN_HEADER, (char *)&task, sizeof(marcel_t));

     mad_pack_long(MAD_IN_HEADER, &depl, 1);
     mad_pack_long(MAD_IN_HEADER, &blk, 1);

     mad_pack_byte(MAD_IN_PLACE, (char *)stack_base + depl, blk);

     block_pack_all(block_descr_ptr);

     if(_pm2_pre_migr_func != NULL)
       (*_pm2_pre_migr_func)(task);

     mad_sendbuf_send();
   }
   marcel_setspecific(_pm2_cloning_key, (any_t)0);
}

int _pm2_fork(int *modules, int nb)
{
  cloning_arg arg;

  marcel_disablemigration(marcel_self());

  arg.modules = modules;
  arg.nb = nb;

  marcel_begin_hibernation(marcel_self(), cloning_func, &arg, TRUE);

  marcel_enablemigration(marcel_self());
  return (int)(long)(*marcel_specificdatalocation(marcel_self(), _pm2_cloning_key));
}

void pm2_threads_list(int max, marcel_t *pids, int *nb, int which)
{ int mask = which | DETACHED_ONLY;

   if(which == MIGRATABLE_ONLY)
      mask |= NOT_BLOCKED_ONLY;

   marcel_threadslist(max, pids, nb, mask);
}

typedef struct {
	int fd;
} backup_arg;


void pm2_backup_group(marcel_t *pids, int nb, char *file)
{
  RAISE(NOT_IMPLEMENTED);
}

void pm2_restore_group(char *file)
{
  RAISE(NOT_IMPLEMENTED);
}

void pm2_set_pre_migration_func(pm2_pre_migration_hook f)
{
   _pm2_pre_migr_func = f;
}

void pm2_set_post_migration_func(pm2_post_migration_hook f)
{
   _pm2_post_migr_func = f;
}

void pm2_set_post_post_migration_func(pm2_post_post_migration_hook f)
{
   _pm2_post_post_migr_func = f;
}

void _pm2_merge(int module, marcel_sem_t *sem, char *dest, char *src, unsigned size)
{
  unsigned tag = NETSERVER_MERGE;

  marcel_disablemigration(marcel_self());

  if(module == __pm2_self) {

    memcpy(dest, src, size);
    marcel_sem_V(sem);

  } else {

    mad_sendbuf_init(module);

    mad_pack_int(MAD_IN_HEADER, &tag, 1);
    mad_pack_byte(MAD_IN_HEADER, (char *)&sem, sizeof(marcel_sem_t *));
    mad_pack_byte(MAD_IN_HEADER, (char *)&dest, sizeof(char *));
    mad_pack_int(MAD_IN_HEADER, &size, 1);
    mad_pack_byte(MAD_IN_HEADER, src, size);

    mad_sendbuf_send();
   }

   marcel_enablemigration(marcel_self());
}

