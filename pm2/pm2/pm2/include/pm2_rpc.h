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

______________________________________________________________________________
$Log: pm2_rpc.h,v $
Revision 1.3  2000/02/28 11:18:02  rnamyst
Changed #include <> into #include "".

Revision 1.2  2000/01/31 15:49:42  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef PM2_RPC_EST_DEF
#define PM2_RPC_EST_DEF

void pm2_rpc_init(void);

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

#include "sys/console.h"
#include "sys/isomalloc_rpc.h"
#ifdef DSM
#include "dsm_rpc.h"
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

_PRIVATE_ int pm2_lrpc_runned_by(marcel_t pid);
_PRIVATE_ char *pm2_lrpc_name(int num);

typedef unsigned pm2_rpc_channel_t;

void pm2_rpc_channel_alloc(pm2_rpc_channel_t *channel);

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
		pm2_rawrpc_waitdata(); \
	} \
        RELEASE_CALLER(_arg); \
	{

_PRIVATE_ void _end_service(rpc_args *args, any_t res, int local);

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

#endif
