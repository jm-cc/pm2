
#include "marcel.h"

	/* Déclaré non statique car utilisé dans marcel.c : */
marcel_attr_t marcel_attr_default = {
  DEFAULT_STACK,           /* stack size */
  NULL,                    /* stack base */
  MARCEL_CREATE_JOINABLE,  /* detached */
  0,                       /* user space */
  FALSE,                   /* immediate activation */
  STD_PRIO,                /* priority */
  1,                       /* not_migratable */
  0,                       /* not_deviatable */
  MARCEL_SCHED_OTHER       /* scheduling policy */
};

/* Déclaré dans marcel.c : */
extern volatile unsigned default_stack;


int marcel_attr_init(marcel_attr_t *attr)
{
   *attr = marcel_attr_default;
   return 0;
}

int marcel_attr_setstacksize(marcel_attr_t *attr, size_t stack)
{
   attr->stack_size = stack;
   return 0;
}

int marcel_attr_getstacksize(marcel_attr_t *attr, size_t *stack)
{
  *stack = attr->stack_size;
  return 0;
}

int marcel_attr_setstackaddr(marcel_attr_t *attr, void *addr)
{
   attr->stack_base = addr;
   return 0;
}

int marcel_attr_getstackaddr(marcel_attr_t *attr, void **addr)
{
   *addr = attr->stack_base;
   return 0;
}

int marcel_attr_setdetachstate(marcel_attr_t *attr, boolean detached)
{
   attr->detached = detached;
   return 0;
}

int marcel_attr_getdetachstate(marcel_attr_t *attr, boolean *detached)
{
   *detached = attr->detached;
   return 0;
}

int marcel_attr_setuserspace(marcel_attr_t *attr, unsigned space)
{
   attr->user_space = space;
   return 0;
}

int marcel_attr_getuserspace(marcel_attr_t *attr, unsigned *space)
{
   *space = attr->user_space;
   return 0;
}

int marcel_attr_setactivation(marcel_attr_t *attr, boolean immediate)
{
  attr->immediate_activation = immediate;
  return 0;
}

int marcel_attr_getactivation(marcel_attr_t *attr, boolean *immediate)
{
  *immediate = attr->immediate_activation;
  return 0;
}

int marcel_attr_setprio(marcel_attr_t *attr, unsigned prio)
{
   if(prio == 0 || prio > MAX_PRIO)
      RAISE(CONSTRAINT_ERROR);
   attr->priority = prio;
   return 0;
}

int marcel_attr_getprio(marcel_attr_t *attr, unsigned *prio)
{
   *prio = attr->priority;
   return 0;
}

int marcel_attr_setmigrationstate(marcel_attr_t *attr, boolean migratable)
{
   attr->not_migratable = (migratable ? 0 : 1 );
   return 0;
}

int marcel_attr_getmigrationstate(marcel_attr_t *attr, boolean *migratable)
{
   *migratable = (attr->not_migratable ? FALSE : TRUE);
   return 0;
}

int marcel_attr_setdeviationstate(marcel_attr_t *attr, boolean deviatable)
{
   attr->not_deviatable = (deviatable ? 0 : 1);
   return 0;
}

int marcel_attr_getdeviationstate(marcel_attr_t *attr, boolean *deviatable)
{
   *deviatable = (attr->not_deviatable ? FALSE : TRUE);
   return 0;
}

int marcel_attr_setschedpolicy(marcel_attr_t *attr, int policy)
{
  attr->sched_policy = policy;
  return 0;
}

int marcel_attr_getschedpolicy(marcel_attr_t *attr, int *policy)
{
  *policy = attr->sched_policy;
  return 0;
}
