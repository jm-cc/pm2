
#include "marcel.h"

#ifdef MA__WORK

volatile int marcel_global_work=0;
marcel_lock_t marcel_work_lock=MARCEL_LOCK_INIT_UNLOCKED;


void do_work(marcel_t self)
{
  int has_done_work=0;
#ifdef DEBUG
  if(locked() != 1)
    RAISE(LOCK_TASK_ERROR);
#endif
  LOCK_WORK(self);
  MTRACE("DO_WORK", self);
  mdebug_work("do_work : marcel_global_work=%p, has_work=%p\n", 
	      (void*)marcel_global_work, (void*)self->has_work);
  while (marcel_global_work || self->has_work) {
    if (self->has_work & MARCEL_WORK_DEVIATE) {
      handler_func_t h=self->deviation_func;
      any_t arg=self->deviation_arg;
      self->deviation_func=NULL;
      self->has_work &= ~MARCEL_WORK_DEVIATE;
      UNLOCK_WORK(self);
      unlock_task();
      (*h)(arg);
      lock_task();
      LOCK_WORK(self);     
    }
    if ((marcel_global_work  || self->has_work) && !has_done_work) {
      RAISE(PROGRAM_ERROR);
    }
  }

  UNLOCK_WORK(self);

}

#endif






