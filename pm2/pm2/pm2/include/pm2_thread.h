
#ifndef PM2_THREAD_EST_DEF
#define PM2_THREAD_EST_DEF

#include "marcel.h"

typedef void (*pm2_func_t)(void *arg);

void pm2_thread_init(void);
void pm2_thread_exit(void);

marcel_t pm2_thread_create(pm2_func_t func, void *arg);

#endif
