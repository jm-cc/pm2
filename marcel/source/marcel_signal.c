/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "marcel.h"
#include <setjmp.h>
#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

/*********************MA__DEBUG******************************/
#define MA__DEBUG
/*********************variables globales*********************/
static struct marcel_sigaction csigaction[MARCEL_NSIG];
static ma_rwlock_t rwlock = MA_RW_LOCK_UNLOCKED;
/* sigpending processus */
static int ksigpending[MARCEL_NSIG];
static marcel_sigset_t gsigpending;
static ma_spinlock_t gsiglock = MA_SPIN_LOCK_UNLOCKED;
static ma_twblocked_t *list_wthreads;

int marcel_deliver_sig(void);

/*********************pause**********************************/
DEF_MARCEL_POSIX(int,pause,(void),(),
{
   LOG_IN();
   PROF_EVENT(pause);
   marcel_sigset_t set;
   marcel_sigemptyset(&set);
   marcel_sigmask(SIG_BLOCK,NULL,&set);
   LOG_OUT();
	return marcel_sigsuspend(&set);
})

DEF_C(int,pause,(void),())
DEF___C(int,pause,(void),())

/*********************alarm**********************************/
static void alarm_timeout(unsigned long data)
{
   /* alarme expirée, signaler le programme */
   if (!marcel_distribwait_sigext(SIGALRM)) {
      ksigpending[SIGALRM] = 1;  
      ma_raise_softirq(MA_SIGNAL_SOFTIRQ);
   }
}

static struct ma_timer_list alarm_timer = MA_TIMER_INITIALIZER(alarm_timeout, 0, 0);

DEF_MARCEL_POSIX(unsigned int, alarm, (unsigned int nb_sec), (nb_sec),
{
   unsigned int ret = 0;

   ma_del_timer_sync(&alarm_timer);

   if (alarm_timer.expires > ma_jiffies) {
      ret = ((alarm_timer.expires - ma_jiffies) * marcel_gettimeslice()) / 1000000;
   }

   if (nb_sec) {
      ma_init_timer(&alarm_timer);
      alarm_timer.expires = ma_jiffies + (nb_sec * 1000000) / marcel_gettimeslice();
      ma_add_timer(&alarm_timer);
   }

   return ret;
})
DEF_C(unsigned int, alarm, (unsigned int nb_sec), (nb_sec))
DEF___C(unsigned int, alarm, (unsigned int nb_sec), (nb_sec))


/***********************get/setitimer**********************/
static void itimer_timeout(unsigned long data);

static struct ma_timer_list itimer_timer = MA_TIMER_INITIALIZER(itimer_timeout, 0, 0);
unsigned long interval;
unsigned long valeur;

static void itimer_timeout(unsigned long data)
{
   /* alarme expirée, signaler le programme */
   if (!marcel_distribwait_sigext(SIGALRM)) {
      ksigpending[SIGALRM] = 1;  
      ma_raise_softirq(MA_SIGNAL_SOFTIRQ);
   }
   if (interval)
   {
      ma_init_timer(&itimer_timer);
      itimer_timer.expires = ma_jiffies + interval / marcel_gettimeslice();
      ma_add_timer(&itimer_timer);
   }
}

static int check_itimer(int which)
{
   if ((which != ITIMER_REAL)&&(which != ITIMER_VIRTUAL)
	  &&(which != ITIMER_PROF))
   {
#ifdef MA__DEBUG
      fprintf(stderr,"set/getitimer : valeur which invalide !\n");
#endif
      errno = EINVAL;
      return -1;
   }
   if (which != ITIMER_REAL)
	{
#ifdef MA__DEBUG
      fprintf(stderr,"set/getitimer : valeur which non supportee !\n");
#endif
      errno = ENOTSUP;
      return -1;
   }
   return 0;
}

static int check_setitimer(int which,const struct itimerval *value)
{
   int ret = check_itimer(which);
   if (ret)
	   return ret;
   if (value)
	{
	   if ((value->it_value.tv_usec > 999999)
		  ||(value->it_interval.tv_usec > 999999))
		{
#ifdef MA__DEBUG
			fprintf(stderr,"setitimer : valeur temporelle trop elevee !\n");
#endif
         errno = EINVAL;
			return -1;
		}
   }
   return 0;
}
/***********************getitimer**********************/

DEF_MARCEL_POSIX(int, getitimer, (int which, struct itimerval *value), (which, value),
{
  /* error checking */
  int ret = check_itimer(which);
  if (ret)
	 return ret;
  /* error checking end */
  
  value->it_interval.tv_sec = interval / 1000000;
  value->it_interval.tv_usec = interval % 1000000;
  value->it_value.tv_sec = (itimer_timer.expires - ma_jiffies) / 1000000;
  value->it_value.tv_usec = (itimer_timer.expires - ma_jiffies) % 1000000;
  
  return 0;
})

DEF_C(int, getitimer, (int which, struct itimerval *value), (which, value))
DEF___C(int, getitimer, (int which, struct itimerval *value), (which, value))

/************************setitimer***********************/

DEF_MARCEL_POSIX(int, setitimer, (int which, const struct itimerval *value, struct itimerval *ovalue), (which, value, ovalue),
{
  /* error checking */
  unsigned int ret = check_setitimer(which,value);
  if (ret)
	 return ret;
  /* error checking end */

  ma_del_timer_sync(&itimer_timer);
 
  if (ovalue)
  {
     ovalue->it_interval.tv_sec = interval / 1000000;
     ovalue->it_interval.tv_usec = interval % 1000000;
     ovalue->it_value.tv_sec = valeur / 1000000;
     ovalue->it_value.tv_usec = valeur % 1000000;
  }
  
  if (itimer_timer.expires > ma_jiffies) 
  {
     ret = ((itimer_timer.expires - ma_jiffies) * marcel_gettimeslice()) / 1000000;
  }
  
  interval = value->it_interval.tv_sec * 1000000 + value->it_interval.tv_usec;
  valeur = value->it_value.tv_sec * 1000000 + value->it_value.tv_usec;

  if (valeur)
  {
      ma_init_timer(&itimer_timer);
      itimer_timer.expires = ma_jiffies + valeur / marcel_gettimeslice();
      ma_add_timer(&itimer_timer);
  }
  return ret;
})
DEF_C(int, setitimer, (int which, const struct itimerval *value, struct itimerval *ovalue), (which, value, ovalue))
DEF___C(int, setitimer, (int which, const struct itimerval *value, struct itimerval *ovalue), (which, value, ovalue))

/*********************marcel_sigtransfer*********************/
static void marcel_sigtransfer(struct ma_softirq_action *action)
{
   LOG_IN();

   marcel_t t;
   int sig;
   int deliver;
   
   ma_spin_lock_softirq(&gsiglock);
   for (sig = 1; sig < MARCEL_NSIG;sig++)
   {
   if (ksigpending[sig] == 1)
      { 
            marcel_sigaddset(&gsigpending,sig); 
            ksigpending[sig] = 0;
      }
   }  

   PROF_EVENT(sigtransfer);

   struct marcel_topo_level *vp;
   int number = LWP_NUMBER(LWP_SELF);

   for_all_vp_from_begin(vp,number)
   {
      ma_spin_lock_softirq(&ma_topo_vpdata(vp,threadlist_lock));//verrou de liste des threads
      list_for_each_entry(t,&ma_topo_vpdata(vp,all_threads),all_threads)
      {
         ma_spin_lock_softirq(&t->siglock);
         deliver = 0;
         for(sig = 1; sig < MARCEL_NSIG ; sig++)
	      {
            if (marcel_sigomnislash(&t->curmask,&gsigpending,&t->sigpending,sig))//&(!1,2,!3)
            {
               marcel_sigaddset(&t->sigpending,sig);
               marcel_sigdelset(&gsigpending,sig);
               deliver = 1;
            }
         }
         if (deliver)
	         marcel_deviate(t,(handler_func_t)marcel_deliver_sig,NULL);
         ma_spin_unlock_softirq(&t->siglock);
         if (marcel_sigisemptyset(&gsigpending))
            break;
      }
      ma_spin_unlock_softirq(&ma_topo_vpdata(vp,threadlist_lock));//verrou de liste des threads
      if (marcel_sigisemptyset(&gsigpending))
         break;
   }
   for_all_vp_from_end(vp,number)
   ma_spin_unlock_softirq(&gsiglock);
   
   LOG_OUT();
   return;
}

/*****************************raise****************************/
static int check_raise(int sig)
{
   if ((sig < 0) || (sig >= MARCEL_NSIG))
   {
#ifdef MA__DEBUG
      fprintf(stderr,"raise : numero de signal invalide !\n");
#endif
      errno = EINVAL;
      return -1;
   }
   return 0;
}

DEF_MARCEL(int, raise, (int sig), (sig),
{
   LOG_IN();

#ifdef MA__DEBUG
   int ret = check_raise(sig);
   if (ret)
	  return ret;
#endif
  
   marcel_kill(marcel_self(),sig);

   LOG_OUT();
   return 0;
})

DEF_POSIX(int, raise, (int sig), (sig),
{
   LOG_IN();

   /* error checking */
   int ret = check_raise(sig);
   if (ret)
	  return ret;
   /* error checking end */
  
   marcel_kill(marcel_self(),sig);

   LOG_OUT();
   return 0;
})

DEF_C(int,raise,(int sig),(sig))
DEF___C(int,raise,(int sig),(sig))


/*********************marcel_pidkill*************************/
#ifdef SA_SIGINFO
static void marcel_pidkill(int sig, siginfo_t *info, void *uc)
#else
static void marcel_pidkill(int sig)
#endif
{
   LOG_IN();

   if ((sig < 0) || (sig >= MARCEL_NSIG))
     return;

   if (sig == SIGABRT || sig == SIGBUS || sig == SIGILL || sig == SIGFPE || sig == SIGPIPE || sig == SIGSEGV) {
#ifdef SA_SIGINFO
      if (csigaction[sig].marcel_sa_flags & SA_SIGINFO) {
	 if (csigaction[sig].marcel_sa_sigaction != (void*) SIG_DFL &&
	     csigaction[sig].marcel_sa_sigaction != (void*) SIG_IGN) {
	    csigaction[sig].marcel_sa_sigaction(sig, info, uc);
	    return;
	 }
      } else
#endif
	 if (csigaction[sig].marcel_sa_handler != SIG_DFL &&
	     csigaction[sig].marcel_sa_handler != SIG_IGN) {
	    csigaction[sig].marcel_sa_handler(sig);
	    return;
	 }
   }

   ma_irq_enter();
#ifdef MA__DEBUG
	fprintf(stderr,"marcel_pidkill --> external signal %d !!\n",sig);
#endif
   PROF_EVENT1(pidkill,sig);
   if (!marcel_distribwait_sigext(sig))
   {
      ksigpending[sig] = 1;  
      ma_raise_softirq_from_hardirq(MA_SIGNAL_SOFTIRQ);
   }
   ma_irq_exit();
   /* TODO: check preempt */

   LOG_OUT();
}
/*********************pthread_kill***************************/
static int check_kill(marcel_t thread, int sig)
{
   if (!MARCEL_THREAD_ISALIVE(thread))
	  return ESRCH; 
   if ((sig < 0) || (sig >= MARCEL_NSIG))
	{
#ifdef MA__DEBUG
      fprintf(stderr,"(p)marcel_kill : numero de signal invalide !\n");
#endif
      return EINVAL;
   }
   return 0;
}

DEF_MARCEL(int, kill, (marcel_t thread, int sig), (thread,sig),
{
  LOG_IN();

#ifdef MA__DEBUG
   int err = check_kill(thread,sig);
   if (err)
     return err;
#endif
   if (!sig)
	  return 0;

   PROF_EVENT2(kill,thread,sig);

   if (!marcel_distribwait_thread(sig,thread))
   {
      ma_spin_lock_softirq(&thread->siglock);
		marcel_sigaddset(&thread->sigpending,sig);
      ma_spin_unlock_softirq(&thread->siglock);
      marcel_deviate(thread,(handler_func_t)marcel_deliver_sig,NULL);
   }
   LOG_OUT();
   return 0;
})

DEF_POSIX(int, kill, (marcel_t thread, int sig), (thread,sig),
{
   LOG_IN();   

   /* codes d'erreur */
   int err = check_kill(thread,sig);
   if (err)
     return err;
	/* fin codes d'erreur */
	if (!sig)
	  return 0;

   PROF_EVENT2(kill,thread,sig);

   if (!marcel_distribwait_thread(sig,thread))
   {
      ma_spin_lock_softirq(&thread->siglock);
		marcel_sigaddset(&thread->sigpending,sig);
      ma_spin_unlock_softirq(&thread->siglock);
      marcel_deviate(thread,(handler_func_t)marcel_deliver_sig,NULL);
   }
   LOG_OUT();
   return 0;
})

DEF_PTHREAD(int, kill, (pthread_t thread, int sig), (thread,sig))
DEF___PTHREAD(int, kill, (pthread_t thread, int sig), (thread,sig))

/*************************pthread_sigmask***********************/
static int check_sigmask(int how)
{
   if ((how != SIG_BLOCK)&&(how != SIG_UNBLOCK)
     &&(how != SIG_SETMASK)) 
   {
#ifdef MA__DEBUG
     fprintf(stderr,"p&marcel_sigmask : valeur how invalide !\n");
#endif
     return EINVAL;
   }
   return 0;
}

int marcel_sigmask(int how, __const marcel_sigset_t *set, marcel_sigset_t *oset)
{
   LOG_IN();
   
   int sig;
   marcel_t cthread = marcel_self();
   marcel_sigset_t cset = cthread->curmask;

#ifdef MA__DEBUG
	int ret = check_sigmask(how);
   if (ret)
     return ret;
#endif

restart:
  
   ma_spin_lock_softirq(&cthread->siglock);
   for (sig = 1; sig < MARCEL_NSIG ; sig++)
      if ((marcel_signandismember(&cthread->sigpending,&cthread->curmask,sig)))
      {
	      marcel_sigdelset(&cthread->sigpending, sig);
         ma_spin_unlock_softirq(&cthread->siglock);   
         ma_set_current_state(MA_TASK_RUNNING);
         marcel_call_function(sig);
         goto restart;
      }

   PROF_EVENT1(sigmask,how);

   /*********modifs des masques******/
   if (oset != NULL) 
      *oset = cset;
   
   if (!set)
   /* si on ne définit pas de nouveau masque, on retourne */
   {
      ma_spin_unlock_softirq(&cthread->siglock);
      LOG_OUT();
      return 0;
   }

   if (how == SIG_BLOCK)
      marcel_sigorset(&cset,&cset,set);
   else if (how == SIG_SETMASK)
	   cset = *set;
	else if (how == SIG_UNBLOCK)
   {
      for(sig=1 ; sig < MARCEL_NSIG ; sig++)
         if (marcel_sigismember(set,sig))
            marcel_sigdelset(&cset,sig);
   }
   else fprintf(stderr,"sigmask : pas de how\n");

   /* SIGKILL et SIGSTOP ne doivent pas être masqués */
   if (marcel_sigismember(&cset,SIGKILL))
      marcel_sigdelset(&cset,SIGKILL);
   if (marcel_sigismember(&cset,SIGSTOP))
      marcel_sigdelset(&cset,SIGSTOP);

	/* si le nouveau masque est le meme que le masque courant, on retourne */
   if (marcel_sigequalset(&cset,&cthread->curmask))
      {
         ma_spin_unlock_softirq(&cthread->siglock);
         LOG_OUT();
         return 0;
      }

   for(sig=1 ; sig < MARCEL_NSIG ; sig++)
      if (marcel_sigismember(&cset,sig))
         if (csigaction[sig].marcel_sa_handler == SIG_DFL)
	         fprintf(stderr,"pthread_sigmask ne supporte pas les signaux avec SIG_DFL pour sa_handler\n");   

   cthread->curmask = cset;

   /***************traitement des signaux************/
   if (!marcel_signandisempty(&gsigpending,&cset))
   {
      ma_spin_unlock_softirq(&cthread->siglock);
      ma_spin_lock_softirq(&gsiglock);
      ma_spin_lock_softirq(&cthread->siglock);
      if (!marcel_signandisempty(&gsigpending,&cset))
         for (sig = 1; sig < MARCEL_NSIG; sig++)
         {
            if (marcel_signandismember(&gsigpending,&cset,sig)) 
	    {
	       marcel_sigaddset(&cthread->sigpending,sig);
	       marcel_sigdelset(&gsigpending,sig);
	    }
         }
      ma_spin_unlock_softirq(&gsiglock);
   }
   
   while (!marcel_signandisempty(&cthread->sigpending,&cset))
   {
      for (sig = 1; sig < MARCEL_NSIG; sig++)
      {
         if (marcel_signandismember(&cthread->sigpending,&cset,sig))
         {    
	         marcel_sigdelset(&cthread->sigpending, sig);
            ma_spin_unlock_softirq(&cthread->siglock);
            ma_set_current_state(MA_TASK_RUNNING);
            marcel_call_function(sig);
            ma_spin_lock_softirq(&cthread->siglock);
         }
      }
   }
   ma_spin_unlock_softirq(&cthread->siglock);
   
   LOG_OUT();
   return 0;
}

int pmarcel_sigmask(int how, __const marcel_sigset_t *set, marcel_sigset_t *oset) 
{
   /* error checking */
   int ret = check_sigmask(how);
   if (ret)
      return ret;
   /*error checking end */
   
   return marcel_sigmask(how, set, oset);
}

#ifdef MA__LIBPTHREAD
int lpt_sigmask(int how,__const sigset_t *set, sigset_t *oset)
{
   int signo,ret;
   marcel_sigset_t marcel_set;
   marcel_sigset_t marcel_oset;
   
   marcel_sigemptyset(&marcel_set);
   
   if (set)
	{
      for (signo = 1;signo < MARCEL_NSIG; signo ++)
        if (sigismember(set,signo))
           marcel_sigaddset(&marcel_set,signo);
      ret = pmarcel_sigmask(how,&marcel_set,&marcel_oset);
   }
   else
	{
  	   ret = pmarcel_sigmask(how,NULL,&marcel_oset);
	}
   
   if (oset)
   {
      sigemptyset(oset);
      for (signo = 1;signo < MARCEL_NSIG; signo ++)
        if (marcel_sigismember(&marcel_oset,signo))
           sigaddset(oset,signo);
   }

   return ret;
}
#endif

DEF_LIBPTHREAD(int, sigmask, (int how, __const sigset_t *set, sigset_t *oset), (how, set, oset))
DEF___LIBPTHREAD(int, sigmask, (int how, __const sigset_t *set, sigset_t *oset), (how, set, oset))

/***************************sigprocmask*************************/
#ifdef MA__LIBPTHREAD

int lpt_sigprocmask(int how, __const sigset_t *set, sigset_t *oset)
{
   if (marcel_createdthreads() == 0)
	{
	   int ret = lpt_sigmask(how,set,oset);
      if (ret)
		{
	   	errno = ret;
         return -1;
		}
      return 0;
   }
	else
   {
#ifdef MA__DEBUG
     fprintf(stderr,"sigprocmask : multithreading here !!!\n");
#endif
     errno = EPERM;
     return -1;
   }
   return -1;
}

DEF_LIBC(int,sigprocmask,(int how, __const sigset_t *set, sigset_t *oset),(how, set, oset));
DEF___LIBC(int,sigprocmask,(int how, __const sigset_t *set, sigset_t *oset),(how, set, oset));

#endif /*  MA__LIBPTHREAD */

/****************************sigpending**************************/
static int check_sigpending(marcel_sigset_t *set)
{
   marcel_t cthread = marcel_self();
   if (set == NULL)
   {
#ifdef MA__DEBUG
      fprintf(stderr,"(p)marcel_sigpending : valeur set NULL !!!\n");
#endif
      errno = EINVAL;
      return -1;
   }
   ma_spin_lock_softirq(&cthread->siglock);
   if (&cthread->sigpending == NULL)
   {
#ifdef MA__DEBUG
      fprintf(stderr,"(p)marcel_sigpending : valeur de champ des signaux en suspend  NULL !!!\n");
      errno = EINVAL;
#endif
      return -1;
   }
   ma_spin_unlock_softirq(&cthread->siglock);
   return 0;
}

DEF_MARCEL(int, sigpending, (marcel_sigset_t *set), (set),
{
   LOG_IN();

   marcel_t cthread;
   cthread = marcel_self();
   int sig;

#ifdef MA__DEBUG
   int ret = check_sigpending(set);
   if (ret)
	  return ret;
#endif
   
   PROF_EVENT(sigpending);

   marcel_sigemptyset(set);
   ma_spin_lock_softirq(&gsiglock);
   ma_spin_lock_softirq(&cthread->siglock);

   for (sig=1 ; sig < MARCEL_NSIG ; sig++)
   {
      if (marcel_sigorismember(&cthread->sigpending,&gsigpending,sig))
      {
		  //MA_BUG_ON(!marcel_sigismember(&cthread->curmask,sig));
         marcel_sigaddset(set,sig);
      }
   }

   ma_spin_unlock_softirq(&cthread->siglock);
   ma_spin_unlock_softirq(&gsiglock);
 
   LOG_OUT();
   return 0;
})

DEF_POSIX(int, sigpending, (marcel_sigset_t *set), (set),
{
   LOG_IN();
   
   marcel_t cthread = marcel_self();
   int sig;

   /* error checking */
   int ret = check_sigpending(set);
   if (ret)
	  return ret;
	/* error checking end */
   
   PROF_EVENT(sigpending);

   marcel_sigemptyset(set);
   ma_spin_lock_softirq(&gsiglock);
   ma_spin_lock_softirq(&cthread->siglock);
   for (sig=1 ; sig < MARCEL_NSIG ; sig++)
   {

   if (marcel_sigorismember(&cthread->sigpending,&gsigpending,sig))
   {
      MA_BUG_ON(!marcel_sigismember(&cthread->curmask,sig));
      marcel_sigaddset(set,sig);
   }
}
   ma_spin_unlock_softirq(&cthread->siglock);
   ma_spin_unlock_softirq(&gsiglock);
   
   LOG_OUT();
   return 0;
})

#ifdef MA__LIBPTHREAD
int lpt_sigpending(sigset_t *set)
{
   int signo;
   marcel_sigset_t marcel_set;
   
   int ret = marcel_sigpending(&marcel_set);

   sigemptyset(set);
   for (signo = 1;signo < MARCEL_NSIG; signo ++)
     if (marcel_sigismember(&marcel_set,signo))
        sigaddset(set,signo);

 	return ret;
}
#endif

DEF_LIBC(int,sigpending,(sigset_t *set),(set))
DEF___LIBC(int,sigpending,(sigset_t *set),(set))

/******************************sigwait*****************************/
DEF_MARCEL_POSIX(int, sigwait, (const marcel_sigset_t *__restrict set, int *__restrict sig), (set,sig),
{

   LOG_IN();

   int signo;
   marcel_t cthread = marcel_self();  

   if ((set == NULL)||(sig == NULL))
   {
#ifdef MA__DEBUG
      fprintf(stderr,"(p)marcel_sigwait : valeur set ou sig invalide !!!\n");
#endif
      return EINVAL;
	}
   PROF_EVENT1(sigwait,sig);

   int old;
   old = __pmarcel_enable_asynccancel();

   ma_spin_lock_softirq(&gsiglock);
   ma_spin_lock_softirq(&cthread->siglock);
   for (signo = 1 ; signo < MARCEL_NSIG ; signo ++)
      if (marcel_sigismember(set,signo))
         if (marcel_sigorismember(&gsigpending,&cthread->sigpending,signo))
         {  
            if (marcel_sigismember(&cthread->sigpending,signo))
               marcel_sigdelset(&cthread->sigpending,signo);
            marcel_sigdelset(&gsigpending,signo);
            *sig = signo;
            ma_spin_unlock_softirq(&gsiglock);     
            ma_spin_unlock_softirq(&cthread->siglock);     
            
            LOG_OUT();
            return 0;
         }
 
   /* ajout d'un thread en attente d'un signal en début de liste */ 
   ma_twblocked_t wthread;
   wthread.t = cthread;
   marcel_sigemptyset(&wthread.waitset);
   marcel_sigorset(&wthread.waitset,&wthread.waitset,set);
   wthread.waitsig = sig;
   wthread.next_twblocked = list_wthreads;
   list_wthreads = &wthread;

   /* pour le thread lui-meme */
   marcel_sigemptyset(&cthread->waitset);
   marcel_sigorset(&cthread->waitset,&cthread->waitset,set);
   cthread->waitsig = sig;
   
   *sig = -1;

 waiting:
   /* puis on attend */
   ma_set_current_state(MA_TASK_INTERRUPTIBLE);  
   ma_spin_unlock_softirq(&gsiglock);
   ma_spin_unlock_softirq(&cthread->siglock);     
 
   ma_schedule();     
   if(*sig == -1)
  	{
      ma_spin_lock_softirq(&gsiglock);
      ma_spin_lock_softirq(&cthread->siglock);     
      goto waiting; 
   }
   signo = 0;

   __pmarcel_disable_asynccancel(old);
   LOG_OUT();
 
	return 0;
})

#ifdef MA__LIBPTHREAD
int lpt_sigwait(const sigset_t *__restrict set,int *__restrict sig)
{
   int signo;
   marcel_sigset_t marcel_set;
   marcel_sigemptyset(&marcel_set);
   for (signo = 1;signo < MARCEL_NSIG; signo ++)
     if (sigismember(set,signo))
        marcel_sigaddset(&marcel_set,signo);
   
   return marcel_sigwait(&marcel_set,sig);
}
#endif

DEF_LIBC(int, sigwait, (const sigset_t *__restrict set, int *__restrict sig),(set,sig))
DEF___LIBC(int, sigwait, (const sigset_t *__restrict set, int *__restrict sig),(set,sig))
/************************distrib****************************/
int marcel_distribwait_sigext(int sig)
{
   LOG_IN();
   ma_spin_lock_softirq(&gsiglock);

   ma_twblocked_t *wthread = list_wthreads;
   ma_twblocked_t *prec_wthread = NULL;

   PROF_EVENT1(marcel_distribwait_sigext,sig);   
   
   for (wthread = list_wthreads, prec_wthread = NULL; 
		   wthread != NULL;
		   prec_wthread = wthread, wthread = wthread->next_twblocked) {
      if (marcel_sigismember(&wthread->waitset,sig))
      {
         *wthread->waitsig = sig;
         if (prec_wthread != NULL)
         {
			   prec_wthread->next_twblocked = wthread->next_twblocked;
         }
         else /*prec_wthread == NULL */
			{
   		   list_wthreads = wthread->next_twblocked;
			}
	      ma_wake_up_state(wthread->t, MA_TASK_INTERRUPTIBLE);
         ma_spin_unlock_softirq(&gsiglock);
         return 1;
      }  
   }
   ma_spin_unlock_softirq(&gsiglock);
   
   LOG_OUT();

   return 0;
}

int marcel_distribwait_thread(int sig,marcel_t thread)
{
   LOG_IN();
   int ret = 0;
   ma_spin_lock_softirq(&thread->siglock);

   PROF_EVENT2(marcel_distribwait_thread,sig,thread);

   if (marcel_sigismember(&thread->waitset,sig))
   {
      *thread->waitsig = sig;
      marcel_sigemptyset(&thread->waitset);
      ma_wake_up_state(thread, MA_TASK_INTERRUPTIBLE);
      ret = 1; 
   
      /* il faut enlever le thread en fin d'attente de la liste */   
      ma_twblocked_t *wthread = list_wthreads;
      ma_twblocked_t *prec_wthread = NULL;
      
      while (thread != wthread->t)
		{
		   prec_wthread = wthread;
         wthread = wthread->next_twblocked;
		}
      if (prec_wthread == NULL)
 		   list_wthreads = wthread->next_twblocked;
      else 
		  prec_wthread->next_twblocked = wthread->next_twblocked;

   }
   ma_spin_unlock_softirq(&thread->siglock);
  
   LOG_OUT();

   return ret;
}

/****************************sigsuspend**************************/
DEF_MARCEL_POSIX(int,sigsuspend,(const marcel_sigset_t *sigmask),(sigmask),
{
   LOG_IN();
   int old;

   marcel_sigset_t oldmask;

   PROF_EVENT(sigsuspend);
   
   old = __pmarcel_enable_asynccancel();
   /* dormir avant de changer le masque */
   ma_set_current_state(MA_TASK_INTERRUPTIBLE);
   marcel_sigmask(SIG_SETMASK,sigmask,&oldmask);
   ma_schedule();
   marcel_sigmask(SIG_SETMASK,&oldmask,NULL);
   errno = EINTR;
   __pmarcel_disable_asynccancel(old);
   LOG_OUT();

   return -1;
})

#ifdef MA__LIBPTHREAD
int lpt_sigsuspend(const sigset_t *sigmask)
{
   int signo;
   marcel_sigset_t marcel_set;
   marcel_sigemptyset(&marcel_set);
   for (signo = 1;signo < MARCEL_NSIG; signo ++)
     if (sigismember(sigmask,signo))
        marcel_sigaddset(&marcel_set,signo);

   return marcel_sigsuspend(&marcel_set);
}
#endif

DEF_LIBC(int,sigsuspend,(const sigset_t *sigmask),(sigmask));
DEF___LIBC(int,sigsuspend,(const sigset_t *sigmask),(sigmask));

/***********************sigjmps******************************/
#ifdef MA__LIBPTHREAD
#if LINUX_SYS
int ma_savesigs (sigjmp_buf env, int savemask)
{
   LOG_IN();

   marcel_t cthread = marcel_self();
   
	fprintf(stderr,"thread %p entre dans ma_savesigs\n",cthread);
 
#ifdef MA__DEBUG
   if (env == NULL)
   {
      fprintf(stderr,"ma_savesigs : valeur env invalide !!!\n");
      errno = EINVAL;
      return -1;
   }
#endif
   env->__mask_was_saved = savemask;
 
   PROF_EVENT(ma_savesigs);
  
   if (savemask != 0)
   {
      sigset_t set;
      sigemptyset(&set);
      lpt_sigmask(SIG_BLOCK, &set, &env->__saved_mask);
   }
   
   LOG_OUT();

	return 0;
}

extern void __libc_longjmp(jmp_buf env, int val);
DEF_MARCEL_POSIX(void, siglongjmp, (sigjmp_buf env, int val), (env,val),
{
   LOG_IN();

   marcel_t cthread = marcel_self();

	if (env == NULL)
	{
#ifdef MA__DEBUG
      fprintf(stderr,"(p)marcel_siglongjump : valeur env NULL\n");
#endif
      errno = EINVAL;
	}

   PROF_EVENT(siglongjmp);

	if (env->__mask_was_saved)
      lpt_sigmask(SIG_SETMASK, &env->__saved_mask, NULL);

   if (cthread->delivering_sig)
   { 
      cthread->delivering_sig = 0;
      marcel_deliver_sig();
   }

	// TODO: vérifier qu'on longjumpe bien dans le même thread !!

#ifdef X86_ARCH
   longjmp(env->__jmpbuf,val);
#else
   __libc_longjmp(env,val);
#endif

   LOG_OUT();
})

DEF_C(void,siglongjmp,(sigjmp_buf env, int val),(env,val))
DEF___C(void,siglongjmp,(sigjmp_buf env, int val),(env,val))
#undef longjmp
#ifdef MA__LIBPTHREAD
weak_alias (siglongjmp, longjmp);
#endif
#endif // LINUX_SYS
#endif

/***************************signal********************************/
DEF_MARCEL_POSIX(sighandler_t,signal,(int sig, sighandler_t handler),(sig,handler),
{
   LOG_IN();
  
   struct marcel_sigaction act;
   struct marcel_sigaction oact;

   /* Check signal extents to protect __sigismember.  */
   if (handler == SIG_ERR || sig < 0 || sig >= MARCEL_NSIG)
   {
#ifdef MA__DEBUG
      fprintf(stderr,"(p)marcel_signal : handler (%p) ou sig (%d) invalide\n",handler,sig);
#endif
      errno = EINVAL;
      return SIG_ERR;
   }

   PROF_EVENT2(signal,sig,handler);
   
   act.marcel_sa_handler = handler;
   marcel_sigemptyset(&act.marcel_sa_mask);
   marcel_sigaddset(&act.marcel_sa_mask,sig);

   act.marcel_sa_flags = SA_RESTART|SA_NODEFER;//attention au siginfo ; NODEFER est juste là pour optimiser: deliver_sig n'a pas besoin de re-ajouter sig à sa_mask
   if (marcel_sigaction(sig, &act, &oact) < 0)
     return SIG_ERR;

   LOG_OUT();

	return oact.marcel_sa_handler;
})

sighandler_t lpt___sysv_signal(int sig, sighandler_t handler)
{
   LOG_IN();
  
   struct marcel_sigaction act;
   struct marcel_sigaction oact;

   /* Check signal extents to protect __sigismember.  */
   if (handler == SIG_ERR || sig < 0 || sig >= MARCEL_NSIG)
   {
#ifdef MA__DEBUG
      fprintf(stderr,"lpt___sysv_signal : handler (%p) ou sig (%d) invalide\n",handler,sig);
#endif
      errno = EINVAL;
      return SIG_ERR;
   }

   PROF_EVENT2(signal,sig,handler);
   
   act.marcel_sa_handler = handler;
   marcel_sigemptyset(&act.marcel_sa_mask);
   marcel_sigaddset(&act.marcel_sa_mask,sig);

   act.marcel_sa_flags = SA_RESETHAND|SA_NODEFER;//attention au siginfo ; NODEFER est juste là pour optimiser: deliver_sig n'a pas besoin de re-ajouter sig à sa_mask
   if (marcel_sigaction(sig, &act, &oact) < 0)
     return SIG_ERR;

   LOG_OUT();

   return oact.marcel_sa_handler;
}

DEF_C(sighandler_t,signal,(int sig, sighandler_t handler),(sig,handler))
DEF___C(sighandler_t,signal,(int sig, sighandler_t handler),(sig,handler))

DEF_LIBC(sighandler_t,__sysv_signal,(int sig, sighandler_t handler),(sig,handler))
DEF___LIBC(sighandler_t,__sysv_signal,(int sig, sighandler_t handler),(sig,handler))

/***************************sigaction*****************************/
DEF_MARCEL_POSIX(int,sigaction,(int sig, const struct marcel_sigaction *act,
     struct marcel_sigaction *oact),(sig,act,oact),
{
   LOG_IN();
   struct kernel_sigaction kact;
   void *handler;

   if ((sig < 1)||(sig >= MARCEL_NSIG))
   {
#ifdef MA__DEBUG
  	   fprintf(stderr,"(p)marcel_sigaction : valeur sig (%d) invalide\n",sig);
#endif
      errno = EINVAL;
      return -1;
   } 

   if (sig == MARCEL_RESCHED_SIGNAL)
      fprintf(stderr,"!! warning: signal %d not supported\n", sig);

   ma_read_lock(&rwlock);
		if (oact != NULL)
      *oact = csigaction[sig];
   ma_read_unlock(&rwlock);

   PROF_EVENT1(sigaction,sig);

   if (!act) 
   {
      LOG_OUT();
      return 0;
   }
 
#ifdef SA_SIGINFO
   if (act->marcel_sa_flags & SA_SIGINFO)
      handler = act->marcel_sa_sigaction;
   else
#endif
      handler = act->marcel_sa_handler;
  
   if (handler == SIG_IGN)
   {
      if ((sig == SIGKILL)||(sig==SIGSTOP))
      {
#ifdef MA__DEBUG
         fprintf(stderr,"!! signal %d ne peut etre ignore\n",sig);
#endif
         errno = EINVAL;
         return -1;
      }
   }
   else if (handler != SIG_DFL)
      if ((sig == SIGKILL)||(sig==SIGSTOP))
      {
#ifdef MA__DEBUG
         fprintf(stderr,"!! signal %d ne peut etre capture\n",sig);
#endif
         errno = EINVAL;
         return -1;
      }
   
   ma_write_lock(&rwlock);
   csigaction[sig] = *act;
   ma_write_unlock(&rwlock);

   /* don't modify kernel handling for signals that we use */
   if (
#ifdef MARCEL_TIMER_SIGNAL
   	(sig == MARCEL_TIMER_SIGNAL)||
#endif
   	(sig == MARCEL_RESCHED_SIGNAL))
   {
      LOG_OUT();
      return 0;
   }

   /* else, set the kernel handler */
   sigemptyset(&kact.sa_mask);
   if (handler == SIG_IGN || handler == SIG_DFL)
#ifdef SA_SIGINFO
      kact.sa_sigaction = handler;
#else
      kact.sa_handler = handler;
#endif
   else
#ifdef SA_SIGINFO
      kact.sa_sigaction = marcel_pidkill;
#else
      kact.sa_handler = marcel_pidkill;
#endif
   kact.sa_flags = SA_NODEFER |
#ifdef SA_SIGINFO
                   SA_SIGINFO |
#endif
                   (act->marcel_sa_flags & SA_RESTART);

   if(kernel_sigaction(sig,&kact,NULL) == -1)
   {
#ifdef MA__DEBUG
      fprintf(stderr,"!! syscall sigaction rate -> sig : %d\n",sig);
      fprintf(stderr,"!! sortie erreur de marcel_sigaction\n");
#endif
      return -1;
   }

   LOG_OUT();
   
   return 0;
})

#ifdef MA__LIBPTHREAD
int lpt_sigaction(int sig, const struct sigaction *act,
                  struct sigaction *oact)
{
   int signo;
   int ret;
   struct marcel_sigaction marcel_act;
   struct marcel_sigaction marcel_oact;
   
   /* changement de structure act en marcel_act */
   if (act) {
   
      marcel_act.marcel_sa_flags = act->sa_flags;
#ifdef SA_SIGINFO
   if (act->sa_flags & SA_SIGINFO)
      marcel_act.marcel_sa_sigaction = act->sa_sigaction;  
	else
#endif
      marcel_act.marcel_sa_handler = act->sa_handler;
   
      /* a cause du passage de sigset_t a marcel_sigset_t */
      marcel_sigemptyset(&marcel_act.marcel_sa_mask);
      for (signo = 1;signo < MARCEL_NSIG; signo ++)
        if (sigismember(&act->sa_mask,signo))
           marcel_sigaddset(&marcel_act.marcel_sa_mask,signo);
   }
   
   /* appel de marcel_sigaction */
   ret = marcel_sigaction(sig,act?&marcel_act:NULL,oact?&marcel_oact:NULL);
   
   /* on rechange tout a l'envers */
   if (oact) {
      
      oact->sa_flags = marcel_oact.marcel_sa_flags; 
#ifdef SA_SIGINFO
   if (oact->sa_flags & SA_SIGINFO)
      oact->sa_sigaction = marcel_oact.marcel_sa_sigaction; 
	else
#endif
      oact->sa_handler = marcel_oact.marcel_sa_handler; 
   
   sigemptyset(&oact->sa_mask);
   for (signo = 1;signo < MARCEL_NSIG; signo ++)
        if (marcel_sigismember(&marcel_oact.marcel_sa_mask,signo))
           sigaddset(&oact->sa_mask,signo);
  
   }
   
   return ret;
}
#endif

DEF_LIBC(int,sigaction,(int sig, const struct sigaction *act,
                     struct sigaction *oact),(sig,act,oact))
DEF___LIBC(int,sigaction,(int sig, const struct sigaction *act,
                     struct sigaction *oact),(sig,act,oact))

/***********************marcel_deliver_sig**********************/
int marcel_deliver_sig(void)
{
   LOG_IN();

   int sig;
   marcel_t cthread = marcel_self();

   PROF_EVENT(deliver_sig);

   ma_spin_lock_softirq(&cthread->siglock);
 
	cthread->delivering_sig = 1;      
	cthread->restart_deliver_sig = 0;

deliver:
   for (sig = 1 ; sig < MARCEL_NSIG ; sig++)
     if (marcel_sigismember(&cthread->sigpending,sig))//== 1
      {
         if (!marcel_sigismember(&cthread->curmask,sig))
         {
            /* signal non bloqué */

	         marcel_sigdelset(&cthread->sigpending,sig);
  	         ma_spin_unlock_softirq(&cthread->siglock);
            ma_set_current_state(MA_TASK_RUNNING);
            marcel_call_function(sig);
            ma_spin_lock_softirq(&cthread->siglock);
         }
      }
   /* on regarde s'il reste des signaux à délivrer */
   if (cthread->restart_deliver_sig)
   {
      cthread->restart_deliver_sig = 0;
      goto deliver;
   }
   cthread->delivering_sig = 0;      
   ma_spin_unlock_softirq(&cthread->siglock);

   LOG_OUT();

	return 0;     
}

/*************************marcel_call_function******************/
int marcel_call_function(int sig)
{
   LOG_IN();

   if ((sig < 0)||(sig >= MARCEL_NSIG))
   {
#ifdef MA__DEBUG
  	   fprintf(stderr,"marcel_call_function : valeur sig (%d) invalide\n",sig);
#endif
      errno = EINVAL;
      return -1;
   }

   struct marcel_sigaction cact;
   ma_read_lock(&rwlock);   
   cact = csigaction[sig];
   ma_read_unlock(&rwlock);   

   PROF_EVENT1(call_function,sig);
  
   /* le signal courant doit etre masqué sauf avec SA_NODEFER */
   if (!(cact.marcel_sa_flags & SA_NODEFER))//==0
      marcel_sigaddset(&cact.marcel_sa_mask,sig);   

	if (cact.marcel_sa_handler == SIG_IGN)
   {
      LOG_OUT();
      return 0;
   }
   
   if (cact.marcel_sa_handler == SIG_DFL)
   {
      if (syscall(SYS_kill,getpid(),sig) == -1)
         return -1;
      LOG_OUT();
      return 0;
   }
   
   marcel_sigset_t oldmask;
   marcel_sigmask(SIG_BLOCK,&cact.marcel_sa_mask,&oldmask);

	if (!(cact.marcel_sa_flags & SA_RESTART))
      SELF_GETMEM(interrupted) = 1;

   if (!(cact.marcel_sa_flags & SA_SIGINFO))
   {
      /* appel de la fonction */
      cact.marcel_sa_handler(sig); 
      /* voir si SIGINT reprend bien son comportement par defaut ds la norme */
   }
   else if((cact.marcel_sa_flags & SA_SIGINFO))
   {
      fprintf(stderr,"** SA_SIGINFO == 1 for signal %d, ENOTSUP\n", sig);
      errno = ENOTSUP;
      return -1;
   }

   /* on remet l'ancien mask */
   marcel_sigmask(SIG_SETMASK,&oldmask,NULL);

   LOG_OUT();
   return 0;
}

/*******************ma_signals_init***********************/
__marcel_init void ma_signals_init(void)
{
   LOG_IN(); 
   ma_open_softirq(MA_SIGNAL_SOFTIRQ, marcel_sigtransfer, NULL);
   LOG_OUT();
}

/***************testset*********/
void marcel_testset(marcel_sigset_t * set,char *what)
{
   int i;
   fprintf(stderr,"signaux du set %s : ",what);
   for (i=1;i<MARCEL_NSIG;i++)
	   if (marcel_sigismember(set,i))
		   fprintf(stderr,"1");
      else
         fprintf(stderr,"0");
	fprintf(stderr,"\n");
}
