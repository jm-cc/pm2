
#ifndef PM2_SYNC_EST_DEF
#define PM2_SYNC_EST_DEF

#include "marcel.h"

typedef struct {
  int *tab_sync;
  marcel_mutex_t sync_mutex;
} pm2_barrier_t;


typedef struct {
  marcel_sem_t mutex, wait;
  pm2_barrier_t node_barrier;
  int local;
  int nb;
} pm2_thread_barrier_t;

typedef struct {
  int local;
} pm2_thread_barrier_attr_t;


void pm2_barrier_init_rpc();

void pm2_sync_init(int myself, int confsize);


/* The following two primitives need to be called by PM2 in a startup function,
to allow for proper barrier initialization. */
void pm2_barrier_init(pm2_barrier_t *bar);

void pm2_thread_barrier_init(pm2_thread_barrier_t *bar, pm2_thread_barrier_attr_t *attr);

/* Node-level barrier */
void pm2_barrier(pm2_barrier_t *bar);

/*Thread-level barrier */
void pm2_thread_barrier(pm2_thread_barrier_t *bar);

#endif
