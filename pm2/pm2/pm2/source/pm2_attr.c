
#include "pm2_attr.h"

pm2_attr_t pm2_attr_default = {
  STD_PRIO,           /* priority */
  MARCEL_SCHED_OTHER, /* scheduling policy */
  0,                  /* channel number */
  NULL                /* completion ptr */
};

int pm2_attr_init(pm2_attr_t *attr)
{
   *attr = pm2_attr_default;
   return 0;
}

int pm2_attr_setprio(pm2_attr_t *attr, unsigned prio)
{
   if(prio == 0 || prio > MAX_PRIO)
      RAISE(CONSTRAINT_ERROR);
   attr->priority = prio;
   return 0;
}

int pm2_attr_getprio(pm2_attr_t *attr, unsigned *prio)
{
   *prio = attr->priority;
   return 0;
}

int pm2_attr_setschedpolicy(pm2_attr_t *attr, int policy)
{
  attr->sched_policy = policy;
  return 0;
}

int pm2_attr_getschedpolicy(pm2_attr_t *attr, int *policy)
{
  *policy = attr->sched_policy;
  return 0;
}

int pm2_attr_setchannel(pm2_attr_t *attr, pm2_channel_t channel)
{
   attr->channel = channel;
   return 0;
}

int pm2_attr_getchannel(pm2_attr_t *attr, pm2_channel_t *channel)
{
   *channel = attr->channel;
   return 0;
}

int pm2_attr_setcompletion(pm2_attr_t *attr, pm2_completion_t *completion)
{
  attr->completion = completion;
  return 0;
}

int pm2_attr_getcompletion(pm2_attr_t *attr, pm2_completion_t **completion)
{
  *completion = attr->completion;
  return 0;
}

