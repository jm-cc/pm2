
#ifndef MARCEL_WORK_EST_DEF
#define MARCEL_WORK_EST_DEF

#ifdef MA__WORK

/* flags indiquant le type de travail à faire */
enum {
  MARCEL_WORK_DEVIATE=0x1,
};

extern volatile int marcel_global_work;
extern marcel_lock_t marcel_work_lock;

#define HAS_WORK(self) (self->has_work || marcel_global_work)
#define LOCK_WORK(self) (marcel_lock_acquire(&marcel_work_lock))
#define UNLOCK_WORK(self) (marcel_lock_release(&marcel_work_lock))

void do_work(marcel_t self);

#else /* MA__WORK */

#define HAS_WORK(self) 0
#define LOCK_WORK(self) ((void)0)
#define UNLOCK_WORK(self) ((void)0)
#define do_work(marcel_self) ((void)0)

#endif /* MA__WORK */

#endif /* MARCEL_WORK_EST_DEF */

