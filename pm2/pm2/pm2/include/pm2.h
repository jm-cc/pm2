
#ifndef PM2_EST_DEF
#define PM2_EST_DEF

#include "marcel.h"
#include "pm2_mad.h"
#include "pm2_types.h"
#include "pm2_attr.h"
#include "pm2_thread.h"
#include "block_alloc.h"
#include "pm2_timing.h"
#include "sys/debug.h"
#include "pm2_rpc.h"
#include "isoaddr_attr.h"
#include "pm2_sync.h"

#ifdef DSM
#include "dsm_pm2.h"
#endif

#include "common.h"

#define MAX_STARTUP_FUNCS   32

/* A startup function may be specified. If so, it will be called after
 * all modules will be spawned but before the current module will
 * listen (and respond to) incomming requests.  */

void pm2_push_startup_func(pm2_startup_func_t f, void *args);

/* 
 * Init the system and spawns `nb_proc-1' additional processes (or one
 * per node if specified). Upon completion, task identifiers are
 * pointed by `*tids' and the test `pm2_self() == (*tids)[0]' is true
 * for the initial process.
 */

#define pm2_init(argc, argv) common_init(argc, argv)

// Needs to be called after profile_init()
_PRIVATE_ void pm2_init_data(int *argc, char **argv);
// Needs to be called after mad_init()
_PRIVATE_ void pm2_init_open_channels(int *argc, char *argv[],
				      unsigned pm2_self,
				      unsigned pm2_config_size);
// Needs to be called after marcel_init()
_PRIVATE_ void pm2_init_thread_related(int *argc, char *argv[]);
// Starts netserver threads => begins answering the network
_PRIVATE_ void pm2_init_listen_network(int *argc, char *argv[]);
// Removes pm2 arguments from cmdline...
_PRIVATE_ void pm2_init_purge_cmdline(int *argc, char *argv[]);

void pm2_halt(void);

void pm2_exit(void);

_PRIVATE_ extern unsigned __pm2_self, __pm2_conf_size;

static __inline__ unsigned pm2_self(void) __attribute__ ((unused));
static __inline__ unsigned pm2_self(void)
{
  return __pm2_self;
}

static __inline__ unsigned pm2_config_size(void) __attribute__ ((unused));
static __inline__ unsigned pm2_config_size(void)
{
  return __pm2_conf_size;
}

void pm2_printf(char *format, ...);


/****************** "RAW" RPC : *******************/

void pm2_rawrpc_register(int *num, pm2_rawrpc_func_t func);

void pm2_rawrpc_begin(int module, int num, pm2_attr_t *pm2_attr);

void pm2_rawrpc_end(void);

#define pm2_rawrpc_waitdata() \
  { \
    mad_recvbuf_receive(); \
    marcel_sem_V((marcel_sem_t *)marcel_getspecific(_pm2_mad_key)); \
  }

void pm2_completion_init(pm2_completion_t *c,
			 pm2_completion_handler_t handler,
			 void *arg);

void pm2_completion_copy(pm2_completion_t *to, 
			 pm2_completion_t *from);

void pm2_pack_completion(mad_send_mode_t sm,
			 mad_receive_mode_t rm,
			 pm2_completion_t *c);
void pm2_unpack_completion(mad_send_mode_t sm,
			   mad_receive_mode_t rm,
			   pm2_completion_t *c);

void pm2_completion_wait(pm2_completion_t *c);

void pm2_completion_signal(pm2_completion_t *c);
void pm2_completion_signal_begin(pm2_completion_t *c);
void pm2_completion_signal_end(void);


void pm2_channel_alloc(pm2_channel_t *channel);


/******************** Migration **************************/

void pm2_set_pre_migration_func(pm2_pre_migration_hook f);
void pm2_set_post_migration_func(pm2_post_migration_hook f);
void pm2_set_post_post_migration_func(pm2_post_post_migration_hook f);

#define pm2_enable_migration()	marcel_enablemigration(marcel_self())
#define pm2_disable_migration()	marcel_disablemigration(marcel_self())

static __inline__ void pm2_freeze(void)
{
  lock_task();
}

static __inline__ void pm2_unfreeze(void) __attribute__ ((unused));
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

static __inline__ void pm2_migrate_self(int module) __attribute__ ((unused));
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
   block_alloc((block_descr_t *)marcel_getspecific(_pm2_block_key), size, NULL)
#define pm2_isofree(addr) \
   block_free((block_descr_t *)marcel_getspecific(_pm2_block_key), addr)
#define pm2_isomalloc_dataset_init(descr) \
   block_init_list(descr)
#define pm2_isomalloc_dataset(descr, size) \
   block_alloc(descr, size, NULL)
#define pm2_isofree_dataset(descr, addr) \
   block_free(descr, addr)
#define pm2_dataset_attach(descr) \
   block_merge_lists(descr, \
                     (block_descr_t *)marcel_getspecific(_pm2_block_key))
#define pm2_dataset_attach_other(task,descr) \
   block_merge_lists(descr, (block_descr_t *) task->key[_pm2_block_key])

void *pm2_malloc(size_t size, isoaddr_attr_t *attr);

void pm2_empty();

/*********************************************************/

typedef void (*pm2_user_func)(int argc, char **argv);
void pm2_set_user_func(pm2_user_func f);


_PRIVATE_ extern marcel_key_t _pm2_lrpc_num_key,
  _pm2_block_key, _pm2_mad_key, _pm2_isomalloc_nego_key;

#define pm2_main marcel_main


/*********************************************************/

/* pm2_set_user_func is obsolete. Please rather use the following
   function, which allows the registration of several startup
   functions... */

static __inline__ void pm2_set_startup_func(pm2_startup_func_t f, void *args) __attribute__ ((unused));
static __inline__ void pm2_set_startup_func(pm2_startup_func_t f, void *args)
{
  pm2_push_startup_func(f, args);
}

#endif
