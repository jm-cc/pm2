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
#include <unistd.h>


// TODO: mettre des LOG_IN / LOG_OUT plutôt (on le fait afficher avec --debug:marcel-log)
// et mettre des PROF_EVENT(bidule) / PROF_EVENT1(bidule, val) / PROF_EVENT2(bidule, val1, val2)


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

/*********************pause**********************************/
DEF_MARCEL_POSIX(int,pause,(void),(),
{
   LOG_IN();
   PROF_EVENT(pause);
   marcel_sigset_t set;
   marcel_sigemptyset(&set);
   marcel_sigmask(SIG_BLOCK,NULL,&set);
   return marcel_sigsuspend(&set);
   LOG_OUT();
})

DEF_C(int,pause,(void),())
DEF___C(int,pause,(void),())

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
	    if (marcel_sigomnislash(&t->curmask,&gsigpending,&t->sigpending,sig))
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
DEF_MARCEL_POSIX(int, raise, (int sig), (sig),
{
   LOG_IN();
 #ifdef MA__DEBUG
   if ((sig < 0) || (sig >= MARCEL_NSIG))
   {
      errno = EINVAL;
      return -1;
   }
#endif
   PROF_EVENT1(raise, sig);
   ksigpending[sig] = 1;
   ma_raise_softirq(MA_SIGNAL_SOFTIRQ);
   
   LOG_OUT();
   return 0;
})

DEF_C(int,raise,(int sig),(sig))
DEF___C(int,raise,(int sig),(sig))


/*********************marcel_pidkill*************************/
static void marcel_pidkill(int sig)
{
   LOG_IN();
   if ((sig < 0) || (sig >= MARCEL_NSIG))
     return;
   ma_irq_enter();
#ifdef MA__DEBUG
   fprintf(stderr,"** external signal %d\n",sig);
#endif
   PROF_EVENT(pidkill);

   ksigpending[sig] = 1;
   ma_raise_softirq_from_hardirq(MA_SIGNAL_SOFTIRQ);
   ma_irq_exit();
   /* TODO: check preempt */

   LOG_OUT();
}
/*********************pthread_kill***************************/
DEF_MARCEL_POSIX(int, kill, (marcel_t thread, int sig), (thread,sig),
{
  LOG_IN();

#ifdef MA__DEBUG
   if ((sig < 0) || (sig >= MARCEL_NSIG))
      return EINVAL;
   if (sig == 0)
   {
      fprintf(stderr,"** case not yet implemented \n");
      return 0;
   }
#endif

   PROF_EVENT(kill);

   ma_spin_lock_softirq(&thread->siglock);
   marcel_sigaddset(&thread->sigpending,sig);
   ma_spin_unlock_softirq(&thread->siglock);
   marcel_deviate(thread,(handler_func_t)marcel_deliver_sig,NULL);
 
   LOG_OUT();
   return 0;
})

DEF_PTHREAD(int, kill, (pthread_t thread, int sig), (thread,sig))
DEF___PTHREAD(int, kill, (pthread_t thread, int sig), (thread,sig))

/*************************pthread_sigmask***********************/
int marcel_sigmask(int how, __const marcel_sigset_t *set, marcel_sigset_t *oset)
{
   LOG_IN();

   int sig;
   marcel_t cthread = marcel_self();
   marcel_sigset_t cset = cthread->curmask;

#ifdef MA__DEBUG
   if ((how != SIG_BLOCK)&&(how != SIG_UNBLOCK)
     &&(how != SIG_SETMASK)) 
      return EINVAL;
#endif

restart:
  
   ma_spin_lock_softirq(&cthread->siglock);
   for (sig = 1; sig < MARCEL_NSIG ; sig++)
      if ((marcel_signandismember(&cthread->sigpending,&cthread->curmask,sig)))
      {
         fprintf(stderr,"for : if : signal %d \n",sig);
	 marcel_sigdelset(&cthread->sigpending, sig);
         ma_spin_unlock_softirq(&cthread->siglock);   
         ma_set_current_state(MA_TASK_RUNNING);
         marcel_call_function(sig);
         goto restart;
      }

   PROF_EVENT(sigmask);

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
      for(sig=1 ; sig < MARCEL_NSIG ; sig++)
         if (marcel_sigismember(set,sig))
            marcel_sigdelset(&cset,sig);
   else fprintf(stderr,"pas de how\n");

   /* SIGKILL et SIGSTOP ne doivent pas être masqués */
   if (marcel_sigismember(&cset,SIGKILL))
      marcel_sigdelset(&cset,SIGKILL);
   if (marcel_sigismember(&cset,SIGSTOP))
      marcel_sigdelset(&cset,SIGSTOP);

   /* si on le masque est le meme, on retourne */
   if (marcel_sigequalset(&cset,set))
   {
      ma_spin_unlock_softirq(&cthread->siglock);
      LOG_OUT();
      return 0;
   }

   cthread->curmask = cset;

#ifdef MA__DEBUG
   for(sig=1 ; sig < MARCEL_NSIG ; sig++)
      if (marcel_sigismember(&cthread->curmask,sig))
         fprintf(stderr,"** signal numero %d maintenant bloque\n",sig);
#endif

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
      ma_spin_unlock_softirq(&cthread->siglock);
      for (sig = 1; sig < MARCEL_NSIG; sig++)
      {
         if (marcel_signandismember(&cthread->sigpending,&cset,sig))
         {    
	    marcel_sigdelset(&cthread->sigpending, sig);
            ma_set_current_state(MA_TASK_RUNNING);
            marcel_call_function(sig);
         }
      }
      ma_spin_lock_softirq(&cthread->siglock);
   }
   ma_spin_unlock_softirq(&cthread->siglock);
   
   LOG_OUT();
   return 0;
}

int pmarcel_sigmask(int how, __const marcel_sigset_t *set, marcel_sigset_t *oset) 
{
   return marcel_sigmask(how, set, oset);
}

#ifdef MA__LIBPTHREAD
int lpt_sigmask(int how,__const sigset_t *set, sigset_t *oset)
{
   int signo,ret;
   marcel_sigset_t marcel_set;
   marcel_sigset_t marcel_oset;
   
   marcel_sigemptyset(&marcel_set);
   for (signo = 1;signo < MARCEL_NSIG; signo ++)
     if (sigismember(set,signo))
        marcel_sigaddset(&marcel_set,signo);
   
   ret = marcel_sigmask(how,&marcel_set,&marcel_oset);
   
   sigemptyset(oset);
   for (signo = 1;signo < MARCEL_NSIG; signo ++)
     if (marcel_sigismember(&marcel_oset,signo))
        sigaddset(oset,signo);
   
   return ret;
}
#endif

DEF_LIBPTHREAD(int, sigmask, (int how, __const sigset_t *set, sigset_t *oset), (how, set, oset))
DEF___LIBPTHREAD(int, sigmask, (int how, __const sigset_t *set, sigset_t *oset), (how, set, oset))

/****************************sigpending**************************/
DEF_MARCEL_POSIX(int, sigpending, (marcel_sigset_t *set), (set),
{
   LOG_IN();
   
   marcel_t cthread;
   cthread = marcel_self();
   int sig;

#ifdef MA__DEBUG
   if (set == NULL)
   {
      errno = EINVAL;
      return -1;
   }
   ma_spin_lock_softirq(&cthread->siglock);
   if (&cthread->sigpending == NULL)
   {
      errno = EINVAL;
      return -1;
   }
   ma_spin_unlock_softirq(&cthread->siglock);
#endif
   
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
   marcel_sigemptyset(&marcel_set);
   for (signo = 1;signo < MARCEL_NSIG; signo ++)
     if (sigismember(set,signo))
        marcel_sigaddset(&marcel_set,signo);
   
   return marcel_sigsuspend(&marcel_set);
}
#endif

DEF_LIBC(int,sigpending,(sigset_t *set),(set))
DEF___LIBC(int,sigpending,(sigset_t *set),(set))

/******************************sigwait*****************************/
DEF_MARCEL_POSIX(int, sigwait, (const marcel_sigset_t *__restrict set, int *__restrict sig), (set,sig),
{
   LOG_IN();

   int signo;
   marcel_t cthread;
   cthread = marcel_self();  
   
#ifdef MA__DEBUG
   if ((set == NULL)||(sig == NULL))
      return EINVAL;
#endif

   PROF_EVENT(sigwait);

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
   
   ma_set_current_state(MA_TASK_INTERRUPTIBLE);  
   ma_spin_unlock_softirq(&gsiglock);
   ma_spin_unlock_softirq(&cthread->siglock);     
   ma_schedule();     
   signo = 0;

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
int marcel_distrib_sigext(int sig)
{
   fprintf(stderr,"* marcel_distrib_sigext\n");
   ma_spin_lock_softirq(&gsiglock);

   ma_twblocked_t *wthread = list_wthreads;
   ma_twblocked_t *prec_wthread = NULL;
   
   for (wthread = list_wthreads, prec_wthread = NULL; 
		   wthread != NULL;
		   prec_wthread = wthread, wthread = wthread->next_twblocked) {
      if (marcel_sigismember(&wthread->waitset,sig))
      {
         *wthread->waitsig = sig;
         prec_wthread->next_twblocked = wthread->next_twblocked;
	 ma_wake_up_state(wthread->t, MA_TASK_INTERRUPTIBLE);
         ma_spin_unlock_softirq(&gsiglock);
         return 1;
      }  
   }
   ma_spin_unlock_softirq(&gsiglock);
   fprintf(stderr,"** pas de thread en attente\n");
   
   return 0;
}

int marcel_distrib_thread(int sig,marcel_t thread)
{
   int ret = 0;
   fprintf(stderr,"* marcel_distrib_thread : %p\n",thread);  
   ma_spin_lock_softirq(&thread->siglock);

   if (marcel_sigismember(&thread->waitset,sig))
   {
      *thread->waitsig = sig;
      marcel_sigemptyset(&thread->waitset);
      fprintf(stderr,"** sig sigwait delivre : %d\n",sig);
      ma_wake_up_state(thread, MA_TASK_INTERRUPTIBLE);
      ret = 1;
   }
   ma_spin_unlock_softirq(&thread->siglock);
  
   return ret;
}

/****************************sigsuspend**************************/
DEF_MARCEL_POSIX(int,sigsuspend,(const marcel_sigset_t *sigmask),(sigmask),
{
   LOG_IN();

   marcel_t cthread;   
   cthread = marcel_self();
   marcel_sigset_t oldmask;

   PROF_EVENT(sigsuspend);
   /* dormir avant de changer le masque */
   ma_set_current_state(MA_TASK_INTERRUPTIBLE);
   marcel_sigmask(SIG_SETMASK,sigmask,&oldmask);
   ma_schedule();

   marcel_sigmask(SIG_SETMASK,&oldmask,NULL);
 
   errno = EINTR;
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
DEF_MARCEL_POSIX(int, sigsetjmp, (sigjmp_buf env, int savemask), (env,savemask),
{
   LOG_IN();

   marcel_t cthread;   
   cthread = marcel_self();
   int valjmp;
#ifdef MA__DEBUG
   if (env == NULL)
   {
      errno = EINVAL;
      return -1;
   }
#endif
   env->__mask_was_saved = savemask;
 
   PROF_EVENT(sigsetjmp);
  
   if (savemask != 0)
   {
      sigset_t set;
      sigemptyset(&set);
      lpt_sigmask(SIG_BLOCK, &set, &env->__saved_mask);
   }
   
   valjmp = setjmp(env->__jmpbuf);

   if (valjmp && env->__mask_was_saved)
      lpt_sigmask(SIG_SETMASK, &env->__saved_mask, NULL);

   LOG_OUT();
   return valjmp;
})

//la glibc ne fournit pas sigsetjmp, seulement __sigsetjmp
//DEF_C(int, sigsetjmp,(sigjmp_buf env, int savemask),(env,savemask))
DEF___C(int, sigsetjmp,(sigjmp_buf env, int savemask),(env,savemask))

DEF_MARCEL_POSIX(void, siglongjmp, (sigjmp_buf env, int val), (env,val),
{
   LOG_IN();

   marcel_t cthread;   
   cthread = marcel_self();
#ifdef MA__DEBUG
   if (env == NULL)
      errno = EINVAL;
#endif

   PROF_EVENT(siglongjmp);

   longjmp(env->__jmpbuf,val);

   LOG_OUT();
})

DEF_C(void,siglongjmp,(sigjmp_buf env, int val),(env,val))
DEF___C(void,siglongjmp,(sigjmp_buf env, int val),(env,val))
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
      errno = EINVAL;
      return SIG_ERR;
   }

   PROF_EVENT(signal);
   
   act.marcel_sa_handler = handler;
   marcel_sigemptyset(&act.marcel_sa_mask);
   marcel_sigaddset(&act.marcel_sa_mask,sig);

   act.marcel_sa_flags = SA_RESTART|SA_NODEFER;//attention au siginfo ; NODEFER est juste là pour optimiser: deliver_sig n'a pas besoin de re-ajouter sig à sa_mask
   if (marcel_sigaction(sig, &act, &oact) < 0)
     return SIG_ERR;

   LOG_OUT();
   return oact.marcel_sa_handler;
})

DEF_C(sighandler_t,signal,(int sig, sighandler_t handler),(sig,handler))
DEF___C(sighandler_t,signal,(int sig, sighandler_t handler),(sig,handler))

/***************************sigaction*****************************/
DEF_MARCEL_POSIX(int,sigaction,(int sig, const struct marcel_sigaction *act,
     struct marcel_sigaction *oact),(sig,act,oact),
{
   LOG_IN();
   struct kernel_sigaction kact;

#ifdef MA__DEBUG
   if ((sig < 1)||(sig >= MARCEL_NSIG))
   {
      errno = EINVAL;
      return -1;
   } 
#endif
   ma_read_lock(&rwlock);
   if (oact != NULL) 
      *oact = csigaction[sig];
   ma_read_unlock(&rwlock);

   PROF_EVENT(sigaction);

   if (!act) 
   {
      LOG_OUT();
      return 0;
   }
 
   if (act->marcel_sa_handler == SIG_IGN)
   {
      if ((sig == SIGKILL)||(sig==SIGSTOP))
      {
#ifdef MA__DEBUG
         fprintf(stderr,"!! signal %d ne peut etre ignore\n",sig);
#endif
         return -1;
      }
   }
   else if (act->marcel_sa_handler != SIG_DFL)
      if ((sig == SIGKILL)||(sig==SIGSTOP))
      {
#ifdef MA__DEBUG
         fprintf(stderr,"!! signal %d ne peut etre capture\n",sig);
#endif
         return -1;
      }
   
   ma_write_lock(&rwlock);
   csigaction[sig] = *act;
   ma_write_unlock(&rwlock);

   sigemptyset(&kact.sa_mask);
   if (act->marcel_sa_handler == SIG_IGN || act->marcel_sa_handler == SIG_DFL)
      kact.sa_handler = act->marcel_sa_handler;
   else
      kact.sa_handler = marcel_pidkill;
   kact.sa_flags = SA_NODEFER | (act->marcel_sa_flags & SA_RESTART);

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
   marcel_act.marcel_sa_handler = act->sa_handler;
   marcel_act.marcel_sa_flags = act->sa_flags;
   marcel_act.marcel_sa_sigaction = act->sa_sigaction;  
   /* a cause du passage de sigset_t a marcel_sigset_t */
   marcel_sigemptyset(&marcel_act.marcel_sa_mask);
   for (signo = 1;signo < MARCEL_NSIG; signo ++)
     if (sigismember(&act->sa_mask,signo))
        marcel_sigaddset(&marcel_act.marcel_sa_mask,signo);
   
   /* appel de marcel_sigaction */
   ret = marcel_sigaction(sig,&marcel_act,&marcel_oact);
   
   /* on rechange tout a l'envers */
   if (oact) {
      oact->sa_handler = marcel_oact.marcel_sa_handler; 
      oact->sa_flags = marcel_oact.marcel_sa_flags; 
      oact->sa_sigaction = marcel_oact.marcel_sa_sigaction; 
   }
   
   for (signo = 1;signo < MARCEL_NSIG; signo ++)
     if (marcel_sigismember(&marcel_oact.marcel_sa_mask,signo))
        sigaddset(&oact->sa_mask,signo);
  
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
   int ret;
   marcel_t cthread = marcel_self();

   PROF_EVENT(deliver_sig);

   ma_spin_lock_softirq(&cthread->siglock);
deliver:
   if (cthread->delivering_sig == 1) 
   {
      cthread->restart_deliver_sig = 1;
      ma_spin_unlock_softirq(&cthread->siglock);
      
      LOG_OUT();
      return 0;     
   }
   else
   {
      cthread->delivering_sig = 1;      
      cthread->restart_deliver_sig = 0;
   }
   ma_spin_unlock_softirq(&cthread->siglock);

   cthread = marcel_self();

   ma_spin_lock_softirq(&cthread->siglock);
   for (sig = 1 ; sig < MARCEL_NSIG ; sig++)
      if (marcel_sigismember(&cthread->sigpending,sig) == 1)
      {
         ret = marcel_sigismember(&cthread->curmask,sig);
         
#ifdef MA__DEBUG
         if (ret == 1)
               fprintf(stderr,"** signal numero %d dans mask -> on ne peut pas encore le delivrer\n",sig);
#endif
         if (ret == 0)
         {
            /* signal non bloqué */
#ifdef MA__DEBUG
            fprintf(stderr,"** signal numero %d non dans mask -> on peut le delivrer\n",sig);
#endif
	    marcel_sigdelset(&cthread->sigpending,sig);
	    ma_spin_unlock_softirq(&cthread->siglock);
            ma_set_current_state(MA_TASK_RUNNING);
            marcel_call_function(sig);
            ma_spin_lock_softirq(&cthread->siglock);
         }
      }
   /* on regarde s'il reste des signaux à délivrer */
   if (cthread->restart_deliver_sig == 1)
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
#ifdef MA__DEBUG
   if ((sig < 0)||(sig >= MARCEL_NSIG))
   {
      errno = EINVAL;
      return -1;
   }
#endif

   struct marcel_sigaction cact;
   ma_read_lock(&rwlock);   
   cact = csigaction[sig];
   ma_read_unlock(&rwlock);   

   PROF_EVENT(call_function);
  
   /* le signal courant doit etre masqué sauf avec SA_NODEFER */
   if ((cact.marcel_sa_flags & SA_NODEFER) == 0)
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
   }
   else if((cact.marcel_sa_flags & SA_SIGINFO))
   {
#ifdef MA__DEBUG
      fprintf(stderr,"** SA_SIGINFO == 1, ENOTSUP\n");
#endif
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
    ma_open_softirq(MA_SIGNAL_SOFTIRQ, marcel_sigtransfer, NULL);
}
