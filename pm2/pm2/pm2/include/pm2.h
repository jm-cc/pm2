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

#ifndef PM2_EST_DEF
#define PM2_EST_DEF

#include <marcel.h>
#include <safe_malloc.h>
#include <madeleine.h>
#include <pm2_attr.h>
#include <block_alloc.h>
#include <sys/slot_distrib.h>
#include <pm2_timing.h>
#include <sys/debug.h>

/* A startup function may be specified. If so, it will be called after
 * all modules will be spawned but before the current module will
 * listen (and respond to) incomming request 
 */
typedef void (*pm2_startup_func_t)(int *modules, int nb);
void pm2_set_startup_func(pm2_startup_func_t f);

void pm2_init_rpc(void);

#define ASK_USER	-1
#define ONE_PER_NODE	0

/* 
 * Init the system and spawns `nb_proc-1' additional processes (or one
 * per node if specified). Upon completion, task identifiers are
 * pointed by `*tids' and the test `pm2_self() == (*tids)[0]' is true
 * for the initial process.
 */

void pm2_init(int *argc, char **argv, int nb_proc, int **tids, int *nb);

void pm2_exit(void);

void pm2_kill_modules(int *modules, int nb);

_PRIVATE_ extern int __pm2_self;

static __inline__ int pm2_self(void)
{
  return __pm2_self;
}

void pm2_enter_critical_section(void);
void pm2_leave_critical_section(void);

void pm2_printf(char *format, ...);

extern exception	LRPC_ERROR;

#define LRPC_DECL_REQ(name, decl) typedef struct { decl } \
	_##name##_req; \
	void name##_pack_req(any_t _arg); \
	void name##_unpack_req(any_t _arg);

#define LRPC_DECL_RES(name, decl) typedef struct { decl } \
	_##name##_res; \
	void name##_pack_res(any_t _arg); \
	void name##_unpack_res(any_t _arg);


#define BEGIN_LRPC_LIST unsigned int
#define END_LRPC_LIST	;

#include <sys/console.h>
#include <sys/isomalloc_rpc.h>
#ifdef DSM
#include <dsm_rpc.h>
#endif

#define LRPC_REQ(name) _##name##_req
#define LRPC_RES(name) _##name##_res

#define PACK_REQ_STUB(name) \
	void name##_pack_req(any_t _arg) \
	{ _##name##_req *arg = (_##name##_req *)_arg;

#define UNPACK_REQ_STUB(name) \
	void name##_unpack_req(any_t _arg) \
	{ _##name##_req *arg = (_##name##_req *)_arg;

#define PACK_RES_STUB(name) \
	void name##_pack_res(any_t _arg) \
	{ _##name##_res *arg = (_##name##_res *)_arg;

#define UNPACK_RES_STUB(name) \
	void name##_unpack_res(any_t _arg) \
	{ _##name##_res *arg = (_##name##_res *)_arg;

#define END_STUB	arg=_arg; }


typedef void (*pack_func_t)(any_t arg);
typedef void (*unpack_func_t)(any_t arg);
typedef void (*rpc_func_t)(any_t arg);

enum {
  DO_NOT_OPTIMIZE,
  OPTIMIZE_IF_LOCAL
};

void pm2_register_service(int *num, rpc_func_t func,
			  int req_size, pack_func_t pack_req, unpack_func_t unpack_req,
			  int res_size, pack_func_t pack_res, unpack_func_t unpack_res,
			  char *name, int optimize_if_local);

#define DECLARE_LRPC(name) \
	pm2_register_service(&name, (rpc_func_t) _##name##_func, \
	sizeof(LRPC_REQ(name)), name##_pack_req, name##_unpack_req, \
	sizeof(LRPC_RES(name)), name##_pack_res, name##_unpack_res, \
	#name, OPTIMIZE_IF_LOCAL)

#define DECLARE_LRPC_WITH_NAME(name, string, optimize_if_local) \
	pm2_register_service(&name, (rpc_func_t) _##name##_func, \
	sizeof(LRPC_REQ(name)), name##_pack_req, name##_unpack_req, \
	sizeof(LRPC_RES(name)), name##_pack_res, name##_unpack_res, \
	string, optimize_if_local)

#define DECLARE_RAW_RPC(name) \
	pm2_register_service(&name, (rpc_func_t) _##name##_func, \
	0, NULL, NULL, \
	0, NULL, NULL, \
	#name, DO_NOT_OPTIMIZE)


_PRIVATE_ int pm2_lrpc_runned_by(marcel_t pid);
_PRIVATE_ char *pm2_lrpc_name(int num);

#ifdef MAD2
_PRIVATE_ struct pm2_channel_struct_t;
_PRIVATE_ typedef struct pm2_channel_struct_t *pm2_channel_t;
#endif

/************* LRPC avec attente differee : **************/

/*
 * LRP_CALL(module,
 *          service_name,
 *          priority,
 *          stack_size,
 *          request_ptr,
 *          result_ptr,
 *          wait_struct_ptr)
 *
 * QUICK_LRP_CALL(module,
 *                service_name,
 *                request_ptr,
 *                result_ptr,
 *                wait_struct_ptr)
 *
 * LRP_WAIT(wait_struct_ptr)
 */

_PRIVATE_ typedef struct _pm2_rpc_wait {
   marcel_sem_t sem;
   any_t result;
   unpack_func_t unpack;
   int tid;
   pointer ptr_att;
} pm2_rpc_wait_t;

void pm2_rpc_call(int module, int num, pm2_attr_t *pm2_attr,
		  any_t args, any_t results, pm2_rpc_wait_t *att);

#define LRP_CALL(module, name, prio, stack, args, results, att) \
	pm2_rpc_call(module, name, NULL, args, results, att)

void pm2_quick_rpc_call(int module, int num, pm2_attr_t *pm2_attr,
			any_t args, any_t results, pm2_rpc_wait_t *att);

#define QUICK_LRP_CALL(module, name, args, results, att) \
	pm2_quick_rpc_call(module, name, NULL, args, results, att)

void pm2_rpc_wait(pm2_rpc_wait_t *att);

#define LRP_WAIT(att) \
	pm2_rpc_wait(att)

void pm2_cancel_wait(pm2_rpc_wait_t *att);


_PRIVATE_ void pm2_rpc_call_begin(int module, int num,
				  pm2_attr_t *pm2_attr,
				  any_t args, any_t results,
				  pm2_rpc_wait_t *att);
_PRIVATE_ void pm2_rpc_call_end(void);

/****************** LRPC synchrones : ********************/

/*
 * LRPC(module,
 *      service_name,
 *      priority,
 *      stack_size,
 *      request_ptr,
 *      result_ptr)
 *
 * QUICK_LRPC(module,
 *            service_name,
 *            request_ptr,
 *            result_ptr)
 *
 */

static __inline__ void pm2_rpc(int module, int num, pm2_attr_t *pm2_attr,
			       any_t args, any_t results)
{
  pm2_rpc_wait_t att;

  pm2_rpc_call(module, num, pm2_attr, args, results, &att);
  pm2_rpc_wait(&att);
}

static __inline__ void pm2_quick_rpc(int module, int num, pm2_attr_t *pm2_attr,
				     any_t args, any_t results)
{
  pm2_rpc_wait_t att;

  pm2_quick_rpc_call(module, num, pm2_attr, args, results, &att);
  pm2_rpc_wait(&att);
}

#define LRPC(module, name, prio, stack, args, results) \
	pm2_rpc(module, name, NULL, args, results)

#define QUICK_LRPC(module, name, args, results) \
	pm2_quick_rpc(module, name, NULL, args, results)


#define pm2_rpc_begin(module, num, pm2_attr, args, results) \
{ \
  pm2_rpc_wait_t att; \
  pm2_rpc_call_begin(module, num, pm2_attr, args, results, &att);

#define pm2_rpc_end() \
  pm2_rpc_call_end(); \
  pm2_rpc_wait(&att); \
}

/****************** LRPC asynchrones : *******************/

/*
 * ASYNC_LRPC(module,
 *            service_name,
 *            priority,
 *            stack_size,
 *            request_ptr)
 *
 * QUICK_ASYNC_LRPC(module,
 *                  service_name,
 *                  request_ptr)
 */

void pm2_async_rpc(int module, int num, pm2_attr_t *pm2_attr,
		   any_t args);

#define ASYNC_LRPC(module, name, prio, stack, args) \
	pm2_async_rpc(module, name, NULL, args)

void pm2_quick_async_rpc(int module, int num, pm2_attr_t *pm2_attr,
			 any_t args);

#define QUICK_ASYNC_LRPC(module, name, args) \
	pm2_quick_async_rpc(module, name, NULL, args)


_PRIVATE_ void pm2_async_rpc_begin(int module, int num,
				   pm2_attr_t *pm2_attr,
				   any_t args);
_PRIVATE_ void pm2_async_rpc_end(void);

_PRIVATE_ void pm2_rawrpc_begin(int module, int num,
				pm2_attr_t *pm2_attr);
_PRIVATE_ void pm2_rawrpc_end(void);

/************ LRPC asynchrones en multicast : ************/

/*
 * MULTI_ASYNC_LRPC(tab_modules,
 *                  nb_modules,
 *                  service_name,
 *                  priority,
 *                  stack_size,
 *                  request_ptr)
 *
 * MULTI_QUICK_ASYNC_LRPC(tab_modules,
 *                        nb_modules,
 *                        service_name,
 *                        request_ptr)
 */

void pm2_multi_async_rpc(int *modules, int nb, int num, pm2_attr_t *pm2_attr,
			 any_t args);

#define MULTI_ASYNC_LRPC(modules, nb, name, prio, stack, args) \
	pm2_multi_async_rpc(modules, nb, name, NULL, args)

void pm2_multi_quick_async_rpc(int *modules, int nb, int num, pm2_attr_t *pm2_attr,
			       any_t args);

#define MULTI_QUICK_ASYNC_LRPC(modules, nb, name, args) \
	pm2_multi_quick_async_rpc(modules, nb, name, NULL, args)


/******************** Migration **************************/

typedef void (*pm2_pre_migration_hook)(marcel_t pid);
typedef any_t (*pm2_post_migration_hook)(marcel_t pid);
typedef void (*pm2_post_post_migration_hook)(any_t key);

void pm2_set_pre_migration_func(pm2_pre_migration_hook f);
void pm2_set_post_migration_func(pm2_post_migration_hook f);
void pm2_set_post_post_migration_func(pm2_post_post_migration_hook f);

#define pm2_enable_migration()	marcel_enablemigration(marcel_self())
#define pm2_disable_migration()	marcel_disablemigration(marcel_self())

static __inline__ void pm2_freeze(void)
{
  lock_task();
}


static __inline__ void pm2_unfreeze(void)
{
  unlock_task();
}

void pm2_migrate_group(marcel_t *pids, int nb, int module);
void pm2_migrate(marcel_t pid, int module);

static __inline__ void pm2_migrate_self(int module)
{
  marcel_t self = marcel_self();

  pm2_freeze();
  pm2_migrate_group(&self, 1, module);
}

void pm2_backup_group(marcel_t *pids, int nb, char *file);

void pm2_restore_group(char *file);

_PRIVATE_ typedef struct {
  void *stack_base;
  marcel_t task;
  unsigned long depl, blk;
} pm2_migr_ctl;


/**********************************************************/

/*                  ISOMALLOC                             */

typedef block_descr_t isomalloc_dataset_t;

#define pm2_isomalloc(size) \
   block_alloc((block_descr_t *)marcel_getspecific(_pm2_block_key), size)
#define pm2_isofree(addr) \
   block_free((block_descr_t *)marcel_getspecific(_pm2_block_key), addr)
#define pm2_isomalloc_dataset_init(descr) \
   block_init_list(descr)
#define pm2_isomalloc_dataset(descr, size) \
   block_alloc(descr, size)
#define pm2_isofree_dataset(descr, addr) \
   block_free(descr, addr)
#define pm2_dataset_attach(descr) \
   block_merge_lists(descr, \
                     (block_descr_t *)marcel_getspecific(_pm2_block_key))

#define pm2_dataset_attach_other(task,descr) \
   block_merge_lists(descr, (block_descr_t *) task->key[_pm2_block_key])

/*********************************************************/

/*
	Les valeurs possibles pour *which* sont :
		ALL_THREADS
	ou	MIGRATABLE_ONLY
*/
void pm2_threads_list(int max, marcel_t *pids, int *nb, int which);


/*********************************************************/

_PRIVATE_ typedef struct {
  int tid, num, not_migratable;
  boolean sync, quick;
  any_t req;
  marcel_sem_t *sem;
  marcel_t initiator;
  pointer ptr_att;
  pm2_rpc_wait_t *true_ptr_att;
  block_descr_t sd;
} rpc_args;

#define EXTERN_LRPC_SERVICE(name) \
	void _##name##_func(rpc_args *arg)

#ifdef MINIMAL_PREEMPTION
#define RELEASE_CALLER(arg) if(!_quick) marcel_explicityield((arg)->initiator)
#else
#define RELEASE_CALLER(arg) marcel_sem_V((arg)->sem)
#endif

_PRIVATE_ extern int _pm2_optimize[];

_PRIVATE_ extern marcel_key_t _pm2_lrpc_num_key, _pm2_block_key, _pm2_isomalloc_nego_key;

_PRIVATE_ extern void _pm2_term_func(void *arg);

#define BEGIN_SERVICE(name) \
	void _##name##_func(rpc_args *_arg) { \
	boolean _sync, _quick, _local = FALSE; \
	LRPC_REQ(name) req; \
	LRPC_RES(name) res; \
        _sync = _arg->sync; \
	_quick = _arg->quick; \
	if(!_quick) { \
		marcel_setspecific(_pm2_lrpc_num_key, (any_t)((long)_arg->num)); \
		marcel_setspecific(_pm2_block_key, (any_t)&_arg->sd); \
		marcel_setspecific(_pm2_isomalloc_nego_key, (any_t) 0);\
		block_init_list(&_arg->sd); \
		marcel_cleanup_push(_pm2_term_func, marcel_self()); \
	} \
	if(_arg->tid == pm2_self() && _pm2_optimize[name]) { \
		if(_quick) {\
			_arg->not_migratable = marcel_self()->not_migratable; \
			_local = TRUE; \
		} \
		memcpy(&req, _arg->req, sizeof(LRPC_REQ(name))); \
	} else { \
		name##_unpack_req(&req); \
		mad_recvbuf_receive(); \
	} \
        RELEASE_CALLER(_arg); \
	{

#define BEGIN_STUBLESS_SERVICE(name) \
	void _##name##_func(rpc_args *_arg) { \
	boolean _sync, _quick, _local = FALSE; \
	LRPC_RES(name) res; \
        _sync = _arg->sync; \
	_quick = _arg->quick; \
	if(!_quick) { \
		marcel_setspecific(_pm2_lrpc_num_key, (any_t)((long)_arg->num)); \
		marcel_setspecific(_pm2_block_key, (any_t)&_arg->sd); \
		marcel_setspecific(_pm2_isomalloc_nego_key, (any_t) 0);\
		block_init_list(&_arg->sd); \
		marcel_cleanup_push(_pm2_term_func, marcel_self()); \
	} \
	if(_arg->tid == pm2_self() && _pm2_optimize[name]) { \
		RAISE(NOT_IMPLEMENTED); \
	} \
	{

#define BEGIN_RAW_SERVICE(name) \
  void _##name##_func(rpc_args *_arg) { \

#define pm2_raw_waitdata() mad_recvbuf_receive()

#define pm2_wait_for_data() \
  { \
    mad_recvbuf_receive(); \
    RELEASE_CALLER((rpc_args *)(marcel_self()->user_space_ptr)); \
  }

_PRIVATE_ void _end_service(rpc_args *args, any_t res, int local);

#define END_RAW_SERVICE(name) \
    }

#define END_SERVICE(name) \
	} \
	{ rpc_args *args; \
        	if(_sync) { \
		if(_quick) { \
			args = _arg; \
			if(_local) \
				marcel_self()->not_migratable = _arg->not_migratable; \
		} else { \
			marcel_disablemigration(marcel_self()); \
			marcel_getuserspace(marcel_self(), (void **)&args); \
		} \
		_end_service(args, &res, _local); \
		} else if(_local && _quick) \
			marcel_self()->not_migratable = _arg->not_migratable; \
	} \
	}



/*********************************************************/


#define SPLIT(nb, tab, type) \
	{ unsigned _total = (nb)+1, my_rank; \
	  char *_dest; \
	  marcel_sem_t _wait, *_ptr_wait; \
	  type *_contrib; \
	  int _module; \
	pm2_disable_migration(); \
	_module = pm2_self(); \
	_contrib = (type *)mad_aligned_malloc(sizeof(type) * _total); \
        _ptr_wait = &_wait; \
	marcel_sem_init(&_wait, 0); \
	my_rank = _pm2_fork(tab, nb); \
	_dest = (char *)&_contrib[my_rank]; \
	if(my_rank) pm2_enable_migration(); 

#define MERGE(my_contrib, tab) \
	_pm2_merge(_module, _ptr_wait, _dest, (char *)my_contrib, sizeof(my_contrib)); \
	if(my_rank != 0) \
		marcel_exit(NULL); \
	*(tab) = _contrib; \
	{ int _i; for(_i=0; _i<_total; _i++) marcel_sem_P(&_wait); }

#define END_SPLIT \
	mad_aligned_free(_contrib); \
	pm2_enable_migration(); \
	}


_PRIVATE_ extern marcel_key_t _pm2_cloning_key;
_PRIVATE_ int _pm2_fork(int *modules, int nb);
_PRIVATE_ void _pm2_merge(int module, marcel_sem_t *sem, char *dest, char *src, unsigned size);

#define pm2_main marcel_main

#endif
