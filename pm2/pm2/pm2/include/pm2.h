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
$Log: pm2.h,v $
Revision 1.15  2000/02/01 17:17:49  rnamyst
Modified to include pm2_mad.h

Revision 1.14  2000/01/31 15:49:38  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef PM2_EST_DEF
#define PM2_EST_DEF

#include <marcel.h>
#include <safe_malloc.h>
#include <pm2_mad.h>
#include <pm2_types.h>
#include <pm2_attr.h>
#include <pm2_thread.h>
#include <block_alloc.h>
#include <sys/slot_distrib.h>
#include <pm2_timing.h>
#include <sys/debug.h>
#include <pm2_rpc.h>

/* A startup function may be specified. If so, it will be called after
 * all modules will be spawned but before the current module will
 * listen (and respond to) incomming requests.
 */
typedef void (*pm2_startup_func_t)(void);
void pm2_set_startup_func(pm2_startup_func_t f);

/* 
 * Init the system and spawns `nb_proc-1' additional processes (or one
 * per node if specified). Upon completion, task identifiers are
 * pointed by `*tids' and the test `pm2_self() == (*tids)[0]' is true
 * for the initial process.
 */

void pm2_init(int *argc, char **argv);

void pm2_halt(void);

void pm2_exit(void);

_PRIVATE_ extern unsigned __pm2_self, __conf_size;

static __inline__ unsigned pm2_self(void)
{
  return __pm2_self;
}

static __inline__ unsigned pm2_config_size(void)
{
  return __conf_size;
}

void pm2_printf(char *format, ...);


/****************** "RAW" RPC : *******************/

typedef void (*pm2_rawrpc_func_t)(void);

void pm2_rawrpc_register(int *num, pm2_rawrpc_func_t func);

void pm2_rawrpc_begin(int module, int num, pm2_attr_t *pm2_attr);

void pm2_rawrpc_end(void);

#define pm2_rawrpc_waitdata() \
  { \
    mad_recvbuf_receive(); \
    marcel_sem_V((marcel_sem_t *)marcel_getspecific(_pm2_mad_key)); \
  }

void pm2_completion_init(pm2_completion_t *c);

void pm2_pack_completion(pm2_completion_t *c);
void pm2_unpack_completion(pm2_completion_t *c);

void pm2_completion_wait(pm2_completion_t *c);
void pm2_completion_signal(pm2_completion_t *c);


void pm2_channel_alloc(pm2_channel_t *channel);


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

/* Les valeurs possibles pour *which* sont :
		ALL_THREADS
	ou	MIGRATABLE_ONLY
*/
void pm2_threads_list(int max, marcel_t *pids, int *nb, int which);

void pm2_migrate_group(marcel_t *pids, int nb, int module);
void pm2_migrate(marcel_t pid, int module);

static __inline__ void pm2_migrate_self(int module)
{
  marcel_t self = marcel_self();

  pm2_freeze();
  pm2_migrate_group(&self, 1, module);
}


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

typedef void (*pm2_user_func)(int argc, char **argv);
void pm2_set_user_func(pm2_user_func f);


_PRIVATE_ extern marcel_key_t _pm2_lrpc_num_key,
  _pm2_block_key, _pm2_mad_key, _pm2_isomalloc_nego_key;

#define pm2_main marcel_main

#endif
