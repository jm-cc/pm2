
/* sample.c */

#include "marcel.h"

#define SCHED_POLICY   MARCEL_SCHED_BALANCE

any_t ALL_IS_OK = (any_t)123456789L;

#define NB    3

char *mess[NB] = { "boys", "girls", "people" };

any_t writer(any_t arg)
{
  int i, j;

  for(i=0;i<10;i++) {
    tfprintf(stderr, "Hi %s! (I'm %p on vp %d)\n",
	     (char*)arg, marcel_self(), marcel_current_vp());
    //tfprintf(stderr, "LWP(%d): Locked = %d\n", marcel_current_vp(), locked());
    j = 19000000; while(j--);
  }

  return ALL_IS_OK;
}

int marcel_main(int argc, char *argv[])
{
  any_t status;
  marcel_t pid[NB];
  marcel_attr_t attr;
  int i;

   marcel_trace_on();

   marcel_init(&argc, argv);

   marcel_attr_init(&attr);

#ifdef PROFILE
   profile_activate(FUT_ENABLE, MARCEL_PROF_MASK);
#endif

   for(i=0; i<NB; i++) {
     marcel_attr_setschedpolicy(&attr, SCHED_POLICY);
     marcel_create(&pid[i], &attr, writer, (any_t)mess[i]);
   }

   for(i=0; i<NB; i++) {
     marcel_join(pid[i], &status);
     if(status == ALL_IS_OK)
       tprintf("Thread %p completed ok.\n", pid[i]);
   }

#ifdef PROFILE
   profile_stop();
#endif

   marcel_end();
   return 0;
}



