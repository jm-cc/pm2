
#ifndef PM2_ATTR_EST_DEF
#define PM2_ATTR_EST_DEF

#include "marcel.h"
#include "madeleine.h"
#include "pm2_types.h"

typedef unsigned pm2_channel_t;

_PRIVATE_ typedef struct {
  unsigned priority;
  int sched_policy;
  pm2_channel_t channel;
  pm2_completion_t *completion;
} pm2_attr_t;

int pm2_attr_init(pm2_attr_t *attr);

int pm2_attr_setprio(pm2_attr_t *attr, unsigned prio);
int pm2_attr_getprio(pm2_attr_t *attr, unsigned *prio);

int pm2_attr_setschedpolicy(pm2_attr_t *attr, int policy);
int pm2_attr_getschedpolicy(pm2_attr_t *attr, int *policy);

int pm2_attr_setchannel(pm2_attr_t *attr, unsigned channel);
int pm2_attr_getchannel(pm2_attr_t *attr, unsigned *channel);

int pm2_attr_setcompletion(pm2_attr_t *attr, pm2_completion_t *completion);
int pm2_attr_getcompletion(pm2_attr_t *attr, pm2_completion_t **completion);

_PRIVATE_ extern pm2_attr_t pm2_attr_default;

#endif
