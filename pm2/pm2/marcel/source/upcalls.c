
#ifdef ACT_TIMER
#ifdef CONFIG_ACT_TIMER
#undef CONFIG_ACT_TIMER
#endif
#define CONFIG_ACT_TIMER
#include <timing.h>
#endif

#include <marcel.h>
#include <marcel_alloc.h>
#include <sched.h>
#include <stdlib.h>

#ifdef DEBUG
#include <stdio.h>
#endif

/** This array contain information per activation. It has 
    nb_act_info elements. It can be created dynamicaly before
    the first upcall (no need to have its size hard-coded).
    It MUST BE initialized BEFORE the first upcall.
 */
#define NB_ACT_INFO 10
#define STACK_SIZE 10000

act_info_t *act_info;
int nb_act_info;

/** mutex used for exclusive acces.
    Only one thread can take this lock. It is used for all critical sections.
*/
volatile marcel_t lock_act_thread=NULL;
/* */
volatile int upcall_updates_current_aid=0;
/* if current_aid_is_correct is true, then current_aid is the number
   of the activation in critical section */
volatile int current_aid_is_correct=0;
volatile act_id_t current_aid=0;
/** Contents of the last data has changed (set in an upcall when
    upcall_updates_current_aid is true */
volatile int current_aid_changed;

/** Used when a thead change of activation during act_update */
volatile int in_update=0;
jmp_buf update_jmp;

/* end protection */

/** buffer where the kernel store information about stooped activations */
act_buf_t act_buf[2];

volatile marcel_t thread_to_schedule;
volatile int schedule_new_thread;

void restart_thread(marcel_t next)
{
  act_update(next);

  next->state_ext=MARCEL_RUNNING;

  if (next->sched_by == SCHED_BY_MARCEL) {
    //ACTDEBUG(printf("restart_thread(%p) (normal)\n", next)); 
    longjmp(next->jb.migr_jb, NORMAL_RETURN);
  } else {
    ACTDEBUG(printf("restart_thread(%p) (activation)\n", next)); 
    thread_to_schedule=next;
    schedule_new_thread=1;
    act_send(ACT_SEND_MYSELF);
    /* Then, we wait for the upcall. It's not necessary immediately if
       an other upcall is pending */
    for(;;)
      sched_yield();
    //act_resume(&(desc->jb.act_jb));
  }
}

inline static void act_swap(act_id_t aid_1, act_id_t aid_2)
{
  act_info_t buf;
  buf=act_info[aid_1];
  act_info[aid_1]=act_info[aid_2];
  act_info[aid_2]=buf;
}

static void end_launch_new()
{
  marcel_t next, self;
  act_resume(NULL, 0);
  self=marcel_self();
  do {
    act_lock(self);
    
    next=cur_lwp->__first[0];
    if (next->state_ext != MARCEL_READY) {
      do {
	next=next->next;
      } while ( (next->state_ext != MARCEL_READY) && 
		(next->next != cur_lwp->__first[0]));
      if (next->next == cur_lwp->__first[0])
	goto something_wrong;
    }
    
    self->state_ext=MARCEL_READY;
    next->state_ext=MARCEL_RUNNING;
    ACTDEBUG(printf("act_new launch %p\n", next));  
    //restart_thread(act_update(next));
    restart_thread(next);

    /** Never there normally */
  something_wrong:
    act_unlock(self);

    mdebug("No new thread to launch !!!\n");
  } while (1);
}  

static void launch_new(act_id_t cur_aid)
{
  marcel_t new_task=act_info[cur_aid].start_thread;

  new_task->state_ext=MARCEL_RUNNING;
  new_task->aid=cur_aid;
  act_info[cur_aid].current_thread=new_task;
  act_info[cur_aid].state=ACT_RUNNING;

  call_ST_FLUSH_WINDOWS();
  set_sp(new_task->initial_sp);
  end_launch_new();
}

inline static int try_lock_act_lock(marcel_t self) \
{ int res; \
   res=1; \
   __asm__ __volatile__ \
  ("xorl %%eax, %%eax ;\n" 
#ifdef __SMP__
     "lock ; "
#endif
     "cmpxchgl %1, %2 ;\n" \
     "je 0f ;\n" \
     "movl $0,%0 ;\n" \
     "0:" \
     : "=g" (res) \
     : "r" (self), "m" (lock_act_thread), "g" (res) \
     : "memory", "%eax", "cc" ); return res; }

void act_lock(marcel_t self)
{
#ifdef ACT_VERBOSE
  int i = 0;
#endif
  volatile marcel_t old;

  //ACTDEBUG(printf("act_lock(%p)\n", self)); 

  /** Busy waiting for an other processor to release the lock
      The cmpxchgl is a very useful instruction for that purpose
   */
    
  if (self==lock_act_thread) {
    ACTDEBUG(printf("act_lock(%p) already have the lock before\n", 
		    self));
  }
  old=lock_act_thread;
  while(!try_lock_act_lock(self)) {
    if (self==lock_act_thread) {
      //ACTDEBUG(printf("act_lock(%p) already have the lock\n", 
	//	      self));
      printf("ARGH!! act_lock(%p) already have the lock. Previous was %p\n", 
		      self, old);
      break;
    } 
      
    /* just to do domething, to be sure to handle upcalls, be not
       really necessary */
    ACTDEBUG(i=(++i) % 1000);
    ACTDEBUG(i?0:
	     printf("act_lock(%p) waiting for lock take by %p\n", 
		    self, lock_act_thread)); 
    act_send(ACT_SEND_MYSELF);
  }
  //ACTDEBUG(printf("act_lock(%p) get the lock\n", self)); 

  /** current_aid_changed takes 1 if the thread in lock_act_thread
      change of activation */
  upcall_updates_current_aid=1;
  do {
    current_aid_changed=0;
    current_aid=self->aid;
  } while (current_aid_changed);
  current_aid_is_correct=1;

  //  if (current_aid != 1)
  //  ACTDEBUG(printf("act_locked current_aid=%i\n", current_aid)); 

}

void act_unlock(marcel_t self)
{
  //ACTDEBUG(printf("act_unlock(%p)\n", self)); 
  //marcel_t self=marcel_self();

  //ACTDEBUG(printf("act_lock(%p) release the lock\n", self)); 
  current_aid_is_correct=0;
  upcall_updates_current_aid=0;

  /** We are not any more in critical section */
  lock_act_thread=NULL;
}

marcel_t act_update(marcel_t new)
{
  //ACTDEBUG(printf("act_update(%p)\n", new)); 
  /** the thread running on this activation is changing. We have to
      update the information.
      The difficulty is that it is possible that we change of activation
      during this call... */

  /** There, for now, the only important thing is that lock_act_thread
  would not be NULL. We put the new value that will be used between
  current_aid_is_correct=0; and lock_act_thread=NULL; in act_unlock()
  if there is an upcall there.  */
  lock_act_thread=new;

  if (setjmp(update_jmp)==RETURN_FROM_UPCALL) {
    /** if we return from an upcall, we have to resume to tell the
	kernel that the upcall is ended.
	Before, we made our job, so it's done.  */
    ACTDEBUG(printf("act_update interrupted ! :-)\n")); 
    new->aid=current_aid;
    act_info[current_aid].current_thread=new;
    act_resume(NULL, 0);
  } else {
    in_update=1;
    /** if we have an incorrect value in new->aid, it does not mind */
    new->aid=current_aid;
    /** Therefore, we MUST NOT have an incorrect value in 
	act_info[current_aid].current_thread.
	If current_aid changes during this call, we resume in the other
	part, so that the assignation is not done.
	That means, if current_aid changes before the assignation, then
	the assigantion is not done.
    */
    act_info[current_aid].current_thread=new;
    in_update=0;
  }
  return new;
}

void act_new(act_id_t aid, act_id_t stopped_aid)
{
  ACTDEBUG(printf("act_new(%i, %i)\n", aid, stopped_aid));
  switch (act_info[aid].state) {
  case ACT_UNUSED:
    return launch_new(aid);
    break;
  default:
  }
#ifdef DEBUG
  printf("Error : come to end of act_new\n");
#endif
}

void act_preempt(act_id_t cur_aid, act_id_t stopped_aid, int fast_restart)
{
  act_id_t aid=-1;
  marcel_t cur_thread, stopped_thread;
  int cur_crit, stopped_crit;

#ifdef ACT_TIMER
  unsigned long temps;
  Tick t;

  GET_TICK(t);
  temps = timing_tick2usec(TICK_DIFF(act_buf[1].tick, t));
  printf("time upcall = %ld.%03ldms\n", temps/1000, temps%1000);
  
#endif

  ACTDEBUG(printf("act_preempt(%i, %i, %i)\n", cur_aid, stopped_aid,
		  fast_restart));

  if (! lock_act_thread) {
    /* no one in critical section */
    if (fast_restart)
      act_resume(&act_buf[0], ACT_FAST_RESTART);
  } else {
    /* Some one in critical section */
    if (current_aid_is_correct) {
      aid=current_aid;
    } else {
      aid=lock_act_thread->aid;
    }
  }

  stopped_thread=act_info[stopped_aid].current_thread;
  stopped_crit = (stopped_aid == aid);

  cur_thread=act_info[cur_aid].current_thread;
  cur_crit = (cur_aid == aid);

  if (! stopped_crit) {
    if (! fast_restart) {
      /** We can restart the other thread automatically */

      /** stopped_aid prempted when nothing particular */
      stopped_thread->sched_by=SCHED_BY_ACT;
      stopped_thread->jb.act_jb=act_buf[1];
      /** Once we set to READY, this thread can be rescheduled */
      stopped_thread->state_ext=MARCEL_READY;
      
      ACTDEBUG(printf("marcel thread %p ready\n", stopped_thread));
#ifdef ACT_VERBOSE
      { marcel_t t = stopped_thread;
      ACTDEBUG(printf("marcel_thread info(%p(%i)->%p(%i)->%p(%i)->"
		  "%p(%i)->%p(%i)\n", t, t->state_ext,
		  t->next, t->next->state_ext, 
		  t->next->next, t->next->next->state_ext, 
		  t->next->next->next, t->next->next->next->state_ext, 
		  t->next->next->next->next, 
		  t->next->next->next->next->state_ext));
      }
#endif
      act_info[stopped_aid].state=ACT_UNUSED;
    }

    /** We asked to run another thread */
    if(cur_crit && schedule_new_thread) {
      act_unlock(thread_to_schedule);
      schedule_new_thread=0;
      thread_to_schedule->aid=cur_aid;
      act_resume(&(thread_to_schedule->jb.act_jb), fast_restart);
    }
    act_resume(&act_buf[0], fast_restart);

#ifdef DEBUG
    printf("Error : come to the end in restart\n");
#endif
    exit(-1);
  }

  /* stopped_aid is in critical section :
     we must swap the two activations :-( 
  */

  ACTDEBUG(printf("Swapping two activations\n"));

  cur_thread->jb.act_jb=act_buf[0];
  cur_thread->state_ext=MARCEL_READY;
  cur_thread->sched_by=SCHED_BY_ACT;
  act_info[cur_aid].state=ACT_UNUSED;

  /** Exchange act_info */
  act_swap(cur_aid, stopped_aid);

  stopped_thread->aid=cur_aid;
  if ( upcall_updates_current_aid ) {
    ACTDEBUG(printf("with upcall_updates_current_aid\n"));  
    current_aid=cur_aid;
    current_aid_changed=1;
    if (in_update) {
      longjmp(update_jmp, RETURN_FROM_UPCALL);
    }
  }

  /** We asked to run another thread */
  if(stopped_crit && schedule_new_thread) {
    ACTDEBUG(printf("want running other\n"));  
    act_unlock(thread_to_schedule);
    schedule_new_thread=0;
    thread_to_schedule->aid=cur_aid;
    act_resume(&(thread_to_schedule->jb.act_jb), 0);
  }

  ACTDEBUG(printf("resuming...\n"));
  act_resume(&act_buf[1], 0);    
}

void act_block(act_id_t cur_aid, act_id_t stopped_aid)
{
  ACTDEBUG(printf("act_block(%i, %i)...\n", cur_aid, stopped_aid));
  return act_restart(cur_aid);
}

void act_unblock(act_id_t cur_aid, act_id_t stopped_aid, int fast_restart)
{
  ACTDEBUG(printf("act_unblock(%i, %i, %i)...\n", cur_aid, stopped_aid,
		  fast_restart));
  return act_preempt(cur_aid, stopped_aid, fast_restart);
}

void act_restart(act_id_t cur_aid)
{
  act_id_t aid=-1;
  marcel_t cur_thread;  
  int cur_crit;
  
  ACTDEBUG(printf("act_restart(%i)\n", cur_aid));
  if (lock_act_thread) {
    /* Some one in critical section */
    if (current_aid_is_correct) {
      aid=current_aid;
    } else {
      aid=lock_act_thread->aid;
    }
  }

  cur_thread=act_info[cur_aid].current_thread;
  cur_crit = (cur_aid == aid);

  if(cur_crit && schedule_new_thread) {
    act_unlock(thread_to_schedule);
    schedule_new_thread=0;
    thread_to_schedule->aid=cur_aid;
    act_resume(&(thread_to_schedule->jb.act_jb), 0);
  }

  act_resume(&act_buf[0], 0);

#ifdef DEBUG
  printf("Error : come to the end in restart\n");
#endif
  exit(-1);
}

static void init_act(int i)
{
  char* bottom;
  marcel_t new_task;
  act_info[i].state=ACT_UNUSED;
  bottom = marcel_slot_alloc();
  new_task = (marcel_t)(MAL_BOT((long)bottom + SLOT_SIZE) 
			- MAL(sizeof(task_desc)));
  memset(new_task, 0, sizeof(*new_task));
  new_task->stack_base = bottom;
  new_task->initial_sp = (long)new_task - MAL(1) - 2*WINDOWSIZE;
  act_info[i].start_thread=new_task;

}

static act_param_t act_param;
static int init=0;

void init_upcalls(int nb_act)
{
  int i;
  void *stack;
  
  if (nb_act<=0)
    nb_act=NB_ACT_INFO;

  ACTDEBUG(printf("init_upcalls(%i)\n", nb_act));

  act_info=calloc(sizeof(*act_info), nb_act);

  for(i=0; i<nb_act; i++) {
    init_act(i);
  }

  nb_act_info=nb_act;

  act_param.nb_max_activations=nb_act;
  //act_param.nb_max_activations=1; //TODO
  act_param.kernel_act_struct=malloc(sizeof(struct kernel_act_struct_t));
  stack=malloc(STACK_SIZE+64); /** 4 must be suffisant */
  act_param.act_sp=stack+STACK_SIZE;
  stack=malloc(STACK_SIZE+64); /** 4 must be suffisant */
  act_param.sig_sp=stack+STACK_SIZE;
  act_param.cur_buf=&act_buf[0];
  act_param.stopped_buf=&act_buf[1];
  act_param.act_new    = &act_new;
  act_param.act_preempt= &act_preempt;
  act_param.act_block  = &act_block;
  act_param.act_unblock= &act_unblock;
  act_param.act_restart= &act_restart;

  init=1;
}

void launch_upcalls(int __nb_act_wanted)
{
  int i;

  if (!init) {
    init_upcalls(0);
  }

  ACTDEBUG(printf("act_init called :-)\n"));
  act_init(&act_param); 

  ACTDEBUG(printf("we want %i act in //\n", __nb_act_wanted));
  for(i=1; i<__nb_act_wanted; i++) {
    /* TODO : autant que de processeurs - 1 */
    act_cntl(ACT_CNTL_INC_PROC, NULL);
  }
  act_send(ACT_SEND_MYSELF);

}





