
#ifndef PM2_TYPES_EST_DEF
#define PM2_TYPES_EST_DEF

#include "marcel.h"

typedef void (*pm2_startup_func_t)(int argc, char *argv[], void *args);

typedef void (*pm2_rawrpc_func_t)(void);

typedef void (*pm2_completion_handler_t)(void *);

typedef void (*pm2_pre_migration_hook)(marcel_t pid);
typedef any_t (*pm2_post_migration_hook)(marcel_t pid);
typedef void (*pm2_post_post_migration_hook)(any_t key);


_PRIVATE_ typedef struct pm2_completion_struct_t {
  marcel_sem_t sem;
  unsigned proc;
  struct pm2_completion_struct_t *c_ptr;
  pm2_completion_handler_t handler;
  void *arg;
} pm2_completion_t;

_PRIVATE_ typedef struct {
  void *stack_base;
  marcel_t task;
  unsigned long depl, blk;
} pm2_migr_ctl;

#endif
